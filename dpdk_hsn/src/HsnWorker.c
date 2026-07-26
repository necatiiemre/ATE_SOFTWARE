/**
 * @file HsnWorker.c
 * @brief HSN TX worker: full-burst Ethernet-only PRBS traffic generation.
 *
 * One worker per (server_port, TX queue). It round-robins through the
 * queue's assigned VL sub-ranges and, independently, through the 509-slot
 * packet-size cycle. Each packet:
 *   - VL-ID -> destination MAC (03:00:00:00:VV:VV)
 *   - per-VL sequence number -> first 8 payload bytes
 *   - PRBS-31 bytes copied from the port cache at offset (seq * MAX_PRBS)
 * There is NO rate limiter: the worker sends as fast as the queue drains
 * (full burst), which is the HSN test's intent.
 *
 * NOTE: not compiled in this environment (no DPDK); mirrors DTN idioms.
 */

#include <stdio.h>
#include <rte_common.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>

#include "HsnWorker.h"
#include "HsnPacket.h"
#include "Packet.h"    /* get_prbs_cache_ext_for_port(), PRBS_CACHE_SIZE */

/* Per-VL TX sequence counters. Each VL-ID is transmitted by exactly one
 * worker, so single-writer -> no atomics. */
static uint64_t hsn_tx_seq[HSN_VL_ID_END + 1];

/* Per-VL software counters (see header). */
uint64_t hsn_tx_pkts[HSN_VL_ID_END + 1];
uint64_t hsn_tx_bytes[HSN_VL_ID_END + 1];

#define HSN_TX_BURST 32

int hsn_tx_worker(void *arg)
{
    struct hsn_tx_params *p = (struct hsn_tx_params *)arg;
    const uint16_t port  = p->port_id;
    const uint16_t queue = p->queue_id;

    uint8_t *prbs = get_prbs_cache_ext_for_port(port);
    if (prbs == NULL) {
        printf("[HSN-TX] port %u queue %u: PRBS cache missing\n", port, queue);
        return -1;
    }

    /* Interleaved packet-size cycle (thread-local). */
    uint16_t size_cycle[HSN_SIZE_CYCLE_PACKETS];
    hsn_build_size_cycle(size_cycle);
    uint32_t size_idx = 0;

    uint16_t fi = 0;      /* current flow index   */
    uint16_t vl_off = 0;  /* offset within flow   */

    struct rte_mbuf *bufs[HSN_TX_BURST];

    printf("[HSN-TX] port %u queue %u started (%u flow(s))\n",
           port, queue, p->flow_count);

    while (!*p->stop) {
        uint16_t nb = 0;

        for (; nb < HSN_TX_BURST; nb++) {
            struct rte_mbuf *m = rte_pktmbuf_alloc(p->pool);
            if (unlikely(m == NULL))
                break;

            const struct hsn_tx_flow *fl = &p->flows[fi];
            uint16_t vl_id   = (uint16_t)(fl->vl_start + vl_off);
            uint16_t payload = size_cycle[size_idx];

            uint64_t seq = hsn_tx_seq[vl_id]++;
            uint64_t off = (seq * (uint64_t)HSN_MAX_PRBS) % (uint64_t)PRBS_CACHE_SIZE;

            uint8_t *data = rte_pktmbuf_mtod(m, uint8_t *);
            uint16_t flen = hsn_build_frame(data, fl->vlan_id, vl_id,
                                            (uint8_t)port, seq,
                                            prbs + off, payload);
            m->data_len = flen;
            m->pkt_len  = flen;
            bufs[nb] = m;

            /* per-VL TX counters (single-writer) */
            hsn_tx_pkts[vl_id]++;
            hsn_tx_bytes[vl_id] += flen;

            /* advance size cycle (independent of VL) */
            if (++size_idx >= HSN_SIZE_CYCLE_PACKETS)
                size_idx = 0;

            /* advance VL, then flow, round-robin */
            if (++vl_off >= fl->vl_count) {
                vl_off = 0;
                if (++fi >= p->flow_count)
                    fi = 0;
            }
        }

        /* Full burst: push everything we built, retrying the tail. */
        uint16_t sent = 0;
        while (sent < nb)
            sent += rte_eth_tx_burst(port, queue, bufs + sent,
                                     (uint16_t)(nb - sent));
    }

    printf("[HSN-TX] port %u queue %u stopped\n", port, queue);
    return 0;
}

/* ============================ RX ============================ */

/* Per-VL RX expected-sequence watermark (single-writer via consistent RSS). */
static uint64_t hsn_rx_expected[HSN_VL_ID_END + 1];

/* Per-VL RX counters (see header). */
uint64_t hsn_rx_pkts[HSN_VL_ID_END + 1];
uint64_t hsn_rx_bytes[HSN_VL_ID_END + 1];
uint64_t hsn_good_pkts[HSN_VL_ID_END + 1];
uint64_t hsn_bad_pkts[HSN_VL_ID_END + 1];
uint64_t hsn_lost_pkts[HSN_VL_ID_END + 1];
uint64_t hsn_dup_pkts[HSN_VL_ID_END + 1];

#define HSN_RX_BURST       64
/* DST(6)+SRC(6): the NIC strips the 802.1Q return tag into mbuf->vlan_tci
 * (RX VLAN-strip offload), so the payload begins right after the two MACs. */
#define HSN_RX_PAYLOAD_OFF 12

int hsn_rx_worker(void *arg)
{
    struct hsn_rx_params *p = (struct hsn_rx_params *)arg;
    const uint16_t port  = p->port_id;
    const uint16_t queue = p->queue_id;
    struct rte_mbuf *bufs[HSN_RX_BURST];

    printf("[HSN-RX] port %u queue %u started\n", port, queue);

    while (!*p->stop) {
        const uint16_t nb = rte_eth_rx_burst(port, queue, bufs, HSN_RX_BURST);

        for (uint16_t i = 0; i < nb; i++) {
            struct rte_mbuf *m = bufs[i];
            const uint8_t *data = rte_pktmbuf_mtod(m, const uint8_t *);
            const uint32_t plen = m->pkt_len;

            if (unlikely(plen < HSN_RX_PAYLOAD_OFF + HSN_SEQ_BYTES)) {
                rte_pktmbuf_free(m);
                continue;
            }

            /* VL-ID from DST MAC[4..5] (present before any VLAN tag). */
            uint16_t vl_id = (uint16_t)((data[4] << 8) | data[5]);
            if (unlikely(vl_id < HSN_VL_ID_START || vl_id > HSN_VL_ID_END)) {
                rte_pktmbuf_free(m);
                continue;
            }

            /* PRBS was generated with the TX port's cache; SRC MAC[5] tells
             * us which. Cross-pairs are same-NUMA, so this cache is local. */
            uint8_t src_port = data[11];
            const uint8_t *prbs_cache = get_prbs_cache_ext_for_port(src_port);

            uint64_t seq;
            memcpy(&seq, data + HSN_RX_PAYLOAD_OFF, HSN_SEQ_BYTES);
            const uint8_t *prbs_rx = data + HSN_RX_PAYLOAD_OFF + HSN_SEQ_BYTES;
            const uint32_t prbs_len = plen - HSN_RX_PAYLOAD_OFF - HSN_SEQ_BYTES;

            /* PRBS content validation (offset recomputed from sequence). */
            if (likely(prbs_cache != NULL)) {
                uint64_t off = (seq * (uint64_t)HSN_MAX_PRBS) % (uint64_t)PRBS_CACHE_SIZE;
                if (memcmp(prbs_rx, prbs_cache + off, prbs_len) == 0)
                    hsn_good_pkts[vl_id]++;
                else
                    hsn_bad_pkts[vl_id]++;
            }

            /* Sequence validation (watermark). */
            uint64_t exp = hsn_rx_expected[vl_id];
            if (seq == exp) {
                hsn_rx_expected[vl_id] = seq + 1;
            } else if (seq > exp) {
                hsn_lost_pkts[vl_id] += (seq - exp);   /* gap = lost packets */
                hsn_rx_expected[vl_id] = seq + 1;
            } else {
                hsn_dup_pkts[vl_id]++;                 /* duplicate / reorder */
            }

            hsn_rx_pkts[vl_id]++;
            hsn_rx_bytes[vl_id] += plen;
            rte_pktmbuf_free(m);
        }
    }

    printf("[HSN-RX] port %u queue %u stopped\n", port, queue);
    return 0;
}

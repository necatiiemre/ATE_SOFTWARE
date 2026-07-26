/**
 * @file HsnPacket.h
 * @brief HSN Ethernet-only packet primitives (size pattern + frame builder).
 *
 * HSN frame (server -> switch), with VLAN tag for switch routing:
 *   [DST MAC 6][SRC MAC 6][VLAN 4][payload]
 * Device-facing (switch strips the VLAN tag at egress-untagged):
 *   [DST MAC 6][SRC MAC 6][payload]
 *
 * No IPv4/UDP, no EtherType field. VL-ID lives in the last 2 bytes of the
 * destination MAC (AFDX style: 03:00:00:00:VV:VV). Payload = [seq 8B][PRBS].
 */
#ifndef HSN_PACKET_H
#define HSN_PACKET_H

#include <stdint.h>
#include <string.h>
#include "Config.h"   /* HSN_SIZE_PATTERN_INIT, HSN_SIZE_CYCLE_PACKETS, ... */

/* L2 sizes */
#define HSN_ETH_HDR_LEN   12               /* DST(6) + SRC(6), no EtherType   */
#define HSN_VLAN_TPID     0x8100           /* 802.1Q tag protocol id          */
#define HSN_VLAN_HDR_LEN  4                /* TPID(2) + TCI(2)                */
#define HSN_L2_LEN        (HSN_ETH_HDR_LEN + HSN_VLAN_HDR_LEN)  /* 16, on wire */
#define HSN_SEQ_BYTES     8

/* PRBS offset uses a constant max so RX can recompute it from the sequence
 * regardless of the (variable) packet size, exactly like DTN's IMIX path. */
#define HSN_MAX_PRBS      (HSN_MAX_PAYLOAD - HSN_SEQ_BYTES)   /* 8992 */

/* Fixed MAC template bytes */
#define HSN_DST_MAC_B0    0x03
#define HSN_SRC_MAC_B0    0x02

/**
 * @brief Build the flattened 509-entry payload-size cycle from the pattern.
 *
 * The sizes are INTERLEAVED (error diffusion) rather than bursted, so the
 * instantaneous packet rate stays close to the average instead of spiking
 * during a run of small packets.
 *
 * @param cycle  out array of length HSN_SIZE_CYCLE_PACKETS
 */
static inline void hsn_build_size_cycle(uint16_t cycle[HSN_SIZE_CYCLE_PACKETS])
{
    const struct hsn_size_entry pat[HSN_SIZE_PATTERN_LEN] = HSN_SIZE_PATTERN_INIT;

    uint32_t total = 0;
    for (int i = 0; i < HSN_SIZE_PATTERN_LEN; i++)
        total += pat[i].count;

    /* Error-diffusion interleave: at each slot, emit the size whose
     * (emitted / count) ratio is furthest behind its target. */
    uint32_t emitted[HSN_SIZE_PATTERN_LEN] = {0};
    for (uint32_t slot = 0; slot < total; slot++) {
        int best = -1;
        double best_deficit = -1.0;
        for (int i = 0; i < HSN_SIZE_PATTERN_LEN; i++) {
            if (emitted[i] >= pat[i].count)
                continue;
            /* deficit = target progress - actual progress */
            double target = (double)pat[i].count * (double)(slot + 1) / (double)total;
            double deficit = target - (double)emitted[i];
            if (deficit > best_deficit) {
                best_deficit = deficit;
                best = i;
            }
        }
        cycle[slot] = pat[best].payload;
        emitted[best]++;
    }
}

/**
 * @brief Build one HSN Ethernet-only frame into a raw buffer.
 *
 * Layout written: [DST 6][SRC 6][0x8100][TCI][seq 8][PRBS payload_size-8].
 *
 * @param buf          output buffer (>= HSN_L2_LEN + payload_size bytes)
 * @param vlan_id      802.1Q VLAN id (switch routing; stripped before device)
 * @param vl_id        16-bit VL-ID (written into DST MAC[4..5])
 * @param src_port     server port id (written into SRC MAC[5])
 * @param seq          per-VL sequence number (first 8 bytes of payload)
 * @param prbs_src     pointer to PRBS bytes to copy (>= payload_size-8 bytes)
 * @param payload_size total payload bytes (seq + PRBS); 128..HSN_MAX_PAYLOAD
 * @return on-wire frame length (HSN_L2_LEN + payload_size)
 */
static inline uint16_t hsn_build_frame(uint8_t *buf, uint16_t vlan_id, uint16_t vl_id,
                                       uint8_t src_port, uint64_t seq,
                                       const uint8_t *prbs_src, uint16_t payload_size)
{
    /* Destination MAC: 03:00:00:00:VV:VV */
    buf[0] = HSN_DST_MAC_B0;
    buf[1] = 0x00; buf[2] = 0x00; buf[3] = 0x00;
    buf[4] = (uint8_t)(vl_id >> 8);
    buf[5] = (uint8_t)(vl_id & 0xFF);

    /* Source MAC: 02:00:00:00:00:PP */
    buf[6] = HSN_SRC_MAC_B0;
    buf[7] = 0x00; buf[8] = 0x00; buf[9] = 0x00; buf[10] = 0x00;
    buf[11] = src_port;

    /* 802.1Q tag: TPID + TCI (priority 0, VID = vlan_id), big-endian on wire */
    buf[12] = (uint8_t)(HSN_VLAN_TPID >> 8);
    buf[13] = (uint8_t)(HSN_VLAN_TPID & 0xFF);
    buf[14] = (uint8_t)((vlan_id >> 8) & 0x0F);
    buf[15] = (uint8_t)(vlan_id & 0xFF);

    /* Payload: [seq 8][PRBS] */
    uint8_t *pl = buf + HSN_L2_LEN;
    memcpy(pl, &seq, HSN_SEQ_BYTES);
    uint16_t prbs_len = (uint16_t)(payload_size - HSN_SEQ_BYTES);
    memcpy(pl + HSN_SEQ_BYTES, prbs_src, prbs_len);

    return (uint16_t)(HSN_L2_LEN + payload_size);
}

#endif /* HSN_PACKET_H */

/**
 * @file HsnWorker.h
 * @brief HSN TX/RX workers (Ethernet-only, full-burst PRBS traffic).
 */
#ifndef HSN_WORKER_H
#define HSN_WORKER_H

#include <stdint.h>
#include <stdbool.h>
#include <rte_mbuf.h>
#include "Config.h"

/* One (VLAN, VL sub-range) a TX worker drives. A 100G-port queue gets one
 * flow (a slice of the port's 304 VLs, same VLAN); a 10G-port queue gets a
 * few flows (one per device port it covers). */
struct hsn_tx_flow {
    uint16_t vlan_id;
    uint16_t vl_start;
    uint16_t vl_count;
};

struct hsn_tx_params {
    uint16_t port_id;
    uint16_t queue_id;
    struct rte_mempool *pool;
    uint16_t flow_count;
    struct hsn_tx_flow flows[HSN_MAX_FLOWS_PER_PORT];
    volatile bool *stop;
};

/* Per-VL software counters (single-writer per VL: one TX worker owns each
 * VL, one RX worker owns each VL via consistent RSS). Read by the stats
 * table. Indexed by VL-ID (0..HSN_VL_ID_END). */
extern uint64_t hsn_tx_pkts[HSN_VL_ID_END + 1];
extern uint64_t hsn_tx_bytes[HSN_VL_ID_END + 1];

/* TX worker entry point (rte_eal_remote_launch target). arg = hsn_tx_params*. */
int hsn_tx_worker(void *arg);

/* ---- RX ---- */

struct hsn_rx_params {
    uint16_t port_id;
    uint16_t queue_id;
    volatile bool *stop;
};

/* Per-VL RX software counters (single-writer via consistent RSS on the
 * VL-ID-bearing DST MAC). Read by the stats table. */
extern uint64_t hsn_rx_pkts[HSN_VL_ID_END + 1];
extern uint64_t hsn_rx_bytes[HSN_VL_ID_END + 1];
extern uint64_t hsn_good_pkts[HSN_VL_ID_END + 1];   /* PRBS content OK      */
extern uint64_t hsn_bad_pkts[HSN_VL_ID_END + 1];    /* PRBS mismatch        */
extern uint64_t hsn_lost_pkts[HSN_VL_ID_END + 1];   /* sequence gaps        */
extern uint64_t hsn_dup_pkts[HSN_VL_ID_END + 1];    /* duplicate / reorder  */

/* RX worker entry point (rte_eal_remote_launch target). arg = hsn_rx_params*. */
int hsn_rx_worker(void *arg);

#endif /* HSN_WORKER_H */

#include "Helpers.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <rte_ethdev.h>
#include <rte_atomic.h>

#include "Config.h"
#include "Packet.h"       // for PACKET_SIZE (byte approximation for per-CMC stats)
#include "TxRxManager.h"  // for rx_stats_per_port
#include "AteMode.h"

// Daemon mode flag - when true, ANSI escape codes are disabled
bool g_daemon_mode = false;

void helper_set_daemon_mode(bool enabled) {
    g_daemon_mode = enabled;
}

// Helper functions
static inline double to_gbps(uint64_t bytes) {
    return (bytes * 8.0) / 1e9;
}

#if STATS_MODE_CMC
static void reset_cmc_prev_bytes(void);
#endif

void helper_reset_stats(const struct ports_config *ports_config,
                        uint64_t prev_tx_bytes[], uint64_t prev_rx_bytes[])
{
    // Reset HW statistics and zero out prev_* counters
    for (uint16_t i = 0; i < ports_config->nb_ports; i++) {
        uint16_t port_id = ports_config->ports[i].port_id;
        rte_eth_stats_reset(port_id);
        prev_tx_bytes[port_id] = 0;
        prev_rx_bytes[port_id] = 0;
    }

    // Reset RX validation statistics (PRBS) + per-VL sequence trackers
    init_rx_stats();

    // Reset TX VL-ID commit counters so shared-queue CMC RX
    // (Σ tx_vl_sequence) does not carry warm-up residue into the test window.
    reset_tx_vl_sequences();

#if STATS_MODE_CMC
    init_cmc_stats();
    reset_cmc_prev_bytes();
#endif
}

#if STATS_MODE_CMC
// ==========================================
// CMC PORT-BASED STATISTICS - 2-TABLE DISPLAY (DSM-A / DSM-B)
// ==========================================
// CMC has exactly two lines. Each gets its own table; layout & columns are
// the same as the legacy CMC tables (Good / Bad / SplitMix64 Fail / CRC32
// Fail / Loss / Bit Error / BER) so the operator sees a familiar shape.
// CMC TX (CMC→Server) = Server RX = HW q_ipackets[queue]
// CMC RX (Server→CMC) = Server TX = HW q_opackets[queue]
// PRBS = cmc_stats[cmc_port] (from RX worker)

// Per-CMC-port prev bytes for delta calculation
static uint64_t cmc_prev_tx_bytes[CMC_PORT_COUNT];
static uint64_t cmc_prev_rx_bytes[CMC_PORT_COUNT];

// Zero the per-CMC prev-byte baselines. Invoked from helper_reset_stats at
// warm-up → test transition so the first test-window Gbps delta is computed
// against zero, not the warm-up byte total.
static void reset_cmc_prev_bytes(void)
{
    for (uint16_t i = 0; i < CMC_PORT_COUNT; i++) {
        cmc_prev_tx_bytes[i] = 0;
        cmc_prev_rx_bytes[i] = 0;
    }
}

// Count CMC flows that share the same server RX (queue) — used to decide
// whether the HW per-queue counter is dedicated to this CMC (single flow)
// or must be split via the software per-VL-ID counters (shared queue).
static uint16_t cmc_flows_on_srv_rx(uint16_t srv_rx_port, uint16_t srv_rx_queue)
{
    uint16_t n = 0;
    for (uint16_t i = 0; i < CMC_PORT_COUNT; i++) {
        if (cmc_port_map[i].tx_server_port == srv_rx_port &&
            cmc_port_map[i].tx_server_queue == srv_rx_queue) {
            n++;
        }
    }
    return n;
}

static uint16_t cmc_flows_on_srv_tx(uint16_t srv_tx_port, uint16_t srv_tx_queue)
{
    uint16_t n = 0;
    for (uint16_t i = 0; i < CMC_PORT_COUNT; i++) {
        if (cmc_port_map[i].rx_server_port == srv_tx_port &&
            cmc_port_map[i].rx_server_queue == srv_tx_queue) {
            n++;
        }
    }
    return n;
}

static void print_cmc_table_group(const uint16_t *indices, uint16_t count,
                                  const struct rte_eth_stats port_hw_stats[])
{
    // Payload Verification now has 8 stat columns (Good, Bad, SplitMix64
    // Fail, CRC32 Fail, XOR Fail, Loss, Bit Error, BER) — XOR Fail was added
    // for the 1-byte XOR-zone check the CMC applies after CRC32.
    printf("  ┌─────────┬────────┬─────────────────────────────────────────────────────────────────────┬─────────────────────────────────────────────────────────────────────┬──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐\n");
    printf("  │ Server  │  CMC   │                          CMC TX (CMC→Server)                        │                          CMC RX (Server→CMC)                        │                                                  Payload Verification                                                                                  │\n");
    printf("  │  Port   │  Port  ├─────────────────────┬─────────────────────┬─────────────────────────┼─────────────────────┬─────────────────────┬─────────────────────────┼─────────────────────┬─────────────────────┬─────────────────────┬─────────────────────┬─────────────────────┬─────────────────────┬─────────────────────┬─────────────┤\n");
    printf("  │         │        │       Packets       │        Bytes        │          Gbps           │       Packets       │        Bytes        │          Gbps           │        Good         │         Bad         │  SplitMix64 Fail    │    CRC32 Fail       │      XOR Fail       │        Loss         │      Bit Error      │     BER     │\n");
    printf("  ├─────────┼────────┼─────────────────────┼─────────────────────┼─────────────────────────┼─────────────────────┼─────────────────────┼─────────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────┤\n");

    for (uint16_t i = 0; i < count; i++) {
        uint16_t cmc = indices[i];
        const struct cmc_port_map_entry *entry = &cmc_port_map[cmc];

        uint16_t srv_rx_port = entry->tx_server_port;
        uint16_t srv_rx_queue = entry->tx_server_queue;
        uint16_t srv_tx_port = entry->rx_server_port;
        uint16_t srv_tx_queue = entry->rx_server_queue;

        uint64_t cmc_tx_pkts, cmc_tx_bytes;
        uint64_t cmc_rx_pkts, cmc_rx_bytes;

        // CMC TX (CMC→Server, server RX side).
        // Use HW q_ipackets/q_ibytes when this CMC is the only flow on that
        // RX queue; otherwise split via software per-CMC rx counters.
        if (cmc_flows_on_srv_rx(srv_rx_port, srv_rx_queue) == 1) {
            cmc_tx_pkts = port_hw_stats[srv_rx_port].q_ipackets[srv_rx_queue];
            cmc_tx_bytes = port_hw_stats[srv_rx_port].q_ibytes[srv_rx_queue];
        } else {
            cmc_tx_pkts = rte_atomic64_read(&cmc_stats[cmc].total_rx_pkts);
            cmc_tx_bytes = cmc_tx_pkts * (uint64_t)PACKET_SIZE;
        }

        // CMC RX (Server→CMC, server TX side).
        // Use HW q_opackets/q_obytes for dedicated queues; otherwise sum the
        // per-VL-ID TX sequence counters over this CMC's TX range.
        if (cmc_flows_on_srv_tx(srv_tx_port, srv_tx_queue) == 1) {
            cmc_rx_pkts = port_hw_stats[srv_tx_port].q_opackets[srv_tx_queue];
            cmc_rx_bytes = port_hw_stats[srv_tx_port].q_obytes[srv_tx_queue];
        } else {
            uint64_t sum = 0;
            for (uint16_t v = 0; v < entry->vl_id_count; v++) {
                sum += get_tx_vl_sequence(srv_tx_port,
                                          (uint16_t)(entry->tx_vl_id_start + v));
            }
            cmc_rx_pkts = sum;
            cmc_rx_bytes = sum * (uint64_t)PACKET_SIZE;
        }

        // Gbps delta calculation
        uint64_t tx_delta = cmc_tx_bytes - cmc_prev_tx_bytes[cmc];
        uint64_t rx_delta = cmc_rx_bytes - cmc_prev_rx_bytes[cmc];
        double tx_gbps = to_gbps(tx_delta);
        double rx_gbps = to_gbps(rx_delta);

        // Update prev values
        cmc_prev_tx_bytes[cmc] = cmc_tx_bytes;
        cmc_prev_rx_bytes[cmc] = cmc_rx_bytes;

        // Payload verification statistics
        uint64_t good = rte_atomic64_read(&cmc_stats[cmc].good_pkts);
        uint64_t bad = rte_atomic64_read(&cmc_stats[cmc].bad_pkts);
        uint64_t sm_fail = rte_atomic64_read(&cmc_stats[cmc].splitmix_fail);
        uint64_t crc_fail = rte_atomic64_read(&cmc_stats[cmc].crc32_fail);
        uint64_t xor_fail = rte_atomic64_read(&cmc_stats[cmc].xor_fail);
        uint64_t lost = rte_atomic64_read(&cmc_stats[cmc].lost_pkts);
        uint64_t bit_errors_raw = rte_atomic64_read(&cmc_stats[cmc].bit_errors);

#if IMIX_ENABLED
        uint64_t lost_bits = lost * (uint64_t)IMIX_AVG_PACKET_SIZE * 8;
#else
        uint64_t lost_bits = lost * (uint64_t)PACKET_SIZE * 8;
#endif
        uint64_t bit_errors = bit_errors_raw + lost_bits;

        double ber = 0.0;
        uint64_t total_bits = cmc_tx_bytes * 8 + lost_bits;
        if (total_bits > 0) {
            ber = (double)bit_errors / (double)total_bits;
        }

        printf("  │    %u    │ %-6s │ %19lu │ %19lu │ %23.4f │ %19lu │ %19lu │ %23.4f │ %19lu │ %19lu │ %19lu │ %19lu │ %19lu │ %19lu │ %19lu │ %11.2e │\n",
               entry->rx_server_port,
               cmc_port_labels[cmc],
               cmc_tx_pkts, cmc_tx_bytes, tx_gbps,
               cmc_rx_pkts, cmc_rx_bytes, rx_gbps,
               good, bad, sm_fail, crc_fail, xor_fail, lost, bit_errors, ber);
    }

    printf("  └─────────┴────────┴─────────────────────┴─────────────────────┴─────────────────────────┴─────────────────────┴─────────────────────┴─────────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────┘\n");
}

// ==========================================
// PRBS-ONLY SUMMARY (final report)
// ==========================================
// The live table above reports the hardware per-queue counters, which is the
// right thing for watching the wire: they show everything that actually moved,
// PRBS and health-monitor traffic alike. It is the wrong thing for the
// end-of-run verdict, where "how much PRBS did I send and get back" should not
// have health-monitor packets folded into it.
//
// So this table is built entirely from the software counters, which are
// incremented after classification and therefore know what each packet was:
//
//   TX packets = sum of the per-VL TX counters over the line's VL range. Taken
//                from the same counters that back the VL-to-VL table, so the
//                two reports agree by construction rather than by luck.
//   TX bytes   = per-line byte total accumulated in the TX worker.
//   RX packets/bytes = PRBS packets that reached payload verification.
//
// Health-monitor traffic gets its own table, and a third one reconciles both
// against the hardware counter so anything unaccounted for is visible.

static uint64_t prbs_tx_pkts_for_line(uint16_t cmc)
{
    const struct cmc_port_map_entry *e = &cmc_port_map[cmc];
    uint64_t sum = 0;
    for (uint16_t i = 0; i < e->vl_id_count; i++) {
        uint16_t vl = (uint16_t)(e->tx_vl_id_start + i);
        if (vl <= MAX_VL_ID) {
            sum += vl_tx_counts[cmc][vl];
        }
    }
    return sum;
}

#define PRBS_RULE \
    "  +--------+--------------+----------------+--------------+----------------+--------------+--------------+------------+------------+------------+--------------+----------------+-------------+\n"

void helper_print_prbs_summary(const struct ports_config *ports_config,
                               uint32_t test_seconds)
{
    // Zeroed up front: the loop only fills entries for configured ports, and
    // the accounting table below indexes by the port recorded in the CMC map.
    struct rte_eth_stats hw[MAX_PORTS];
    memset(hw, 0, sizeof(hw));

    for (uint16_t i = 0; i < ports_config->nb_ports; i++) {
        uint16_t port_id = ports_config->ports[i].port_id;
        if (rte_eth_stats_get(port_id, &hw[port_id]) != 0) {
            memset(&hw[port_id], 0, sizeof(struct rte_eth_stats));
        }
    }

    printf("\n");
    printf("================================================================================\n");
    printf("  PRBS TRAFFIC — FINAL TOTALS (health-monitor traffic excluded)\n");
    printf("  Test duration: %u s after warm-up\n", test_seconds);
    printf("================================================================================\n");

    fputs(PRBS_RULE, stdout);
    printf("  | %-6s | %12s | %14s | %12s | %14s | %12s | %12s | %10s | %10s | %10s | %12s | %14s | %11s |\n",
           "Line", "TX pkts", "TX bytes", "RX pkts", "RX bytes", "Good", "Bad",
           "SM fail", "CRC fail", "XOR fail", "Loss", "Bit errors", "BER");
    fputs(PRBS_RULE, stdout);

    uint64_t t_tx_p = 0, t_tx_b = 0, t_rx_p = 0, t_rx_b = 0;
    uint64_t t_good = 0, t_bad = 0, t_sm = 0, t_crc = 0, t_xor = 0;
    uint64_t t_lost = 0, t_bits = 0;

    for (uint16_t cmc = 0; cmc < CMC_PORT_COUNT; cmc++) {
        uint64_t tx_p = prbs_tx_pkts_for_line(cmc);
        uint64_t tx_b = cmc_tx_bytes_total[cmc];
        uint64_t rx_p = (uint64_t)rte_atomic64_read(&cmc_stats[cmc].total_rx_pkts);
        uint64_t rx_b = (uint64_t)rte_atomic64_read(&cmc_stats[cmc].rx_bytes);
        uint64_t good = (uint64_t)rte_atomic64_read(&cmc_stats[cmc].good_pkts);
        uint64_t bad  = (uint64_t)rte_atomic64_read(&cmc_stats[cmc].bad_pkts);
        uint64_t sm   = (uint64_t)rte_atomic64_read(&cmc_stats[cmc].splitmix_fail);
        uint64_t crc  = (uint64_t)rte_atomic64_read(&cmc_stats[cmc].crc32_fail);
        uint64_t xr   = (uint64_t)rte_atomic64_read(&cmc_stats[cmc].xor_fail);
        uint64_t lost = (uint64_t)rte_atomic64_read(&cmc_stats[cmc].lost_pkts);
        uint64_t braw = (uint64_t)rte_atomic64_read(&cmc_stats[cmc].bit_errors);

#if IMIX_ENABLED
        uint64_t lost_bits = lost * (uint64_t)IMIX_AVG_PACKET_SIZE * 8;
#else
        uint64_t lost_bits = lost * (uint64_t)PACKET_SIZE * 8;
#endif
        uint64_t bit_errors = braw + lost_bits;
        uint64_t total_bits = rx_b * 8 + lost_bits;
        double ber = (total_bits > 0) ? ((double)bit_errors / (double)total_bits) : 0.0;

        printf("  | %-6s | %12lu | %14lu | %12lu | %14lu | %12lu | %12lu | %10lu | %10lu | %10lu | %12lu | %14lu | %11.2e |\n",
               cmc_port_labels[cmc], tx_p, tx_b, rx_p, rx_b, good, bad,
               sm, crc, xr, lost, bit_errors, ber);

        t_tx_p += tx_p; t_tx_b += tx_b; t_rx_p += rx_p; t_rx_b += rx_b;
        t_good += good; t_bad += bad; t_sm += sm; t_crc += crc; t_xor += xr;
        t_lost += lost; t_bits += bit_errors;
    }

    fputs(PRBS_RULE, stdout);
    {
#if IMIX_ENABLED
        uint64_t t_lost_bits = t_lost * (uint64_t)IMIX_AVG_PACKET_SIZE * 8;
#else
        uint64_t t_lost_bits = t_lost * (uint64_t)PACKET_SIZE * 8;
#endif
        uint64_t t_total_bits = t_rx_b * 8 + t_lost_bits;
        double t_ber = (t_total_bits > 0) ? ((double)t_bits / (double)t_total_bits) : 0.0;
        printf("  | %-6s | %12lu | %14lu | %12lu | %14lu | %12lu | %12lu | %10lu | %10lu | %10lu | %12lu | %14lu | %11.2e |\n",
               "TOTAL", t_tx_p, t_tx_b, t_rx_p, t_rx_b, t_good, t_bad,
               t_sm, t_crc, t_xor, t_lost, t_bits, t_ber);
        fputs(PRBS_RULE, stdout);
    }

    printf("\n  PRBS delivery: %lu of %lu packets returned", t_rx_p, t_tx_p);
    if (t_tx_p > 0) {
        printf("  (%.6f%% missing)",
               (double)(t_tx_p > t_rx_p ? t_tx_p - t_rx_p : 0) * 100.0 / (double)t_tx_p);
    }
    printf("\n");

    // ---- Health monitor, counted separately ----
    printf("\n");
    printf("  === HEALTH MONITOR TRAFFIC (not included in the PRBS totals above) ===\n");
    printf("  +--------+--------------+----------------+\n");
    printf("  | %-6s | %12s | %14s |\n", "Line", "HM RX pkts", "HM RX bytes");
    printf("  +--------+--------------+----------------+\n");

    uint64_t t_hm_p = 0, t_hm_b = 0;
    for (uint16_t cmc = 0; cmc < CMC_PORT_COUNT; cmc++) {
        uint64_t hp = (uint64_t)rte_atomic64_read(&cmc_stats[cmc].hm_rx_pkts);
        uint64_t hb = (uint64_t)rte_atomic64_read(&cmc_stats[cmc].hm_rx_bytes);
        printf("  | %-6s | %12lu | %14lu |\n", cmc_port_labels[cmc], hp, hb);
        t_hm_p += hp; t_hm_b += hb;
    }
    printf("  +--------+--------------+----------------+\n");
    printf("  | %-6s | %12lu | %14lu |\n", "TOTAL", t_hm_p, t_hm_b);
    printf("  +--------+--------------+----------------+\n");

    // ---- Reconciliation against the hardware counter ----
    // If this does not add up, one of the software counters is missing a path;
    // showing the difference is better than quietly presenting totals that do
    // not match the NIC.
    printf("\n");
    printf("  === RX ACCOUNTING (hardware queue counter vs. classified traffic) ===\n");
    printf("  +--------+--------------+--------------+--------------+--------------+--------------+\n");
    printf("  | %-6s | %12s | %12s | %12s | %12s | %12s |\n",
           "Line", "HW RX pkts", "PRBS", "Health mon", "Other/short", "Unaccounted");
    printf("  +--------+--------------+--------------+--------------+--------------+--------------+\n");

    for (uint16_t cmc = 0; cmc < CMC_PORT_COUNT; cmc++) {
        const struct cmc_port_map_entry *e = &cmc_port_map[cmc];
        uint64_t hw_p = 0;
        if (e->tx_server_port < MAX_PORTS &&
            e->tx_server_queue < RTE_ETHDEV_QUEUE_STAT_CNTRS) {
            hw_p = hw[e->tx_server_port].q_ipackets[e->tx_server_queue];
        }
        uint64_t prbs_p  = (uint64_t)rte_atomic64_read(&cmc_stats[cmc].total_rx_pkts);
        uint64_t hm_p    = (uint64_t)rte_atomic64_read(&cmc_stats[cmc].hm_rx_pkts);
        uint64_t other_p = (uint64_t)rte_atomic64_read(&cmc_stats[cmc].other_rx_pkts);
        uint64_t seen    = prbs_p + hm_p + other_p;

        char unacc[24];
        if (hw_p >= seen) {
            snprintf(unacc, sizeof(unacc), "%lu", hw_p - seen);
        } else {
            snprintf(unacc, sizeof(unacc), "-%lu", seen - hw_p);
        }

        printf("  | %-6s | %12lu | %12lu | %12lu | %12lu | %12s |\n",
               cmc_port_labels[cmc], hw_p, prbs_p, hm_p, other_p, unacc);
    }
    printf("  +--------+--------------+--------------+--------------+--------------+--------------+\n");
    printf("  Unaccounted counts packets the NIC delivered that no classifier claimed.\n");
    printf("  A small positive value right after shutdown is normal (a burst can land\n");
    printf("  between the last software flush and this snapshot).\n");
}

static void helper_print_cmc_stats(const struct ports_config *ports_config,
                                   bool warmup_complete, unsigned loop_count,
                                   unsigned test_time)
{
    // Clear the screen
    if (!g_daemon_mode) {
        printf("\033[2J\033[H");
    } else {
        printf("\n========== [%s %u sec] ==========\n",
               warmup_complete ? "TEST" : "WARM-UP",
               warmup_complete ? test_time : loop_count);
    }

    // Header
    printf("╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    if (!warmup_complete) {
        printf("║                                                              CMC PORT STATS - WARM-UP (%3u/120 sec)                                                                                                                                  ║\n", loop_count);
    } else {
        printf("║                                                              CMC PORT STATS - TEST Duration: %5u sec                                                                                                                                 ║\n", test_time);
    }
    printf("╚══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝\n");

    // Fetch HW stats once (per port)
    struct rte_eth_stats port_hw_stats[MAX_PORTS];
    for (uint16_t i = 0; i < ports_config->nb_ports; i++) {
        uint16_t port_id = ports_config->ports[i].port_id;
        if (rte_eth_stats_get(port_id, &port_hw_stats[port_id]) != 0) {
            memset(&port_hw_stats[port_id], 0, sizeof(struct rte_eth_stats));
        }
    }

    // Table 1: DSM-A line (VLAN 97 → 225, SRC MAC tail 0x20)
    printf("\n  === DSM-A (VLAN 97 -> 225 | VL %u..%u -> %u..%u | SRC MAC tail 0x%02X) ===\n",
           CMC_TX_VL_ID_BASE, CMC_TX_VL_ID_BASE + CMC_TOTAL_VL_COUNT - 1,
           CMC_RX_VL_ID_BASE, CMC_RX_VL_ID_BASE + CMC_TOTAL_VL_COUNT - 1,
           CMC_NET_A_SRC_MAC_TAIL);
    print_cmc_table_group(dsma_cmc_indices, DSMA_COUNT, port_hw_stats);

    // Table 2: DSM-B line (VLAN 98 → 226, SRC MAC tail 0x40)
    printf("\n  === DSM-B (VLAN 98 -> 226 | VL %u..%u -> %u..%u | SRC MAC tail 0x%02X) ===\n",
           CMC_TX_VL_ID_BASE, CMC_TX_VL_ID_BASE + CMC_TOTAL_VL_COUNT - 1,
           CMC_RX_VL_ID_BASE, CMC_RX_VL_ID_BASE + CMC_TOTAL_VL_COUNT - 1,
           CMC_NET_B_SRC_MAC_TAIL);
    print_cmc_table_group(dsmb_cmc_indices, DSMB_COUNT, port_hw_stats);

    // Warnings
    bool has_warning = false;
    for (uint16_t cmc = 0; cmc < CMC_DPDK_PORT_COUNT; cmc++) {
        uint64_t bad = rte_atomic64_read(&cmc_stats[cmc].bad_pkts);
        uint64_t sm_fail = rte_atomic64_read(&cmc_stats[cmc].splitmix_fail);
        uint64_t crc_fail = rte_atomic64_read(&cmc_stats[cmc].crc32_fail);
        uint64_t xor_fail = rte_atomic64_read(&cmc_stats[cmc].xor_fail);
        uint64_t bit_err = rte_atomic64_read(&cmc_stats[cmc].bit_errors);
        uint64_t lost = rte_atomic64_read(&cmc_stats[cmc].lost_pkts);

        if (bad > 0 || bit_err > 0 || lost > 0) {
            if (!has_warning) {
                printf("\n  WARNINGS:\n");
                has_warning = true;
            }
            if (bad > 0)
                printf("      %s (CMC %u): %lu bad packets! (SM:%lu CRC:%lu XOR:%lu)\n",
                       cmc_port_labels[cmc], cmc, bad, sm_fail, crc_fail, xor_fail);
            if (bit_err > 0)
                printf("      %s (CMC %u): %lu bit errors!\n", cmc_port_labels[cmc], cmc, bit_err);
            if (lost > 0)
                printf("      %s (CMC %u): %lu lost packets!\n", cmc_port_labels[cmc], cmc, lost);
        }
    }

    printf("\n  Press Ctrl+C to stop\n");
}
#endif /* STATS_MODE_CMC */

// ==========================================
// SERVER PORT-BASED STATISTICS TABLE (Legacy table)
// ==========================================
static void helper_print_server_stats(const struct ports_config *ports_config,
                                      const uint64_t prev_tx_bytes[],
                                      const uint64_t prev_rx_bytes[],
                                      bool warmup_complete, unsigned loop_count,
                                      unsigned test_time)
{
    // Clear screen (only in interactive mode, disabled in daemon mode for log files)
    if (!g_daemon_mode) {
        printf("\033[2J\033[H");
    } else {
        // Daemon mode: separator line between tables
        printf("\n========== [%s %u sec] ==========\n",
               warmup_complete ? "TEST" : "WARM-UP",
               warmup_complete ? test_time : loop_count);
    }

    // Header (240 characters wide)
    printf("╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    if (!warmup_complete) {
        printf("║                                                                    WARM-UP PHASE (%3u/120 sec) - Statistics will be reset at 120 seconds                                                                                        ║\n", loop_count);
    } else {
        printf("║                                                                    TEST IN PROGRESS - Test Duration: %5u sec                                                                                                                   ║\n", test_time);
    }
    printf("╚══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝\n\n");

    // Main statistics table (240 characters)
    printf("┌──────┬─────────────────────────────────────────────────────────────────────┬─────────────────────────────────────────────────────────────────────┬───────────────────────────────────────────────────────────────────────────────────────────────────┐\n");
    printf("│ Port │                            TX (Transmitted)                          │                            RX (Received)                            │                                      PRBS Verification                                              │\n");
    printf("│      ├─────────────────────┬─────────────────────┬─────────────────────────┼─────────────────────┬─────────────────────┬─────────────────────────┼─────────────────────┬─────────────────────┬─────────────────────┬─────────────────────┬─────────────┤\n");
    printf("│      │       Packets       │        Bytes        │          Gbps           │       Packets       │        Bytes        │          Gbps           │        Good         │         Bad         │        Lost         │      Bit Error      │     BER     │\n");
    printf("├──────┼─────────────────────┼─────────────────────┼─────────────────────────┼─────────────────────┼─────────────────────┼─────────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────┤\n");

    struct rte_eth_stats st;

    for (uint16_t i = 0; i < ports_config->nb_ports; i++) {
        uint16_t port_id = ports_config->ports[i].port_id;

        if (rte_eth_stats_get(port_id, &st) != 0) {
            printf("│  %2u  │         N/A         │         N/A         │           N/A           │         N/A         │         N/A         │           N/A           │         N/A         │         N/A         │         N/A         │         N/A         │     N/A     │\n", port_id);
            continue;
        }

        // HW statistics
        uint64_t tx_pkts = st.opackets;
        uint64_t tx_bytes = st.obytes;
        uint64_t rx_pkts = st.ipackets;
        uint64_t rx_bytes = st.ibytes;

        // Per-second rate calculation
        uint64_t tx_bytes_delta = tx_bytes - prev_tx_bytes[port_id];
        uint64_t rx_bytes_delta = rx_bytes - prev_rx_bytes[port_id];
        double tx_gbps = to_gbps(tx_bytes_delta);
        double rx_gbps = to_gbps(rx_bytes_delta);

        // PRBS verification statistics
        uint64_t good = rte_atomic64_read(&rx_stats_per_port[port_id].good_pkts);
        uint64_t bad = rte_atomic64_read(&rx_stats_per_port[port_id].bad_pkts);
        uint64_t lost = rte_atomic64_read(&rx_stats_per_port[port_id].lost_pkts);
        uint64_t bit_errors_raw = rte_atomic64_read(&rx_stats_per_port[port_id].bit_errors);

        // Include lost packets in bit_errors (each lost packet = all bits erroneous)
#if IMIX_ENABLED
        uint64_t lost_bits = lost * (uint64_t)IMIX_AVG_PACKET_SIZE * 8;
#else
        uint64_t lost_bits = lost * (uint64_t)PACKET_SIZE * 8;
#endif
        uint64_t bit_errors = bit_errors_raw + lost_bits;

        // Bit Error Rate (BER) calculation (lost packet bits added to total)
        double ber = 0.0;
        uint64_t total_bits = rx_bytes * 8 + lost_bits;
        if (total_bits > 0) {
            ber = (double)bit_errors / (double)total_bits;
        }

        // Print table row
        printf("│  %2u  │ %19lu │ %19lu │ %23.4f │ %19lu │ %19lu │ %23.4f │ %19lu │ %19lu │ %19lu │ %19lu │ %11.2e │\n",
               port_id,
               tx_pkts, tx_bytes, tx_gbps,
               rx_pkts, rx_bytes, rx_gbps,
               good, bad, lost, bit_errors, ber);
    }

    printf("└──────┴─────────────────────┴─────────────────────┴─────────────────────────┴─────────────────────┴─────────────────────┴─────────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────┘\n");

    // Warnings
    bool has_warning = false;
    for (uint16_t i = 0; i < ports_config->nb_ports; i++) {
        uint16_t port_id = ports_config->ports[i].port_id;

        uint64_t bad_pkts = rte_atomic64_read(&rx_stats_per_port[port_id].bad_pkts);
        uint64_t bit_errors = rte_atomic64_read(&rx_stats_per_port[port_id].bit_errors);
        uint64_t lost_pkts = rte_atomic64_read(&rx_stats_per_port[port_id].lost_pkts);

        if (bad_pkts > 0 || bit_errors > 0 || lost_pkts > 0) {
            if (!has_warning) {
                printf("\n  WARNINGS:\n");
                has_warning = true;
            }
            if (bad_pkts > 0) {
                printf("      Port %u: %lu bad packets detected!\n", port_id, bad_pkts);
            }
            if (bit_errors > 0) {
                printf("      Port %u: %lu bit errors detected!\n", port_id, bit_errors);
            }
            if (lost_pkts > 0) {
                printf("      Port %u: %lu lost packets detected!\n", port_id, lost_pkts);
            }
        }

        // HW missed packets check
        struct rte_eth_stats st2;
        if (rte_eth_stats_get(port_id, &st2) == 0 && st2.imissed > 0) {
            if (!has_warning) {
                printf("\n  WARNINGS:\n");
                has_warning = true;
            }
            printf("      Port %u: %lu packets missed by hardware (imissed)!\n", port_id, st2.imissed);
        }
    }

    printf("\n  Press Ctrl+C to stop\n");
}

// ==========================================
// PUBLIC API: helper_print_stats
// ==========================================
// CMC mode (STATS_MODE_CMC=1): per-network 2-table CMC display (Net A / Net B).
// Both ATE mode and unit-test mode use the same layout — the only difference
// is verification semantics, which are captured in the same per-CMC counters
// (Good/Bad/Lost/Bit Error). The SplitMix64 / CRC32 / XOR Fail columns stay
// at zero in ATE mode by design (the pure-PRBS path doesn't touch them).
// Legacy (STATS_MODE_CMC=0): single port-aggregate table.

void helper_print_stats(const struct ports_config *ports_config,
                        const uint64_t prev_tx_bytes[], const uint64_t prev_rx_bytes[],
                        bool warmup_complete, unsigned loop_count, unsigned test_time)
{
#if STATS_MODE_CMC
    helper_print_cmc_stats(ports_config, warmup_complete, loop_count, test_time);
    (void)prev_tx_bytes;
    (void)prev_rx_bytes;
#else
    helper_print_server_stats(ports_config, prev_tx_bytes, prev_rx_bytes,
                              warmup_complete, loop_count, test_time);
#endif
}
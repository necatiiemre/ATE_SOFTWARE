// _GNU_SOURCE must precede any system header so open_memstream() is declared.
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "Helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <rte_ethdev.h>
#include <rte_atomic.h>

#include "Config.h"
#include "TxRxManager.h"  // for rx_stats_per_port
#include "DpdkExternalTx.h" // for External TX stats
#include "RawSocketPort.h"  // for reset_raw_socket_stats
#include "ShutdownSnapshot.h" // capture last-second DTN table for Ctrl+C dump
#if PTP_ENABLED
#include "PtpSlave.h"       // ptp_get_stats for the end-of-test totals
#endif
#if HEALTH_MONITOR_ENABLED
#include "HealthMonitor.h"  // get_health_monitor_stats for the totals
#endif

// Daemon mode flag - when true, ANSI escape codes are disabled
bool g_daemon_mode = false;

void helper_set_daemon_mode(bool enabled) {
    g_daemon_mode = enabled;
}

// Helper functions
static inline double to_gbps(uint64_t bytes) {
    return (bytes * 8.0) / 1e9;
}

static inline double to_mbps(uint64_t bytes) {
    return (bytes * 8.0) / 1e6;
}

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

    // Reset RX validation statistics (PRBS)
    init_rx_stats();

#if STATS_MODE_DTN
    init_dtn_stats();
#endif

    // Reset raw socket and global sequence tracking
    reset_raw_socket_stats();
}

#if STATS_MODE_DTN
// ==========================================
// DTN PORT-BASED STATISTICS TABLE
// ==========================================
// 34 rows: DTN Port 0-31 (DPDK) + DTN Port 32 (Port12) + DTN Port 33 (Port13)
// Columns: TX Pkts/Bytes/Mbps | RX Pkts/Bytes/Mbps | Good/Bad/Lost/BitErr/BER
//
// DTN TX (DTN→Server) = Server RX = HW q_ipackets[queue]
// DTN RX (Server→DTN) = Server TX = HW q_opackets[queue]
// PRBS = dtn_stats[dtn_port] (from RX worker)

// Per-queue prev bytes (for per-DTN-port delta calculation)
// [dtn_port][0=tx_bytes, 1=rx_bytes]
static uint64_t dtn_prev_tx_bytes[DTN_PORT_COUNT];
static uint64_t dtn_prev_rx_bytes[DTN_PORT_COUNT];

// Renders the DTN statistics table to `out`. All printf calls below are
// redirected to `out` by the macro so the exact same text can be both printed
// to the console/log and captured for the Ctrl+C snapshot.
#define printf(...) fprintf(out, __VA_ARGS__)
static void helper_render_dtn_stats(FILE *out,
                                    const struct ports_config *ports_config,
                                    unsigned test_time)
{
    // Clear the screen
    if (!g_daemon_mode) {
        printf("\033[2J\033[H");
    } else {
        printf("\n========== [TEST %u sec] ==========\n", test_time);
    }

    // Header
    printf("╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                                                              DTN PORT STATS - TEST Duration: %5u sec                                                                                                                         ║\n", test_time);
    printf("╚══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝\n\n");

    // Table header
    printf("┌──────┬─────────────────────────────────────────────────────────────────────┬─────────────────────────────────────────────────────────────────────┬───────────────────────────────────────────────────────────────────────────────────────────────────┐\n");
    printf("│ DTN  │                          DTN TX (DTN→Server)                        │                          DTN RX (Server→DTN)                        │                                      PRBS Verification                                               │\n");
    printf("│ Port ├─────────────────────┬─────────────────────┬─────────────────────────┼─────────────────────┬─────────────────────┬─────────────────────────┼─────────────────────┬─────────────────────┬─────────────────────┬─────────────────────┬─────────────┤\n");
    printf("│      │       Packets       │        Bytes        │          Mbps           │       Packets       │        Bytes        │          Mbps           │        Good         │         Bad         │        Lost         │      Bit Error      │     BER     │\n");
    printf("├──────┼─────────────────────┼─────────────────────┼─────────────────────────┼─────────────────────┼─────────────────────┼─────────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────┤\n");

    // Fetch HW stats once (per port)
    struct rte_eth_stats port_hw_stats[MAX_PORTS];
    for (uint16_t i = 0; i < ports_config->nb_ports; i++) {
        uint16_t port_id = ports_config->ports[i].port_id;
        if (rte_eth_stats_get(port_id, &port_hw_stats[port_id]) != 0) {
            memset(&port_hw_stats[port_id], 0, sizeof(struct rte_eth_stats));
        }
    }

    // Port 12 per-target stats: DTN 32 → DTN 0-7, 16-23 evenly distributed
    // Each target is evenly distributed across 4 DTN ports (target bytes / 4)
    struct raw_socket_port *port12 = &raw_ports[0];
    uint64_t port12_target_tx_bytes[MAX_RAW_TARGETS] = {0};
    uint64_t port12_target_tx_pkts[MAX_RAW_TARGETS] = {0};
    uint16_t port12_target_dest[MAX_RAW_TARGETS] = {0};
    for (uint16_t t = 0; t < port12->tx_target_count; t++) {
        pthread_spin_lock(&port12->tx_targets[t].stats.lock);
        port12_target_tx_bytes[t] = port12->tx_targets[t].stats.tx_bytes;
        port12_target_tx_pkts[t] = port12->tx_targets[t].stats.tx_packets;
        pthread_spin_unlock(&port12->tx_targets[t].stats.lock);
        port12_target_dest[t] = port12->tx_targets[t].config.dest_port;
    }

    // DTN Port 0-31 (DPDK ports)
    for (uint16_t dtn = 0; dtn < DTN_DPDK_PORT_COUNT; dtn++) {
        const struct dtn_port_map_entry *entry = &dtn_port_map[dtn];

        // DTN TX (DTN→Server) = Server RX = HW q_ipackets[queue] on tx_server_port
        uint16_t srv_rx_port = entry->tx_server_port;
        uint16_t srv_rx_queue = entry->tx_server_queue;
        uint64_t dtn_tx_pkts = port_hw_stats[srv_rx_port].q_ipackets[srv_rx_queue];
        uint64_t dtn_tx_bytes = port_hw_stats[srv_rx_port].q_ibytes[srv_rx_queue];

        // DTN RX (Server→DTN) = Server TX = HW q_opackets[queue] on rx_server_port
        uint16_t srv_tx_port = entry->rx_server_port;
        uint16_t srv_tx_queue = entry->rx_server_queue;
        uint64_t dtn_rx_pkts = port_hw_stats[srv_tx_port].q_opackets[srv_tx_queue];
        uint64_t dtn_rx_bytes = port_hw_stats[srv_tx_port].q_obytes[srv_tx_queue];

        // Port 12 contribution: DTN 32 → DTN 0-7, 16-23 (each target equally across 4 DTN ports)
        for (uint16_t t = 0; t < port12->tx_target_count; t++) {
            if (port12_target_dest[t] == entry->rx_server_port) {
                dtn_rx_bytes += port12_target_tx_bytes[t] / 4;
                dtn_rx_pkts += port12_target_tx_pkts[t] / 4;
                break;
            }
        }

        // Mbps delta calculation
        uint64_t tx_delta = dtn_tx_bytes - dtn_prev_tx_bytes[dtn];
        uint64_t rx_delta = dtn_rx_bytes - dtn_prev_rx_bytes[dtn];
        double tx_mbps = to_mbps(tx_delta);
        double rx_mbps = to_mbps(rx_delta);

        // Update prev values
        dtn_prev_tx_bytes[dtn] = dtn_tx_bytes;
        dtn_prev_rx_bytes[dtn] = dtn_rx_bytes;

        // PRBS statistics (from dtn_stats)
        uint64_t good = rte_atomic64_read(&dtn_stats[dtn].good_pkts);
        uint64_t bad = rte_atomic64_read(&dtn_stats[dtn].bad_pkts);
        uint64_t lost = rte_atomic64_read(&dtn_stats[dtn].lost_pkts);
        uint64_t bit_errors_raw = rte_atomic64_read(&dtn_stats[dtn].bit_errors);

        // Include lost packets in bit_errors (each lost packet = all bits erroneous)
#if IMIX_ENABLED
        uint64_t lost_bits = lost * (uint64_t)IMIX_AVG_PACKET_SIZE * 8;
#else
        uint64_t lost_bits = lost * (uint64_t)PACKET_SIZE * 8;
#endif
        uint64_t bit_errors = bit_errors_raw + lost_bits;

        // BER calculation (lost packet bits added to total)
        double ber = 0.0;
        uint64_t total_bits = dtn_tx_bytes * 8 + lost_bits;
        if (total_bits > 0) {
            ber = (double)bit_errors / (double)total_bits;
        }

        printf("│  %2u  │ %19lu │ %19lu │ %23.2f │ %19lu │ %19lu │ %23.2f │ %19lu │ %19lu │ %19lu │ %19lu │ %11.2e │\n",
               dtn,
               dtn_tx_pkts, dtn_tx_bytes, tx_mbps,
               dtn_rx_pkts, dtn_rx_bytes, rx_mbps,
               good, bad, lost, bit_errors, ber);
    }

    // DTN Port 32 (Port 12 - 1G raw socket)
    // DTN TX = DTN→Server = dpdk_ext_rx_stats (server receives from this port)
    // DTN RX = Server→DTN = raw socket TX aggregate (server sends through this port)
    {
        struct raw_socket_port *port12 = &raw_ports[0];
        // DTN TX: What server received from Port 12 (DPDK External TX RX stats)
        pthread_spin_lock(&port12->dpdk_ext_rx_stats.lock);
        uint64_t dtn32_tx_pkts = port12->dpdk_ext_rx_stats.rx_packets;
        uint64_t dtn32_tx_bytes = port12->dpdk_ext_rx_stats.rx_bytes;
        uint64_t dtn32_good = port12->dpdk_ext_rx_stats.good_pkts;
        uint64_t dtn32_bad = port12->dpdk_ext_rx_stats.bad_pkts;
        uint64_t dtn32_bit_err = port12->dpdk_ext_rx_stats.bit_errors;
        pthread_spin_unlock(&port12->dpdk_ext_rx_stats.lock);

        // DTN RX: What server sent through Port 12 (raw socket TX aggregate)
        uint64_t dtn32_rx_pkts = 0, dtn32_rx_bytes = 0;
        for (uint16_t t = 0; t < port12->tx_target_count; t++) {
            pthread_spin_lock(&port12->tx_targets[t].stats.lock);
            dtn32_rx_pkts += port12->tx_targets[t].stats.tx_packets;
            dtn32_rx_bytes += port12->tx_targets[t].stats.tx_bytes;
            pthread_spin_unlock(&port12->tx_targets[t].stats.lock);
        }

        uint64_t tx_delta = dtn32_tx_bytes - dtn_prev_tx_bytes[DTN_RAW_PORT_12];
        uint64_t rx_delta = dtn32_rx_bytes - dtn_prev_rx_bytes[DTN_RAW_PORT_12];
        dtn_prev_tx_bytes[DTN_RAW_PORT_12] = dtn32_tx_bytes;
        dtn_prev_rx_bytes[DTN_RAW_PORT_12] = dtn32_rx_bytes;
        double tx_mbps = to_mbps(tx_delta);
        double rx_mbps = to_mbps(rx_delta);

        uint64_t dtn32_lost = get_global_sequence_lost();

        // Include lost packets in bit_errors
#if IMIX_ENABLED
        uint64_t dtn32_lost_bits = dtn32_lost * (uint64_t)RAW_IMIX_AVG_PACKET_SIZE * 8;
#else
        uint64_t dtn32_lost_bits = dtn32_lost * (uint64_t)RAW_PKT_TOTAL_SIZE * 8;
#endif
        uint64_t dtn32_bit_err_eff = dtn32_bit_err + dtn32_lost_bits;

        double ber = 0.0;
        uint64_t total_bits = dtn32_tx_bytes * 8 + dtn32_lost_bits;
        if (total_bits > 0) ber = (double)dtn32_bit_err_eff / (double)total_bits;

        printf("│  32  │ %19lu │ %19lu │ %23.2f │ %19lu │ %19lu │ %23.2f │ %19lu │ %19lu │ %19lu │ %19lu │ %11.2e │\n",
               dtn32_tx_pkts, dtn32_tx_bytes, tx_mbps,
               dtn32_rx_pkts, dtn32_rx_bytes, rx_mbps,
               dtn32_good, dtn32_bad, dtn32_lost, dtn32_bit_err_eff, ber);
    }

    // DTN Port 33 (Port 13 - 100M raw socket)
    {
        struct raw_socket_port *port13 = &raw_ports[1];
        // DTN TX: What server received from Port 13 (DPDK External TX RX stats)
        pthread_spin_lock(&port13->dpdk_ext_rx_stats.lock);
        uint64_t dtn33_tx_pkts = port13->dpdk_ext_rx_stats.rx_packets;
        uint64_t dtn33_tx_bytes = port13->dpdk_ext_rx_stats.rx_bytes;
        uint64_t dtn33_good = port13->dpdk_ext_rx_stats.good_pkts;
        uint64_t dtn33_bad = port13->dpdk_ext_rx_stats.bad_pkts;
        uint64_t dtn33_bit_err = port13->dpdk_ext_rx_stats.bit_errors;
        pthread_spin_unlock(&port13->dpdk_ext_rx_stats.lock);

        // DTN RX: What server sent through Port 13
        uint64_t dtn33_rx_pkts = 0, dtn33_rx_bytes = 0;
        for (uint16_t t = 0; t < port13->tx_target_count; t++) {
            pthread_spin_lock(&port13->tx_targets[t].stats.lock);
            dtn33_rx_pkts += port13->tx_targets[t].stats.tx_packets;
            dtn33_rx_bytes += port13->tx_targets[t].stats.tx_bytes;
            pthread_spin_unlock(&port13->tx_targets[t].stats.lock);
        }

        uint64_t tx_delta = dtn33_tx_bytes - dtn_prev_tx_bytes[DTN_RAW_PORT_13];
        uint64_t rx_delta = dtn33_rx_bytes - dtn_prev_rx_bytes[DTN_RAW_PORT_13];
        dtn_prev_tx_bytes[DTN_RAW_PORT_13] = dtn33_tx_bytes;
        dtn_prev_rx_bytes[DTN_RAW_PORT_13] = dtn33_rx_bytes;
        double tx_mbps = to_mbps(tx_delta);
        double rx_mbps = to_mbps(rx_delta);

        uint64_t dtn33_lost = get_global_sequence_lost_p13();

        // Include lost packets in bit_errors
#if IMIX_ENABLED
        uint64_t dtn33_lost_bits = dtn33_lost * (uint64_t)RAW_IMIX_AVG_PACKET_SIZE * 8;
#else
        uint64_t dtn33_lost_bits = dtn33_lost * (uint64_t)RAW_PKT_TOTAL_SIZE * 8;
#endif
        uint64_t dtn33_bit_err_eff = dtn33_bit_err + dtn33_lost_bits;

        double ber = 0.0;
        uint64_t total_bits = dtn33_tx_bytes * 8 + dtn33_lost_bits;
        if (total_bits > 0) ber = (double)dtn33_bit_err_eff / (double)total_bits;

        printf("│  33  │ %19lu │ %19lu │ %23.2f │ %19lu │ %19lu │ %23.2f │ %19lu │ %19lu │ %19lu │ %19lu │ %11.2e │\n",
               dtn33_tx_pkts, dtn33_tx_bytes, tx_mbps,
               dtn33_rx_pkts, dtn33_rx_bytes, rx_mbps,
               dtn33_good, dtn33_bad, dtn33_lost, dtn33_bit_err_eff, ber);
    }

    printf("└──────┴─────────────────────┴─────────────────────┴─────────────────────────┴─────────────────────┴─────────────────────┴─────────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────┘\n");

    // DTN warnings
    bool has_warning = false;
    for (uint16_t dtn = 0; dtn < DTN_DPDK_PORT_COUNT; dtn++) {
        uint64_t bad = rte_atomic64_read(&dtn_stats[dtn].bad_pkts);
        uint64_t bit_err = rte_atomic64_read(&dtn_stats[dtn].bit_errors);
        uint64_t lost = rte_atomic64_read(&dtn_stats[dtn].lost_pkts);

        if (bad > 0 || bit_err > 0 || lost > 0) {
            if (!has_warning) {
                printf("\n  WARNINGS:\n");
                has_warning = true;
            }
            if (bad > 0)
                printf("      DTN Port %u: %lu bad packets!\n", dtn, bad);
            if (bit_err > 0)
                printf("      DTN Port %u: %lu bit errors!\n", dtn, bit_err);
            if (lost > 0)
                printf("      DTN Port %u: %lu lost packets!\n", dtn, lost);
        }
    }

    printf("\n  Press Ctrl+C to stop\n");
}
#undef printf

// Wrapper: render the DTN table once into a memory buffer, then (a) print it to
// stdout exactly as before and (b) hand the captured text to ShutdownSnapshot
// so the last full second before a Ctrl+C can be dumped to a file.
static void helper_print_dtn_stats(const struct ports_config *ports_config,
                                   unsigned test_time)
{
    char *buf = NULL;
    size_t buf_size = 0;
    FILE *ms = open_memstream(&buf, &buf_size);
    if (ms == NULL) {
        // Fallback: no capture, render straight to stdout (original behavior).
        helper_render_dtn_stats(stdout, ports_config, test_time);
        return;
    }

    helper_render_dtn_stats(ms, ports_config, test_time);
    fclose(ms);  // flushes and finalizes `buf`

    if (buf != NULL) {
        fputs(buf, stdout);
        shutdown_snapshot_store(SNAP_SLOT_DTN, buf);
        free(buf);
    }
}
#endif /* STATS_MODE_DTN */

// ==========================================
// SERVER PORT-BASED STATISTICS TABLE (Legacy table)
// ==========================================
static void helper_print_server_stats(const struct ports_config *ports_config,
                                      const uint64_t prev_tx_bytes[],
                                      const uint64_t prev_rx_bytes[],
                                      unsigned test_time)
{
    // Clear screen (only in interactive mode, disabled in daemon mode for log files)
    if (!g_daemon_mode) {
        printf("\033[2J\033[H");
    } else {
        // Daemon mode: separator line between tables
        printf("\n========== [TEST %u sec] ==========\n", test_time);
    }

    // Header (240 characters wide)
    printf("╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                                                                    TEST IN PROGRESS - Test Duration: %5u sec                                                                                                                   ║\n", test_time);
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
        printf("│  %2u  │ %19lu │ %19lu │ %23.2f │ %19lu │ %19lu │ %23.2f │ %19lu │ %19lu │ %19lu │ %19lu │ %11.2e │\n",
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
// END-OF-TEST TOTALS (printed once)
// ==========================================
// Rendered a single time after the RX drain window closes. Three blocks:
//   1. PRBS totals   - the numbers the DTN table shows, aggregated.
//   2. Purity check  - proof that no PTP / Health Monitor / other foreign
//                      traffic reached the PRBS counters. All zeros = the
//                      totals above are pure PRBS.
//   3. PTP / Health Monitor totals, which are deliberately NOT part of the
//      PRBS numbers and are reported separately here.
// The text is also stored in the shutdown snapshot so it lands in the summary
// log, so every printf below goes through `out`.
#define printf(...) fprintf(out, __VA_ARGS__)
static void helper_render_final_totals(FILE *out,
                                       const struct ports_config *ports_config,
                                       unsigned test_time)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                    END OF TEST - TOTALS (%6u sec)                         ║\n", test_time);
    printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");

#if STATS_MODE_DTN
    // ---------- 1. PRBS totals ----------
    struct rte_eth_stats hw[MAX_PORTS];
    for (uint16_t i = 0; i < ports_config->nb_ports; i++) {
        uint16_t port_id = ports_config->ports[i].port_id;
        if (rte_eth_stats_get(port_id, &hw[port_id]) != 0) {
            memset(&hw[port_id], 0, sizeof(hw[port_id]));
        }
    }

    uint64_t tot_tx_pkts = 0, tot_tx_bytes = 0;
    uint64_t tot_rx_pkts = 0, tot_rx_bytes = 0;
    uint64_t tot_good = 0, tot_bad = 0, tot_lost = 0, tot_bit_err = 0;
    uint64_t tot_short = 0, tot_other = 0;

    struct raw_socket_port *p12 = &raw_ports[0];
    struct raw_socket_port *p13 = &raw_ports[1];

    for (uint16_t dtn = 0; dtn < DTN_DPDK_PORT_COUNT; dtn++) {
        const struct dtn_port_map_entry *e = &dtn_port_map[dtn];
        tot_tx_pkts  += hw[e->tx_server_port].q_ipackets[e->tx_server_queue];
        tot_tx_bytes += hw[e->tx_server_port].q_ibytes[e->tx_server_queue];
        tot_rx_pkts  += hw[e->rx_server_port].q_opackets[e->rx_server_queue];
        tot_rx_bytes += hw[e->rx_server_port].q_obytes[e->rx_server_queue];
    }
    // Port 12/13 rows: same sources the per-second table uses. Their PRBS
    // quality counters live in dpdk_ext_rx_stats and the global sequence
    // trackers, NOT in dtn_stats[32]/[33] - those two entries are never
    // written, since rx_worker only ever indexes dtn_stats by a DPDK port's
    // RX VLAN (0-31). Reading them here would add the raw ports' packets to
    // the TX total while dropping their Good/Lost entirely.
    struct raw_socket_port *raws[2] = { p12, p13 };
    for (int r = 0; r < 2; r++) {
        struct raw_socket_port *rp = raws[r];
        pthread_spin_lock(&rp->dpdk_ext_rx_stats.lock);
        tot_tx_pkts  += rp->dpdk_ext_rx_stats.rx_packets;
        tot_tx_bytes += rp->dpdk_ext_rx_stats.rx_bytes;
        tot_good     += rp->dpdk_ext_rx_stats.good_pkts;
        tot_bad      += rp->dpdk_ext_rx_stats.bad_pkts;
        tot_bit_err  += rp->dpdk_ext_rx_stats.bit_errors;
        pthread_spin_unlock(&rp->dpdk_ext_rx_stats.lock);
        for (uint16_t t = 0; t < rp->tx_target_count; t++) {
            pthread_spin_lock(&rp->tx_targets[t].stats.lock);
            tot_rx_pkts  += rp->tx_targets[t].stats.tx_packets;
            tot_rx_bytes += rp->tx_targets[t].stats.tx_bytes;
            pthread_spin_unlock(&rp->tx_targets[t].stats.lock);
        }
    }
    tot_lost += get_global_sequence_lost();
    tot_lost += get_global_sequence_lost_p13();

    // DPDK rows only: dtn_stats[32]/[33] are never populated (see above).
    for (uint16_t dtn = 0; dtn < DTN_DPDK_PORT_COUNT; dtn++) {
        tot_good    += (uint64_t)rte_atomic64_read(&dtn_stats[dtn].good_pkts);
        tot_bad     += (uint64_t)rte_atomic64_read(&dtn_stats[dtn].bad_pkts);
        tot_lost    += (uint64_t)rte_atomic64_read(&dtn_stats[dtn].lost_pkts);
        tot_bit_err += (uint64_t)rte_atomic64_read(&dtn_stats[dtn].bit_errors);
        tot_short   += (uint64_t)rte_atomic64_read(&dtn_stats[dtn].short_pkts);
        tot_other   += (uint64_t)rte_atomic64_read(&dtn_stats[dtn].other_pkts);
    }

    double ber = 0.0;
    uint64_t total_bits = tot_tx_bytes * 8;
    if (total_bits > 0) ber = (double)tot_bit_err / (double)total_bits;

    printf("\n--- PRBS Totals (all 34 DTN ports) ---\n");
    printf("  DTN TX  (DTN->Server) : %20lu pkts  %20lu bytes\n", tot_tx_pkts, tot_tx_bytes);
    printf("  DTN RX  (Server->DTN) : %20lu pkts  %20lu bytes\n", tot_rx_pkts, tot_rx_bytes);
    printf("  PRBS Good             : %20lu\n", tot_good);
    printf("  PRBS Bad              : %20lu\n", tot_bad);
    printf("  Lost                  : %20lu\n", tot_lost);
    printf("  Bit Errors            : %20lu\n", tot_bit_err);
    printf("  BER                   : %20.2e\n", ber);

    // ---------- 2. Purity check ----------
    uint64_t hm12 = 0, fo12 = 0, hm13 = 0, fo13 = 0;
    raw_socket_get_excluded_counts(0, &hm12, &fo12);
    raw_socket_get_excluded_counts(1, &hm13, &fo13);

    bool pure = (tot_other == 0 && fo12 == 0 && fo13 == 0);

    printf("\n--- Purity Check (non-PRBS traffic kept OUT of the numbers above) ---\n");
    printf("  Foreign frames on PRBS queues (DPDK 0-31) : %lu\n", tot_other);
    printf("  Health Monitor frames excluded  (Port 13) : %lu\n", hm13);
    printf("  Health Monitor frames excluded  (Port 12) : %lu\n", hm12);
    printf("  Other foreign frames excluded   (Port 12) : %lu\n", fo12);
    printf("  Other foreign frames excluded   (Port 13) : %lu\n", fo13);
    printf("  Undersized frames dropped                 : %lu\n", tot_short);

    // Name the foreign traffic rather than just counting it: "328 foreign
    // frames" does not say whether PTP escaped its queue or the switch is
    // simply flooding LLDP at us.
    {
        uint16_t ftypes[8];
        uint64_t fcounts[8];
        int fn = txrx_get_foreign_ethertypes(ftypes, fcounts,
                                             (int)(sizeof(ftypes) / sizeof(ftypes[0])));
        if (fn > 0) {
            printf("  Foreign traffic by EtherType:\n");
            for (int i = 0; i < fn; i++) {
                const char *name;
                switch (ftypes[i]) {
                    case 0x88F7: name = "PTP (escaped queue 5!)"; break;
                    case 0x88CC: name = "LLDP (switch)";          break;
                    case 0x0806: name = "ARP";                    break;
                    case 0x8809: name = "LACP/slow protocols";    break;
                    case 0x86DD: name = "IPv6";                   break;
                    case 0x8892: name = "PROFINET";               break;
                    default:     name = (ftypes[i] < 0x0600) ? "802.3 LLC (STP/BPDU)"
                                                             : "unknown";     break;
                }
                printf("      0x%04X %-24s : %lu\n", ftypes[i], name, fcounts[i]);
            }
        }
    }
    printf("  => PRBS totals are %s\n",
           pure ? "PURE (no foreign frame reached a PRBS counter)"
                : "SUSPECT - foreign frames reached a PRBS queue, see above");
    if (!pure) {
        printf("     Check the log for \"should be Q5\" (PTP rte_flow rule failed)\n");
    }
#else
    (void)ports_config;
#endif /* STATS_MODE_DTN */

    // ---------- 3. PTP totals (not part of the PRBS numbers) ----------
#if PTP_ENABLED
    {
        ptp_session_stats_t ps[PTP_MAX_SESSIONS];
        uint8_t ps_count = 0;
        memset(ps, 0, sizeof(ps));
        ptp_get_stats(ps, &ps_count);

        uint64_t sync_rx = 0, dreq_tx = 0, dresp_rx = 0;
        for (uint8_t i = 0; i < ps_count; i++) {
            sync_rx  += ps[i].sync_rx_count;
            dreq_tx  += ps[i].delay_req_tx_count;
            dresp_rx += ps[i].delay_resp_rx_count;
        }

        printf("\n--- PTP Totals (separate from PRBS) ---\n");
        printf("  Sessions              : %20u\n", (unsigned)ps_count);
        printf("  Sync received         : %20lu\n", sync_rx);
        printf("  Delay_Req sent        : %20lu\n", dreq_tx);
        printf("  Delay_Resp received   : %20lu\n", dresp_rx);
        printf("  PTP packets total     : %20lu\n", sync_rx + dreq_tx + dresp_rx);
    }
#endif

    // ---------- 4. Health Monitor totals (not part of the PRBS numbers) ----------
#if HEALTH_MONITOR_ENABLED
    {
        struct health_monitor_stats hs;
        memset(&hs, 0, sizeof(hs));
        get_health_monitor_stats(&hs);

        printf("\n--- Health Monitor Totals (separate from PRBS) ---\n");
        printf("  Queries sent          : %20lu\n", hs.queries_sent);
        printf("  Responses received    : %20lu\n", hs.responses_received);
        printf("  Incomplete cycles     : %20lu\n", hs.timeouts);
        printf("  HM packets total      : %20lu\n",
               hs.queries_sent + hs.responses_received);
    }
#endif

    printf("\n");
}
#undef printf

void helper_print_final_totals(const struct ports_config *ports_config,
                               unsigned test_time)
{
    char *buf = NULL;
    size_t buf_size = 0;
    FILE *ms = open_memstream(&buf, &buf_size);
    if (ms == NULL) {
        helper_render_final_totals(stdout, ports_config, test_time);
        return;
    }

    helper_render_final_totals(ms, ports_config, test_time);
    fclose(ms);

    if (buf != NULL) {
        fputs(buf, stdout);
        shutdown_snapshot_store(SNAP_SLOT_TOTALS, buf);
        free(buf);
    }
}

// ==========================================
// PUBLIC API: helper_print_stats
// ==========================================
// Draws DTN or Server table based on STATS_MODE_DTN flag

void helper_print_stats(const struct ports_config *ports_config,
                        const uint64_t prev_tx_bytes[], const uint64_t prev_rx_bytes[],
                        unsigned test_time)
{
#if STATS_MODE_DTN
    helper_print_dtn_stats(ports_config, test_time);
    (void)prev_tx_bytes;
    (void)prev_rx_bytes;
#else
    helper_print_server_stats(ports_config, prev_tx_bytes, prev_rx_bytes,
                              test_time);
#endif
}
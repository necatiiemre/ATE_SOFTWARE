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
#include <rte_cycles.h>  // rte_delay_ms
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

#if STATS_MODE_DTN
// Rate baselines for the DTN table: the byte counters the previous render saw,
// used to turn cumulative counters into a per-second rate. Declared here
// because helper_reset_stats() has to clear them alongside the HW counters
// they are measured against.
static uint64_t dtn_prev_tx_bytes[DTN_PORT_COUNT];
static uint64_t dtn_prev_rx_bytes[DTN_PORT_COUNT];
#endif

void helper_reset_stats(const struct ports_config *ports_config,
                        uint64_t prev_tx_bytes[], uint64_t prev_rx_bytes[])
{
    // Order matters here, and it is the whole point of this function.
    //
    // Two counters have to start from the same instant: what the server has
    // SENT (the HW queue counters) and what came back VALIDATED (dtn_stats).
    // They cannot literally be zeroed together, so whatever sits between them
    // is an error: every packet transmitted in that window is counted as sent
    // while its validation is wiped a moment later, and the totals report it
    // as sent-but-never-returned.
    //
    // This used to zero the HW counters first and everything else after,
    // including telling the workers to drop their locals - about 1.4 ms of
    // work, which at ~2 M packets/s is some 3000 packets. That is exactly the
    // fixed offset the end-of-test totals kept reporting: constant per run,
    // indifferent to run length, in the direction of "sent more than came
    // back".
    //
    // So: everything slow happens first, and the two headline counters are
    // zeroed back to back at the end with nothing in between. The residual is
    // then one flight time's worth of packets - the packets in the air when
    // the two counters are zeroed - which is irreducible and about fifty times
    // smaller.

    // 1. Tell the RX workers to drop what they are holding, and let them
    //    notice. They check once per burst, so this is orders of magnitude
    //    more time than they need; doing it here rather than last means their
    //    locals are already empty before either headline counter is touched.
    txrx_reset_worker_locals();
    rte_delay_ms(2);

    // 2. Everything that is not one of the two headline counters. Slow, and
    //    deliberately out of the way before the pair below.
    reset_rx_stats_counters();

#if STATS_MODE_DTN
    // The DTN table's own rate baselines. rte_eth_stats_reset() below zeroes
    // the HW byte counters the TX rate is measured from, so leaving these at
    // their pre-reset values would make the next render compute a delta
    // against a counter that has gone backwards.
    memset(dtn_prev_tx_bytes, 0, sizeof(dtn_prev_tx_bytes));
    memset(dtn_prev_rx_bytes, 0, sizeof(dtn_prev_rx_bytes));
#endif

    // 3. The headline counters. Both directions of the DTN table now live in
    //    dtn_stats - sent in prbs_tx_pkts, validated in good/bad - so a single
    //    init_dtn_stats() zeroes the pair atomically with respect to each
    //    other and the ordering problem this step was written for is gone for
    //    the packet columns. rte_eth_stats_reset() still follows because the
    //    Mbps columns read the HW byte counters, and it is kept adjacent so
    //    the rates restart from the same instant as the counts.
    // Adjacent on purpose. reset_raw_socket_stats() zeroes the raw ports' send
    // counters and init_dtn_stats() zeroes the arrivals attributed to them, and
    // anything sent between the two is counted on one side only. It used to sit
    // with the slow resets above, leaving a window that showed up as raw
    // packets sent and never returned.
    reset_raw_socket_stats();
#if STATS_MODE_DTN
    init_dtn_stats();
#endif
    for (uint16_t i = 0; i < ports_config->nb_ports; i++) {
        rte_eth_stats_reset(ports_config->ports[i].port_id);
    }

    // 4. Rate baselines for the server table, now that the counters they
    //    track are zero.
    for (uint16_t i = 0; i < ports_config->nb_ports; i++) {
        uint16_t port_id = ports_config->ports[i].port_id;
        prev_tx_bytes[port_id] = 0;
        prev_rx_bytes[port_id] = 0;
    }
}

#if STATS_MODE_DTN
// ==========================================
// DTN PORT-BASED STATISTICS TABLE
// ==========================================
// 34 rows: DTN Port 0-31 (DPDK) + DTN Port 32 (Port12) + DTN Port 33 (Port13)
// Columns: TX Pkts/Bytes/Mbps | RX Pkts/Bytes/Mbps | Good/Bad/Lost/BitErr/BER
//
// Both packet/byte columns are PRBS software counters from dtn_stats[dtn_port]:
//   DTN TX (DTN→Server) = good + bad + raw_origin  (RX worker) - everything
//       that came out of this DTN port and validated, whether the paired TX
//       worker sent it or it entered the device at Port 12/13
//   DTN RX (Server→DTN) = prbs_tx_pkts + ext_tx_pkts  (TX worker on queues
//       0-3, external TX worker on queue 4) - everything the server sent
//       toward this DTN port, by either path
// The HW queue counters are still read, but only for the Mbps columns: a rate
// needs a value that is exact at every instant, and the software counters are
// folded in on a flush cadence that does not line up with this render.

// Renders the DTN statistics table to `out`. All printf calls below are
// redirected to `out` by the macro so the exact same text can be both printed
// to the console/log and captured for the Ctrl+C snapshot.
#define printf(...) fprintf(out, __VA_ARGS__)
static void helper_render_dtn_stats(FILE *out,
                                    const struct ports_config *ports_config,
                                    unsigned test_time)
{
    // Kept in the signature so the two callers and the snapshot path stay
    // identical; the table itself no longer reads anything from it.
    (void)ports_config;

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

    // No HW queue counters are read here any more. Every column - packets,
    // bytes and both rates - comes from the software counters, which is what
    // lets a rate be checked against the total printed beside it.

    // DTN Port 0-31 (DPDK ports)
    for (uint16_t dtn = 0; dtn < DTN_DPDK_PORT_COUNT; dtn++) {
        // DTN TX (DTN→Server) = every PRBS packet that came out of this DTN
        // port and was validated here. Taken from the PRBS counters, not from
        // the HW queue counter: the NIC also steers non-PRBS traffic onto
        // queues 0-3 (roughly one small frame per port per second, counted as
        // `short` below), and q_ipackets would report those as test traffic.
        //
        // Both streams, because both really came out of this port. The
        // loopback traffic the paired TX worker sent, and the traffic that
        // entered the device at Port 12/13 and left through here - the DPDK
        // and raw socket pipelines run at the same time and this column says
        // what the port carried. The device's own counter for this port
        // includes both as well, so the two are directly comparable.
        //
        // They are still counted apart (raw_origin_*), because a row's TX
        // against its pair's RX only balances for the loopback stream alone.
        // That comparison moved to the end-of-test totals, which subtract
        // raw_origin_* to make it.
        const uint64_t dtn_raw_good =
            (uint64_t)rte_atomic64_read(&dtn_stats[dtn].raw_origin_good);
        const uint64_t dtn_raw_bad =
            (uint64_t)rte_atomic64_read(&dtn_stats[dtn].raw_origin_bad);
        uint64_t dtn_tx_pkts =
            (uint64_t)rte_atomic64_read(&dtn_stats[dtn].good_pkts) +
            (uint64_t)rte_atomic64_read(&dtn_stats[dtn].bad_pkts) +
            dtn_raw_good + dtn_raw_bad;
        uint64_t dtn_tx_bytes =
            (uint64_t)rte_atomic64_read(&dtn_stats[dtn].prbs_rx_bytes) +
            (uint64_t)rte_atomic64_read(&dtn_stats[dtn].raw_origin_bytes);

        // DTN RX (Server→DTN) = what the server sent toward this DTN port.
        // Taken from the TX worker's own PRBS counters, NOT from the HW
        // q_opackets counter for that queue.
        //
        // Both columns of this row are now the same kind of measurement:
        // software counters over exactly the packets that carry a PRBS
        // payload, cleared by the same init_dtn_stats() call. The HW counter
        // it replaces was neither - it counted every frame the stack put on
        // the queue, and it was zeroed by rte_eth_stats_reset() at a different
        // instant from the TX column's reset. That mismatch, not packet loss,
        // is what made a row show more sent than came back while the
        // watermark check reported no gaps.
        // Both send paths toward this DTN port: the paired TX worker on queues
        // 0-3, and the external TX worker on queue 4 carrying this port's VLAN.
        // The second was missing, and it showed: a port hands part of its send
        // budget to queue 4, that share appeared in no row, and every affected
        // row's RX came out below its TX by exactly it - 877 against 927 on
        // the rows behind Port 12, 925 against 935 behind Port 13, and equal
        // on the two ports that run no external TX at all.
        uint64_t dtn_rx_pkts =
            (uint64_t)rte_atomic64_read(&dtn_stats[dtn].prbs_tx_pkts) +
            (uint64_t)rte_atomic64_read(&dtn_stats[dtn].ext_tx_pkts);
        uint64_t dtn_rx_bytes =
            (uint64_t)rte_atomic64_read(&dtn_stats[dtn].prbs_tx_bytes) +
            (uint64_t)rte_atomic64_read(&dtn_stats[dtn].ext_tx_bytes);

        // Mbps delta calculation - from the byte totals printed beside them.
        //
        // These read the HW queue counters until recently, and that could not
        // survive the send column covering both queues. q_obytes is per queue:
        // queues 0-3 carry the loopback stream and queue 4 the external TX,
        // so no single queue counter measures what the RX column now reports,
        // and there is no honest way to take queue 4's share of it for one row
        // - it is shared by the four rows behind that port, and dividing it by
        // four is the estimate this file has already had to remove once.
        //
        // The reason the rate was read from hardware in the first place was
        // that the software counters are folded in on a flush cadence. That no
        // longer holds: the main loop requests a flush before every render and
        // waits for it, and every worker that feeds these counters answers -
        // the DPDK TX and RX workers, the raw socket TX and RX workers and the
        // external TX workers. So the counters are current when they are read.
        //
        // Taking the rate from the column beside it also makes this class of
        // mistake impossible rather than merely fixed: the rate is the delta
        // of the number printed next to it, so the two cannot describe
        // different traffic again.
        uint64_t tx_delta = dtn_tx_bytes - dtn_prev_tx_bytes[dtn];
        uint64_t rx_delta = dtn_rx_bytes - dtn_prev_rx_bytes[dtn];
        double tx_mbps = to_mbps(tx_delta);
        double rx_mbps = to_mbps(rx_delta);

        // Update prev values
        dtn_prev_tx_bytes[dtn] = dtn_tx_bytes;
        dtn_prev_rx_bytes[dtn] = dtn_rx_bytes;

        // PRBS statistics. Same set as the TX column above - both streams -
        // so Good + Bad still adds up to the packet count beside it and the
        // BER covers everything this port actually carried.
        uint64_t good = (uint64_t)rte_atomic64_read(&dtn_stats[dtn].good_pkts) +
                        dtn_raw_good;
        uint64_t bad  = (uint64_t)rte_atomic64_read(&dtn_stats[dtn].bad_pkts) +
                        dtn_raw_bad;
        uint64_t lost = rte_atomic64_read(&dtn_stats[dtn].lost_pkts);
        uint64_t bit_errors_raw =
            (uint64_t)rte_atomic64_read(&dtn_stats[dtn].bit_errors) +
            (uint64_t)rte_atomic64_read(&dtn_stats[dtn].raw_origin_bits);

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
    // Everything the NIC delivered to the PRBS queues, PRBS or not. Compared
    // against the PRBS figure below so the gap is visible rather than implied.
    uint64_t tot_hw_q_pkts = 0;
    // The sent side is two independent pipelines that happen to cross, and an
    // aggregate difference cannot say which of them is short. Kept apart:
    //   loop leg  - DPDK queues 0-3 and the raw ports' own TX, both of which
    //               come back and are validated on the DPDK queues
    //   ext leg   - the external-TX queue, which comes back at Ports 12/13
    uint64_t sent_loop = 0, sent_ext = 0;
    // Validated packets from the DPDK rows alone. The HW queue counter above
    // covers those rows only, so comparing it against the grand total - which
    // also carries the raw ports - was comparing different sets and produced
    // more "validated" packets than the hardware ever delivered.
    uint64_t tot_dpdk_validated = 0;
    uint64_t tot_raw_validated = 0;
    // Validated PRBS packets that reached a DPDK queue from a raw socket port
    // rather than from that row's paired TX worker. Kept out of the DTN rows'
    // TX columns so each row measures one stream, and counted here so the
    // arrival breakdown below still adds up.
    uint64_t tot_raw_origin = 0;

    struct raw_socket_port *p12 = &raw_ports[0];
    struct raw_socket_port *p13 = &raw_ports[1];

    for (uint16_t dtn = 0; dtn < DTN_DPDK_PORT_COUNT; dtn++) {
        const struct dtn_port_map_entry *e = &dtn_port_map[dtn];
        // TX side: every PRBS packet validated on this row's queue. The DTN
        // table splits these - the row's own stream in good/bad, traffic that
        // entered from Port 12/13 in raw_origin_* - so that a row compares
        // like with like. The totals put them back together: sent_loop below
        // counts the raw ports' transmissions too, so leaving their arrivals
        // out here would invent a 1.3 M packet shortfall.
        const uint64_t dtn_raw_origin =
            (uint64_t)rte_atomic64_read(&dtn_stats[dtn].raw_origin_good) +
            (uint64_t)rte_atomic64_read(&dtn_stats[dtn].raw_origin_bad);
        const uint64_t dtn_validated =
            (uint64_t)rte_atomic64_read(&dtn_stats[dtn].good_pkts) +
            (uint64_t)rte_atomic64_read(&dtn_stats[dtn].bad_pkts) +
            dtn_raw_origin;
        tot_tx_pkts       += dtn_validated;
        tot_dpdk_validated += dtn_validated;
        tot_tx_bytes += (uint64_t)rte_atomic64_read(&dtn_stats[dtn].prbs_rx_bytes) +
                        (uint64_t)rte_atomic64_read(&dtn_stats[dtn].raw_origin_bytes);
        tot_raw_origin += dtn_raw_origin;
        tot_hw_q_pkts += hw[e->tx_server_port].q_ipackets[e->tx_server_queue];
        // RX side: what the TX worker on this queue actually put on the wire,
        // from its own PRBS counters rather than the HW queue counter. Queues
        // 0-3 were already meant to carry nothing but PRBS, but "meant to" was
        // the assumption this reconciliation exists to test - and the HW
        // counter is zeroed by rte_eth_stats_reset() at a different instant
        // from the validated side it is compared against. Reading both sides
        // from counters cleared by the same init_dtn_stats() call removes both
        // sources of skew at once.
        tot_rx_pkts  += (uint64_t)rte_atomic64_read(&dtn_stats[dtn].prbs_tx_pkts);
        tot_rx_bytes += (uint64_t)rte_atomic64_read(&dtn_stats[dtn].prbs_tx_bytes);
        sent_loop    += (uint64_t)rte_atomic64_read(&dtn_stats[dtn].prbs_tx_pkts);
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
        tot_tx_pkts       += rp->dpdk_ext_rx_stats.rx_packets;
        tot_raw_validated += rp->dpdk_ext_rx_stats.rx_packets;
        tot_tx_bytes += rp->dpdk_ext_rx_stats.rx_bytes;
        tot_good     += rp->dpdk_ext_rx_stats.good_pkts;
        tot_bad      += rp->dpdk_ext_rx_stats.bad_pkts;
        tot_bit_err  += rp->dpdk_ext_rx_stats.bit_errors;
        pthread_spin_unlock(&rp->dpdk_ext_rx_stats.lock);
        for (uint16_t t = 0; t < rp->tx_target_count; t++) {
            pthread_spin_lock(&rp->tx_targets[t].stats.lock);
            tot_rx_pkts  += rp->tx_targets[t].stats.tx_packets;
            tot_rx_bytes += rp->tx_targets[t].stats.tx_bytes;
            sent_loop    += rp->tx_targets[t].stats.tx_packets;
            pthread_spin_unlock(&rp->tx_targets[t].stats.lock);
        }
    }
    tot_lost += get_global_sequence_lost();
    tot_lost += get_global_sequence_lost_p13();

#if DPDK_EXT_TX_ENABLED
    // External TX closes the books. The DTN 32/33 rows count what came back
    // to Ports 12/13 on the TX side, but those packets left the server from
    // the DPDK ports' external-TX queue, which none of the sums above touch:
    // the DPDK rows only read queues 0-3. Left out, the server appears to have
    // received more than it ever sent. Queue 4 carries external TX alone, so
    // its HW counter measures exactly the missing leg - and being a HW
    // counter, rte_eth_stats_reset() clears it with everything else.
    {
        static const struct dpdk_ext_tx_port_config ext_cfgs[] =
            DPDK_EXT_TX_PORTS_CONFIG_INIT;
        for (size_t i = 0; i < sizeof(ext_cfgs) / sizeof(ext_cfgs[0]); i++) {
            uint16_t p = ext_cfgs[i].port_id;
            if (p >= MAX_PORTS) continue;
            tot_rx_pkts  += hw[p].q_opackets[DPDK_EXT_TX_QUEUE_ID];
            tot_rx_bytes += hw[p].q_obytes[DPDK_EXT_TX_QUEUE_ID];
            sent_ext     += hw[p].q_opackets[DPDK_EXT_TX_QUEUE_ID];
        }
    }
#endif

    // DPDK rows only: dtn_stats[32]/[33] are never populated (see above).
    for (uint16_t dtn = 0; dtn < DTN_DPDK_PORT_COUNT; dtn++) {
        tot_good    += (uint64_t)rte_atomic64_read(&dtn_stats[dtn].good_pkts) +
                       (uint64_t)rte_atomic64_read(&dtn_stats[dtn].raw_origin_good);
        tot_bad     += (uint64_t)rte_atomic64_read(&dtn_stats[dtn].bad_pkts) +
                       (uint64_t)rte_atomic64_read(&dtn_stats[dtn].raw_origin_bad);
        tot_lost    += (uint64_t)rte_atomic64_read(&dtn_stats[dtn].lost_pkts);
        tot_bit_err += (uint64_t)rte_atomic64_read(&dtn_stats[dtn].bit_errors) +
                       (uint64_t)rte_atomic64_read(&dtn_stats[dtn].raw_origin_bits);
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

    // ---------- 2. Where every arrival went ----------
    // Each line below is measured, and they are meant to add up: the hardware
    // says how many packets it put on the PRBS queues, and every one of them
    // was either validated as PRBS or turned away for a stated reason.
    uint64_t hm12 = 0, fo12 = 0, hm13 = 0, fo13 = 0;
    raw_socket_get_excluded_counts(0, &hm12, &fo12);
    raw_socket_get_excluded_counts(1, &hm13, &fo13);

    const uint64_t turned_away = tot_short + tot_other;
    const uint64_t hw_accounted = tot_dpdk_validated + turned_away;
    const uint64_t hw_unaccounted =
        (tot_hw_q_pkts > hw_accounted) ? (tot_hw_q_pkts - hw_accounted) : 0;
    const uint64_t hw_over =
        (hw_accounted > tot_hw_q_pkts) ? (hw_accounted - tot_hw_q_pkts) : 0;

    printf("\n--- Where every arrival went ---\n");
    printf("  DPDK queues 0-3, HW packets received      : %lu\n", tot_hw_q_pkts);
    printf("    validated as PRBS (counted above)       : %lu\n", tot_dpdk_validated);
    // Real, validated traffic that simply entered from Port 12/13 instead of
    // from the queue its arrival row is paired with. Included in the figure
    // above and held out of the DTN table's per-row columns, so each row
    // compares one stream against itself. This is the exact count that used
    // to be approximated as "target packets / 4" and added to the wrong side.
    printf("      of which entered from Port 12/13      : %lu\n", tot_raw_origin);
    printf("    undersized, never validated             : %lu\n", tot_short);
    // Name them by size. One dominant length points at a specific protocol;
    // a spread points at something variable. The per-frame detail for the
    // first dozen is logged as they arrive, earlier in the run.
    if (tot_short > 0) {
        uint32_t ulens[8];
        uint64_t ucounts[8];
        int un = txrx_get_undersized_lengths(ulens, ucounts,
                                             (int)(sizeof(ulens) / sizeof(ulens[0])));
        for (int i = 0; i < un; i++) {
            printf("        %5u bytes                          : %lu\n",
                   ulens[i], ucounts[i]);
        }
    }
    printf("    foreign (PTP/ARP/...), never validated  : %lu\n", tot_other);
    if (hw_over) {
        printf("    OVER-COUNTED                            : %lu\n", hw_over);
    } else {
        // Not a mystery: rx_worker gives up on a frame whose VL-ID belongs to
        // no PRBS stream it can check against - "if raw_port not found, just
        // count as external (no PRBS check)" - and such a frame lands in none
        // of the categories above.
        printf("    unrecognised VL-ID, nothing to check     : %lu\n",
               hw_unaccounted);
    }
    // Name the foreign traffic rather than just counting it: "319 foreign
    // frames" does not say whether PTP escaped its queue or the switch is
    // simply flooding LLDP at us.
    if (tot_other > 0) {
        uint16_t ftypes[8];
        uint64_t fcounts[8];
        int fn = txrx_get_foreign_ethertypes(ftypes, fcounts,
                                             (int)(sizeof(ftypes) / sizeof(ftypes[0])));
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
    printf("  Raw Ports 12/13, validated as PRBS        : %lu\n", tot_raw_validated);
    printf("  Health Monitor frames excluded  (P12/P13) : %lu / %lu\n", hm12, hm13);
    printf("  Other foreign frames excluded   (P12/P13) : %lu / %lu\n", fo12, fo13);

    // ---------- 3. Sent vs came back ----------
    // Both sides are now PRBS counters over the same packets, cleared by the
    // same reset, so the difference here is small and can fall either way.
    //
    // A residue here was for a long time blamed on the counter reset. That was
    // wrong, and the per-second tables disproved it: the pairs held to a packet
    // or two for the whole run and only broke in the single second traffic
    // stopped. The cause was the shutdown - the first drain table was rendered
    // while the senders were still leaving, their last sends not yet handed
    // over while the arrivals of those same packets were already counted - and
    // the senders are now waited for before anything is read.
    //
    // So a difference here is no longer explained away. It is reported with its
    // sign, and anything large enough to be real traffic is called out.
    const bool short_return = (tot_rx_pkts > tot_tx_pkts);
    const uint64_t difference = short_return ? (tot_rx_pkts - tot_tx_pkts)
                                             : (tot_tx_pkts - tot_rx_pkts);
    const double diff_pct =
        tot_rx_pkts ? (double)difference * 100.0 / (double)tot_rx_pkts : 0.0;
    // 100 ppm. The reset residue lands three orders of magnitude below this;
    // anything above it is too big to be the boundary and wants investigating.
    const bool diff_significant = (diff_pct > 0.01);

    // Per leg, because the aggregate cannot say which pipeline is short.
    // Signed: clamping a leg that came back with more than it sent to zero
    // printed "difference: 0" for a leg that was actually +218, which reads as
    // a leg that balances rather than one carrying the reset residue.
    const int64_t loop_diff = (int64_t)tot_dpdk_validated - (int64_t)sent_loop;
    const int64_t ext_diff  = (int64_t)tot_raw_validated - (int64_t)sent_ext;

    printf("\n--- Sent vs came back, by path ---\n");
    printf("  Loopback leg (DPDK queues 0-3 + Ports 12/13 TX)\n");
    printf("    sent                                    : %lu\n", sent_loop);
    printf("    returned and validated on DPDK queues   : %lu\n", tot_dpdk_validated);
    printf("    difference (back - sent)                : %+ld\n", (long)loop_diff);
    // The loopback leg is two sub-legs and the aggregate cannot say which is
    // short. The DPDK sub-leg compares each row's TX worker against what came
    // back on its paired queue; the raw sub-leg compares each raw port's own
    // transmissions against the arrivals attributed to it. tx_errors is the
    // count of send() calls the kernel refused - it has always been collected
    // and never shown, so a refused batch looked like a silent loss.
    {
        uint64_t raw_back[MAX_RAW_SOCKET_PORTS] = {0};
        txrx_get_raw_origin_by_port(raw_back, MAX_RAW_SOCKET_PORTS);
        uint64_t dpdk_sent = 0, dpdk_back = 0;
        for (uint16_t d = 0; d < DTN_DPDK_PORT_COUNT; d++) {
            dpdk_sent += (uint64_t)rte_atomic64_read(&dtn_stats[d].prbs_tx_pkts);
            dpdk_back += (uint64_t)rte_atomic64_read(&dtn_stats[d].good_pkts) +
                         (uint64_t)rte_atomic64_read(&dtn_stats[d].bad_pkts);
        }
        printf("    of that, DPDK TX workers  sent/back    : %lu / %lu  (%+ld)\n",
               dpdk_sent, dpdk_back, (long)((int64_t)dpdk_back - (int64_t)dpdk_sent));
        // The two counters above disagree by a couple of hundred packets, and
        // the per-second tables put the whole of it in the single second the
        // TX workers stop. These are the same traffic counted by the workers
        // themselves, outside the flush path entirely: whichever line below is
        // non-zero names the counter that is wrong.
        {
            uint64_t own_tx = 0, own_rx = 0;
            txrx_get_own_worker_totals(&own_tx, &own_rx);
            printf("      TX workers' own count vs sent         : %lu  (%+ld)\n",
                   own_tx, (long)((int64_t)own_tx - (int64_t)dpdk_sent));
            printf("      RX workers' own count vs back         : %lu  (%+ld)\n",
                   own_rx, (long)((int64_t)own_rx - (int64_t)dpdk_back));
            const uint32_t fto = txrx_flush_timeouts();
            printf("      flush requests that timed out        : %u%s\n", fto,
                   fto ? "   <- a table was rendered mid-handover" : "");
        }
        for (int r = 0; r < 2; r++) {
            struct raw_socket_port *rp = &raw_ports[r];
            uint64_t sent = 0, errs = 0;
            for (uint16_t t = 0; t < rp->tx_target_count; t++) {
                pthread_spin_lock(&rp->tx_targets[t].stats.lock);
                sent += rp->tx_targets[t].stats.tx_packets;
                errs += rp->tx_targets[t].stats.tx_errors;
                pthread_spin_unlock(&rp->tx_targets[t].stats.lock);
            }
            printf("    of that, Port %-2u          sent/back    : %lu / %lu  (%+ld)"
                   ", send() errors %lu\n",
                   rp->port_id, sent, raw_back[r],
                   (long)((int64_t)raw_back[r] - (int64_t)sent), errs);
        }
    }
    printf("  External TX leg (queue 4 -> Ports 12/13)\n");
    printf("    sent                                    : %lu\n", sent_ext);
    printf("    returned and validated at Ports 12/13   : %lu\n", tot_raw_validated);
    printf("    difference (back - sent)                : %+ld\n", (long)ext_diff);

    printf("\n--- Sent vs came back ---\n");
    printf("  Sent      (Server->DTN)                   : %lu\n", tot_rx_pkts);
    printf("  Returned and validated (DTN->Server)      : %lu\n", tot_tx_pkts);
    printf("  Difference                                : %s%lu  (%.5f%%)\n",
           short_return ? "-" : "+", difference, diff_pct);
    if (diff_significant) {
        printf("     LARGER THAN THE RESET BOUNDARY CAN EXPLAIN - %s\n",
               short_return ? "packets went out and did not come back"
                            : "the two sides are not counting the same traffic");
    } else if (difference) {
        printf("     small enough to be a counter read against a moving\n");
        printf("     target rather than traffic; sign can fall either way\n");
    }

    // ---------- 3b. Each loopback stream against its own pair ----------
    // The DTN table's TX column carries both pipelines, because that is what
    // the port carried. This comparison needs one: what a TX worker sent
    // toward DTN N comes back out of DTN N+16, so RX(N) is checked against
    // TX(N+16) with the Port 12/13 traffic taken out of it. Anything else in
    // the row would make every pair differ by that port's share of the raw
    // traffic and say nothing about the link.
    {
        uint64_t worst = 0;
        uint16_t worst_pair = 0;
        int64_t  worst_diff = 0;
        printf("\n--- Each stream against its own pair (Port 12/13 traffic excluded) ---\n");
        for (uint16_t n = 0; n < DTN_DPDK_PORT_COUNT / 2; n++) {
            const uint16_t m = n + DTN_DPDK_PORT_COUNT / 2;
            // Sent toward DTN n, returned out of DTN m - and the reverse.
            const uint64_t sent_n = (uint64_t)rte_atomic64_read(&dtn_stats[n].prbs_tx_pkts);
            const uint64_t back_m = (uint64_t)rte_atomic64_read(&dtn_stats[m].good_pkts) +
                                    (uint64_t)rte_atomic64_read(&dtn_stats[m].bad_pkts);
            const uint64_t sent_m = (uint64_t)rte_atomic64_read(&dtn_stats[m].prbs_tx_pkts);
            const uint64_t back_n = (uint64_t)rte_atomic64_read(&dtn_stats[n].good_pkts) +
                                    (uint64_t)rte_atomic64_read(&dtn_stats[n].bad_pkts);
            const int64_t d1 = (int64_t)back_m - (int64_t)sent_n;
            const int64_t d2 = (int64_t)back_n - (int64_t)sent_m;
            const uint64_t a1 = (uint64_t)(d1 < 0 ? -d1 : d1);
            const uint64_t a2 = (uint64_t)(d2 < 0 ? -d2 : d2);
            if (a1 > worst) { worst = a1; worst_pair = n; worst_diff = d1; }
            if (a2 > worst) { worst = a2; worst_pair = m; worst_diff = d2; }
        }
        if (worst == 0) {
            printf("  all %d pairs balance exactly\n", DTN_DPDK_PORT_COUNT);
        } else {
            printf("  worst pair: DTN %u, off by %+ld packets\n",
                   worst_pair, (long)worst_diff);
        }
    }

    // ---------- 4. The device's own view ----------
#if HEALTH_MONITOR_ENABLED
    // The DTN keeps per-port frame counters of its own, and nothing here clears
    // them - so read as absolutes they are always ahead of ours by whatever it
    // carried before our counters were zeroed, which says nothing about the
    // test. A difference between two of its own readings does not have that
    // problem: the baseline was taken in the quiet window with no traffic
    // moving, so the monitor's own 1 Hz sampling cannot skew it.
    //
    // Its Rx on port N is what we sent toward DTN N; its Tx on port N is what
    // we validated coming back from it. Both are compared against exactly the
    // columns of the DTN table.
    {
    bool any = false;
    uint64_t sum_our_sent = 0, sum_dev_rx = 0;
    uint64_t sum_our_back = 0, sum_dev_tx = 0;
    for (uint16_t dtn = 0; dtn < DTN_PORT_COUNT; dtn++) {
        uint64_t dtx = 0, drx = 0;
        if (!health_monitor_get_port_delta((int)dtn, &dtx, &drx)) continue;
        if (!any) {
            printf("\n--- The device's own counters (difference since the quiet window) ---\n");
            printf("  DTN |          we sent |        device Rx |   diff"
                   " |    we validated |        device Tx |   diff\n");
            any = true;
        }
        uint64_t our_sent, our_back;
        if (dtn < DTN_DPDK_PORT_COUNT) {
            our_sent = (uint64_t)rte_atomic64_read(&dtn_stats[dtn].prbs_tx_pkts);
            our_back = (uint64_t)rte_atomic64_read(&dtn_stats[dtn].good_pkts) +
                       (uint64_t)rte_atomic64_read(&dtn_stats[dtn].bad_pkts) +
                       (uint64_t)rte_atomic64_read(&dtn_stats[dtn].raw_origin_good) +
                       (uint64_t)rte_atomic64_read(&dtn_stats[dtn].raw_origin_bad);
        } else {
            // Rows 32/33 are the raw ports: what they sent, and what came
            // back to them from the external-TX leg.
            struct raw_socket_port *rp = &raw_ports[dtn - DTN_RAW_PORT_12];
            our_sent = 0;
            for (uint16_t t = 0; t < rp->tx_target_count; t++) {
                pthread_spin_lock(&rp->tx_targets[t].stats.lock);
                our_sent += rp->tx_targets[t].stats.tx_packets;
                pthread_spin_unlock(&rp->tx_targets[t].stats.lock);
            }
            pthread_spin_lock(&rp->dpdk_ext_rx_stats.lock);
            our_back = rp->dpdk_ext_rx_stats.rx_packets;
            pthread_spin_unlock(&rp->dpdk_ext_rx_stats.lock);
        }
        sum_our_sent += our_sent; sum_dev_rx += drx;
        sum_our_back += our_back; sum_dev_tx += dtx;
        printf("  %3u | %16lu | %16lu | %+6ld | %15lu | %16lu | %+6ld\n",
               dtn, our_sent, drx, (long)((int64_t)drx - (int64_t)our_sent),
               our_back, dtx, (long)((int64_t)dtx - (int64_t)our_back));
    }
    if (any) {
        printf("  --- | %16lu | %16lu | %+6ld | %15lu | %16lu | %+6ld\n",
               sum_our_sent, sum_dev_rx,
               (long)((int64_t)sum_dev_rx - (int64_t)sum_our_sent),
               sum_our_back, sum_dev_tx,
               (long)((int64_t)sum_dev_tx - (int64_t)sum_our_back));
    } else {
        printf("\n--- The device's own counters ---\n");
        printf("  no baseline (health monitor had no reading before the test)\n");
    }
    }
#endif

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
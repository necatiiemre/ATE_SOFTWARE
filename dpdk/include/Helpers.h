#pragma once
#include <signal.h>
#include <getopt.h>
#include "Common.h"
#include "Port.h"

// Daemon mode flag - when true, ANSI escape codes are disabled for log file output
extern bool g_daemon_mode;

/**
 * Set daemon mode flag
 * When enabled, ANSI escape codes are disabled in helper_print_stats
 */
void helper_set_daemon_mode(bool enabled);

/**
 * Signal handler for graceful shutdown
 * Catches SIGINT (Ctrl+C) and SIGTERM
 */
static void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\n\nSignal %d received, preparing to exit...\n", signum);
        force_quit = true;
    }
}

// Resets HW statistics and local counters. Called once just before the test
// loop starts so the first printed second is the first second of the test.
// Where the per VL-ID counter table is written at the end of a test. Beside
// the summary log rather than in it: about 4,400 VL-IDs carry traffic, which
// is too many to print but exactly what you want when one of them stops
// matching.
#ifndef VL_COUNTER_LOG_PATH
#define VL_COUNTER_LOG_PATH "/tmp/DTN_IRSW_EQ_VL_ID_Counters.log"
#endif

void helper_reset_stats(const struct ports_config *ports_config,
                        uint64_t prev_tx_bytes[], uint64_t prev_rx_bytes[]);

// Call every second: prints full statistics table + queue distributions
void helper_print_stats(const struct ports_config *ports_config,
                        const uint64_t prev_tx_bytes[], const uint64_t prev_rx_bytes[],
                        unsigned test_time);

// Call ONCE at the end of the run, after the RX drain window has closed and
// before the snapshot is frozen. Prints the aggregate PRBS totals, a purity
// check proving no PTP / Health Monitor / other foreign traffic reached those
// counters, and the PTP and Health Monitor totals separately. Also stored in
// the shutdown snapshot so it appears in the summary log.
void helper_print_final_totals(const struct ports_config *ports_config,
                               unsigned test_time);
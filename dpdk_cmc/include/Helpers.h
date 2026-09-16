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

// Resets HW statistics and local counters during warm-up
void helper_reset_stats(const struct ports_config *ports_config,
                        uint64_t prev_tx_bytes[], uint64_t prev_rx_bytes[]);

// Call every second: prints full statistics table + queue distributions
void helper_print_stats(const struct ports_config *ports_config,
                        const uint64_t prev_tx_bytes[], const uint64_t prev_rx_bytes[],
                        bool warmup_complete, unsigned loop_count, unsigned test_time);

/**
 * End-of-run totals built from the software counters, so the TX/RX figures
 * cover PRBS traffic only. Health-monitor traffic is reported in its own
 * table, and a third table reconciles both against the hardware queue counter.
 *
 * Meant for the final snapshot rather than the live view: call it once TX has
 * stopped and the RX drain window has closed.
 */
void helper_print_prbs_summary(const struct ports_config *ports_config,
                               uint32_t test_seconds);
#include "FinalReport.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "Config.h"
#include "Helpers.h"
#include "Port.h"
#include "VlFlowReport.h"
#include "health_monitor.h"
#include "PsuTelemetryReceiver.h"
#include "AteMode.h"

/* Swap fd 1 for the report file, remembering the original so it can be put
 * back. Returns the saved descriptor, or -1 if the redirect did not happen
 * (in which case the caller must not try to restore). */
static int stdout_redirect_to(const char *path)
{
    fflush(stdout);

    int saved = dup(STDOUT_FILENO);
    if (saved < 0) {
        return -1;
    }

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        close(saved);
        return -1;
    }

    if (dup2(fd, STDOUT_FILENO) < 0) {
        close(fd);
        close(saved);
        return -1;
    }
    close(fd);
    return saved;
}

static void stdout_restore(int saved)
{
    if (saved < 0) {
        return;
    }
    fflush(stdout);
    dup2(saved, STDOUT_FILENO);
    close(saved);
}

int final_report_write(const char *path,
                       const struct ports_config *ports_config,
                       const uint64_t prev_tx_bytes[],
                       const uint64_t prev_rx_bytes[],
                       uint32_t test_seconds,
                       bool warmup_complete)
{
    if (path == NULL || ports_config == NULL) {
        return -1;
    }

    /* Kept in the signature so the caller does not have to care whether the
     * snapshot happens to need the rate baselines; the PRBS table works off
     * cumulative software counters and does not. */
    (void)prev_tx_bytes;
    (void)prev_rx_bytes;

    int saved = stdout_redirect_to(path);
    if (saved < 0) {
        printf("FINAL: cannot open '%s' for writing, skipping snapshot\n", path);
        return -1;
    }

    /* The stats table clears the screen with ANSI escapes unless it believes
     * it is writing to a log. For the duration of the capture, it is. */
    const bool prev_daemon_mode = g_daemon_mode;
    helper_set_daemon_mode(true);

    char ts[64];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    if (tm_info == NULL || strftime(ts, sizeof(ts), "%B %d, %Y %H:%M:%S", tm_info) == 0) {
        snprintf(ts, sizeof(ts), "unknown");
    }

    printf("================================================================================\n");
    printf(" CMC FINAL RESULT\n");
    printf("================================================================================\n");
    printf(" Generated      : %s\n", ts);
    printf(" Test duration  : %u s after warm-up%s\n", test_seconds,
           warmup_complete ? "" : "  (WARM-UP NEVER COMPLETED)");
    printf(" Mode           : %s\n",
           ate_mode_enabled() ? "ATE loopback (pure PRBS)"
                              : "Unit test (SplitMix64 + CRC32C + XOR + PRBS)");
    printf(" Drain window   : %u s of RX-only traffic before this snapshot\n",
           (unsigned)CMC_DRAIN_SECONDS);
    printf(" Per-VL detail  : %s (and %s)\n",
           CMC_VL_TABLE_LOG_PATH, CMC_VL_TABLE_CSV_PATH);
    printf("================================================================================\n");

    /* 1. Final per-line TX/RX totals, PRBS traffic only.
     *
     * Deliberately not helper_print_stats(): that one reports the hardware
     * per-queue counters, which lump health-monitor packets in with PRBS and
     * cannot be separated after the fact. The verdict at the end of a run
     * should be about the PRBS stream alone, so this table is built from the
     * software counters and the health-monitor traffic is reported next to it
     * on its own. */
    helper_print_prbs_summary(ports_config, test_seconds);

    /* 2. VL-to-VL roll-up per DSM line and DPM block. */
    vlflow_print_summary(test_seconds);

    /* 3. PSU telemetry as of the last packet received from MainSoftware. */
    psu_telem_print_table();

    /* 4. Health monitor: last value seen from every source, not just the
     *    ones that happened to report in the final second. */
    hm_print_last_snapshot();

    printf("\n");
    printf("================================================================================\n");
    printf(" END OF FINAL RESULT\n");
    printf("================================================================================\n");

    helper_set_daemon_mode(prev_daemon_mode);
    stdout_restore(saved);

    printf("FINAL: final result snapshot written to %s\n", path);
    return 0;
}

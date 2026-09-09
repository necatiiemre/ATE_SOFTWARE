/*
 * VMC acceleration test.
 *
 * The VMC sends its health monitor without being asked, so this test does not
 * configure anything: it opens one interface, sorts what arrives into the six
 * reports the VMC produces from each of its two sides, and keeps the dashboard
 * up until the operator stops it.
 *
 * Which interface, which VL carries which report, and which message id sorts
 * the four CBIT reports apart are all in AppConfig.h - one table, so a
 * different rig is one edit and nothing here changes.
 */

#include "units/VmcTest.h"

#include "AppConfig.h"
#include "HealthMonitor.h"
#include "Log.h"
#include "Prompt.h"
#include "RawSocket.h"
#include "SafeShutdown.h"
#include "VmcHealth.h"

#include <stdio.h>
#include <string.h>

#define RX_BUFFER_SIZE 4096

static uint8_t      g_rx[RX_BUFFER_SIZE];
static raw_socket_t g_link;
static vmc_health_t g_health;

static void close_socket_action(void *ctx)
{
    raw_socket_close((raw_socket_t *)ctx);
}

static void print_plan(const vmc_config_t *c)
{
    printf("\n  listening   : %s\n", c->iface);
    printf("  reports     : six per side, sorted by VL id\n\n");
    printf("    %-21s  %6s  %6s\n", "report", "FLCS", "VS");
    printf("    %-21s  %6s  %6s\n", "---------------------", "------", "------");
    for (int r = 0; r < VMC_REPORT_COUNT; r++)
        printf("    %-21s  %6u  %6u\n", vmc_report_name((vmc_report_t)r),
               vmc_report_vl(c, VMC_FLCS, (vmc_report_t)r),
               vmc_report_vl(c, VMC_VS, (vmc_report_t)r));
    printf("\n  the four CBIT reports share a VL and are told apart by the "
           "message id:\n");
    printf("    %u DTN end system, %u DTN switch, %u board monitor, %u board flags\n",
           c->msg_dtn_es, c->msg_dtn_sw, c->msg_bm_engineering, c->msg_bm_flag);
    printf("  PBIT is guarded by message id %u, because other traffic shares "
           "its VL.\n", c->msg_pbit_response);
}

/**
 * @brief Wait until something arrives, so a dead link is not mistaken for a
 *        quiet one.
 */
static bool wait_for_unit(unsigned timeout_s)
{
    uint64_t deadline = hm_now_ms() + (uint64_t)timeout_s * 1000u;

    log_line("waiting for the VMC on %s (up to %u s)", g_link.name, timeout_s);
    while (hm_now_ms() < deadline) {
        if (safe_shutdown_requested())
            return false;

        int n = raw_socket_recv(&g_link, g_rx, sizeof g_rx, 500);
        if (n <= 0)
            continue;
        if (vmc_health_ingest(&g_health, g_rx, (size_t)n)) {
            log_line("VMC is up: first report on %s", g_link.name);
            return true;
        }
    }
    log_line("no VMC health monitor within %u s", timeout_s);
    return false;
}

static void monitor_run(const timing_config_t *timing)
{
    hm_watch_t watch;
    uint64_t   started = hm_now_ms();
    uint64_t   next_draw = started;
    unsigned   interruptions = 0;

    hm_watch_init(&watch);
    watch.alive = true;
    log_line("monitoring - press Ctrl+C to end the test");

    while (!safe_shutdown_requested()) {
        int n = raw_socket_recv(&g_link, g_rx, sizeof g_rx, 100);
        if (n > 0 && vmc_health_ingest(&g_health, g_rx, (size_t)n))
            hm_watch_saw_frame(&watch);

        /* The VMC is powered separately, so a gap in the reports is the only
         * sign the run has of it dropping out. Nothing is re-sent afterwards -
         * unlike the DTN, the VMC needs no configuration from us. */
        if (hm_watch_update(&watch, timing->heartbeat_timeout_ms)) {
            if (!watch.alive) {
                interruptions++;
                log_line("VMC WENT QUIET - no reports for %u ms (event %u)",
                         timing->heartbeat_timeout_ms, interruptions);
            } else {
                log_line("VMC is back after event %u", interruptions);
            }
        }

        uint64_t now = hm_now_ms();
        if (now >= next_draw) {
            next_draw = now + timing->display_interval_ms;
            vmc_health_render(&g_health, (now - started) / 1000);
            printf("\n%u interruption(s)   %s\n", interruptions,
                   watch.alive ? "VMC ALIVE" : "VMC QUIET");
            puts("Ctrl+C to end the test");
            fflush(stdout);
        }
    }

    uint64_t elapsed = (hm_now_ms() - started) / 1000;
    printf("\n");
    log_line("test stopped by the operator");
    log_line("elapsed %llus, %llu reports, %u interruption(s)",
             (unsigned long long)elapsed, (unsigned long long)watch.frames,
             interruptions);
    vmc_health_log_summary(&g_health);
}

unit_result_t vmc_test_run(void)
{
    const vmc_config_t    *config = app_config_vmc();
    const timing_config_t *timing = app_config_timing();
    int handle = -1;
    unit_result_t result = UNIT_RESULT_ERROR;

    g_link.fd = -1;
    vmc_health_init(&g_health, config);

    print_plan(config);

    if (!prompt_yes_no("\nStart the test", false))
        return UNIT_RESULT_ABORTED;

    bool carrier = false;
    if (!raw_socket_link_up(config->iface, &carrier)) {
        printf("%s is down. Bring it up first.\n", config->iface);
        return UNIT_RESULT_ERROR;
    }
    if (!carrier)
        printf("Warning: %s has no carrier - is the cable connected?\n", config->iface);

    if (!log_open("VMC", "health"))
        puts("Warning: could not open a log file; the run will not be recorded.");
    else
        printf("Logging to %s\n\n", log_path());

    if (!raw_socket_open(&g_link, config->iface, true))
        goto done;
    handle = safe_shutdown_register(config->iface, SHUTDOWN_PRIO_SOCKET,
                                    close_socket_action, &g_link);

    log_line("VMC health monitor on %s", config->iface);
    if (!wait_for_unit(timing->device_ready_timeout_s)) {
        result = safe_shutdown_requested() ? UNIT_RESULT_ABORTED : UNIT_RESULT_ERROR;
        goto done;
    }

    monitor_run(timing);
    result = UNIT_RESULT_PASS;

done:
    safe_shutdown_unregister(handle);
    raw_socket_close(&g_link);
    log_close();
    safe_shutdown_clear();
    return result;
}

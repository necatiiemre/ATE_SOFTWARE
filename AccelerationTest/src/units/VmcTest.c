/*
 * VMC acceleration test.
 *
 * The VMC sends its health monitor without being asked, so this test does not
 * configure anything: it opens both interfaces, sorts what arrives into the six
 * reports the VMC produces from each of its two sides, and keeps the dashboard
 * up until the operator stops it.
 *
 * One interface per side - the first carries FLCS, the second VS - so the link
 * a frame arrived on is what says which side it belongs to. Which interfaces,
 * which VL carries which report, and which message id sorts the four CBIT
 * reports apart are all in AppConfig.h: one table, so a different rig is one
 * edit and nothing here changes.
 */

#include "units/VmcTest.h"

#include "AppConfig.h"
#include "HealthMonitor.h"
#include "Log.h"
#include "Prompt.h"
#include "RawSocket.h"
#include "SafeShutdown.h"
#include "VmcHealth.h"
#include "VmcPrint.h"

#include <stdio.h>
#include <string.h>

#define RX_BUFFER_SIZE 4096

static uint8_t      g_rx[RX_BUFFER_SIZE];
static raw_socket_t g_links[APP_MAX_VMC_LINKS];
static vmc_health_t g_health;

/* dpdk_vmc stops the test when a temperature stays outside its limits for ten
 * consecutive reports. The check is copied along with the printers, so give it
 * a flag to raise and end the run the same way. */
static volatile bool g_temperature_abort;

static void close_socket_action(void *ctx)
{
    raw_socket_close((raw_socket_t *)ctx);
}

static void print_plan(const vmc_config_t *c)
{
    printf("\n  listening   :");
    for (uint8_t l = 0; l < c->link_count; l++)
        printf(" %s (%s)%s", c->links[l].iface,
               vmc_side_name((vmc_side_t)c->links[l].side),
               l + 1 < c->link_count ? "," : "\n");
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
    printf("\n  the side comes from the interface, not the VL id - a report whose\n");
    printf("  VL names the other side is still filed by its cable, and counted.\n");
}

/**
 * @brief Wait until something arrives, so a dead link is not mistaken for a
 *        quiet one.
 */
static bool wait_for_unit(const vmc_config_t *config, unsigned timeout_s)
{
    uint64_t deadline = hm_now_ms() + (uint64_t)timeout_s * 1000u;

    log_line("waiting for the VMC on %s and %s (up to %u s)",
             config->links[0].iface, config->links[1].iface, timeout_s);
    while (hm_now_ms() < deadline) {
        if (safe_shutdown_requested())
            return false;

        size_t which = 0;
        int n = raw_socket_recv_any(g_links, config->link_count, g_rx, sizeof g_rx,
                                    500, &which);
        if (n <= 0)
            continue;
        if (vmc_health_ingest(&g_health, (uint8_t)which, g_rx, (size_t)n)) {
            log_line("VMC is up: first report on %s (%s)",
                     config->links[which].iface,
                     vmc_side_name((vmc_side_t)config->links[which].side));
            return true;
        }
    }
    log_line("no VMC health monitor within %u s", timeout_s);
    return false;
}

static void monitor_run(const vmc_config_t *config, const timing_config_t *timing)
{
    hm_watch_t watch;
    uint64_t   started = hm_now_ms();
    uint64_t   next_draw = started;
    unsigned   interruptions = 0;

    hm_watch_init(&watch);
    watch.alive = true;
    log_line("monitoring - press Ctrl+C to end the test");

    while (!safe_shutdown_requested() && !g_temperature_abort) {
        size_t which = 0;
        int n = raw_socket_recv_any(g_links, config->link_count, g_rx, sizeof g_rx,
                                    100, &which);
        if (n > 0 && vmc_health_ingest(&g_health, (uint8_t)which, g_rx, (size_t)n))
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
            printf("[ATE] %llus elapsed, %u interruption(s), %s - Ctrl+C to end\n",
                   (unsigned long long)((now - started) / 1000), interruptions,
                   watch.alive ? "VMC alive" : "VMC QUIET");
            fflush(stdout);
        }
    }

    uint64_t elapsed = (hm_now_ms() - started) / 1000;
    printf("\n");
    if (g_temperature_abort)
        log_line("test stopped by the temperature check");
    else
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
    int handles[APP_MAX_VMC_LINKS];
    unit_result_t result = UNIT_RESULT_ERROR;

    for (size_t i = 0; i < APP_MAX_VMC_LINKS; i++) {
        g_links[i].fd = -1;
        handles[i] = -1;
    }
    g_temperature_abort = false;
    hm_set_abort_flag(&g_temperature_abort);
    vmc_health_init(&g_health, config);

    print_plan(config);

    if (!prompt_yes_no("\nStart the test", false))
        return UNIT_RESULT_ABORTED;

    for (uint8_t l = 0; l < config->link_count; l++) {
        bool carrier = false;

        if (!raw_socket_link_up(config->links[l].iface, &carrier)) {
            printf("%s is down. Bring it up first.\n", config->links[l].iface);
            return UNIT_RESULT_ERROR;
        }
        if (!carrier)
            printf("Warning: %s has no carrier - is the cable connected?\n",
                   config->links[l].iface);
    }

    if (!log_open("VMC", "health"))
        puts("Warning: could not open a log file; the run will not be recorded.");
    else
        printf("Logging to %s\n\n", log_path());

    for (uint8_t l = 0; l < config->link_count; l++) {
        if (!raw_socket_open(&g_links[l], config->links[l].iface, true))
            goto done;
        handles[l] = safe_shutdown_register(config->links[l].iface,
                                            SHUTDOWN_PRIO_SOCKET,
                                            close_socket_action, &g_links[l]);
        log_line("VMC health monitor: %s carries %s", config->links[l].iface,
                 vmc_side_name((vmc_side_t)config->links[l].side));
    }

    if (!wait_for_unit(config, timing->device_ready_timeout_s)) {
        result = safe_shutdown_requested() ? UNIT_RESULT_ABORTED : UNIT_RESULT_ERROR;
        goto done;
    }

    monitor_run(config, timing);
    result = UNIT_RESULT_PASS;

done:
    for (size_t i = 0; i < APP_MAX_VMC_LINKS; i++) {
        safe_shutdown_unregister(handles[i]);
        raw_socket_close(&g_links[i]);
    }
    log_close();
    safe_shutdown_clear();
    return result;
}

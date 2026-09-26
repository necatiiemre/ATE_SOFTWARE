#include "CmcTest.h"

#include "AppConfig.h"
#include "CmcDataPlane.h"
#include "CmcPacket.h"
#include "CmcPmm.h"
#include "CmcStats.h"
#include "Log.h"
#include "MmmsHandler.h"
#include "Prompt.h"
#include "SafeShutdown.h"
#include "health_monitor.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Where the unit's MMMS logs land when the handover runs. The reference's path,
 * kept so whoever knows where to look still knows. */
#define CMC_MMMS_OUTPUT_DIR "/tmp/mmms_logs"

/* Threads read this; the dashboard loop sets it. */
static volatile bool g_stop;

static cmc_data_plane_t *g_dp;
static cmc_pmm_t        *g_pmm;

/* ------------------------------------------------------------------ */
/* The sink: what the data plane does with traffic that is not its own */
/* ------------------------------------------------------------------ */

/* The health monitor rides on the DSM links. hm_handle_packet only ever sees a
 * UDP payload, which is exactly what the data plane hands it, so the reference's
 * decoder is called unchanged. */
static void on_health(uint16_t vl_id, const uint8_t *payload, uint16_t len)
{
    hm_handle_packet(vl_id, payload, len);
}

static void on_mmms(const uint8_t *payload, uint16_t len)
{
    mmms_handle_packet(payload, len);
}

static const cmc_sink_t g_sink = {
    .is_health_vl = hm_is_health_monitor_vl_id,
    .health       = on_health,
    .mmms_vl_id   = MMMS_RESPONSE_VL_ID,
    .mmms         = on_mmms,
};

/* ------------------------------------------------------------------ */
/* Setup                                                              */
/* ------------------------------------------------------------------ */

static void prbs_progress(size_t done, size_t total)
{
    printf("\r  PRBS-31: %zu of %zu MB", done / (1024 * 1024), total / (1024 * 1024));
    fflush(stdout);
}

static void print_plan(const cmc_config_t *c)
{
    log_line("CMC acceleration test");
    putchar('\n');
    log_line("  the data plane, both networks given the same traffic:");
    for (uint8_t n = 0; n < c->net_count; n++)
        log_line("    %-8s %-5s %-6s  VL %u..%u out, %u..%u back, src MAC tail 0x%02X",
                 c->nets[n].iface, c->nets[n].unit_label, c->nets[n].label,
                 c->tx_vl_start, c->tx_vl_start + c->vl_count - 1,
                 c->rx_vl_start, c->rx_vl_start + c->vl_count - 1,
                 c->nets[n].src_mac_tail);
    log_line("    %.4f Gbps in total, %u byte frames, %s",
             c->target_gbps, (unsigned)CMC_FRAME_LEN(c->vlan_tagged),
             c->vlan_tagged ? "802.1Q tagged" : "untagged");
    putchar('\n');
    log_line("  the PMM lines, listen only:");
    for (uint8_t p = 0; p < c->pmm_count; p++)
        log_line("    %-8s %-5s  %s:%u -> %s:%u",
                 c->pmms[p].iface, c->pmms[p].label,
                 c->pmms[p].unit_ip, c->pmms[p].unit_port,
                 c->pmms[p].local_ip, c->pmms[p].local_port);
    putchar('\n');
    log_line("  the health monitors come in on the two DSM links and are decoded");
    log_line("  the first %u s is warm-up; the counters are zeroed at the end of it",
             c->warmup_s);
    log_line("  Ctrl+C stops the traffic and asks the unit for its MMMS logs");
    putchar('\n');
    log_line("  the unit's power is not touched - switch it on by hand first");
}

/* ------------------------------------------------------------------ */
/* The dashboard                                                      */
/* ------------------------------------------------------------------ */

/*
 * One redraw. The order is the reference's with one thing moved: it prints the
 * loss tables first and the health monitor after, and here the health monitor
 * comes first and the loss tables last. That is the only difference in the
 * output, and it is the one that was asked for - the tables are what an operator
 * watches, and the bottom of the screen is where they stay put while the health
 * monitor above them grows and shrinks.
 */
static void redraw(const cmc_config_t *c, cmc_stats_view_t *view,
                   bool warmup_complete, unsigned elapsed_s)
{
    /* The screen is cleared once, here, rather than inside the tables the way
     * the reference does it - they are no longer the first thing printed. */
    printf("\033[2J\033[H");

    hm_print_dashboard();

    cmc_stats_print_banner(c, warmup_complete, elapsed_s);
    cmc_stats_print_all(view, g_dp, c);
    cmc_pmm_print_table(g_pmm);
    cmc_stats_print_warnings(g_dp, c);

    printf("\n  Press Ctrl+C to stop\n");
    fflush(stdout);
}

/*
 * The run. One second per turn: redraw, and watch for the two things that change
 * phase - the end of warm-up, and the operator asking to stop.
 *
 * Ctrl+C is two-stage, as the reference has it. The first one stops the
 * data-plane traffic and asks the unit for its MMMS logs, which needs an
 * otherwise idle wire; the handover then runs until the unit says it is finished
 * or the timeout passes. A second Ctrl+C during that gives up on the logs and
 * stops. The stop request is cleared after the first press so the second one is
 * visible as a new request rather than as the same one still standing.
 */
static void run(const cmc_config_t *c)
{
    cmc_stats_view_t view;
    bool warmup_complete = false;
    bool handover = false;
    unsigned loop_count = 0;         /* seconds since the traffic started */
    unsigned test_time = 0;          /* seconds since warm-up ended */

    /* The redraw interval, in seconds. Both counters are kept in seconds rather
     * than in ticks, so the warm-up boundary stays where it was configured
     * whatever this is set to. */
    const unsigned tick = c->stats_interval_s ? c->stats_interval_s : 1;

    cmc_stats_view_reset(&view);
    log_line("running - press Ctrl+C to stop");

    for (;;) {
        sleep(tick);
        loop_count += tick;

        if (safe_shutdown_requested()) {
            if (!handover) {
                handover = true;
                safe_shutdown_clear();

                log_line("stopping the traffic and asking the unit for its MMMS logs "
                         "(Ctrl+C again to skip)");
                cmc_data_plane_pause_tx(g_dp, true);
                usleep(2000);          /* let the sender finish its slot */

                /* Network A's link, borrowed: the traffic is paused, so
                 * nothing else is using it, and a second socket on the same
                 * interface would buy nothing. */
                if (mmms_send_trigger(cmc_data_plane_link(g_dp, 0),
                                      c->vlan_tagged) != 0) {
                    log_line("could not send the MMMS trigger - stopping without "
                             "the unit's logs");
                    break;
                }
            } else {
                log_line("giving up on the MMMS logs at the operator's request");
                break;
            }
        }

        if (handover) {
            mmms_check_timeout();
            if (mmms_is_done()) {
                log_line("MMMS handover finished");
                break;
            }
            fflush(stdout);
            continue;
        }

        if (!warmup_complete && loop_count >= c->warmup_s) {
            printf("\n═══════════════════════════════════════════════════════════════\n");
            printf("   WARM-UP COMPLETE - RESETTING STATS - TEST STARTING NOW\n");
            printf("═══════════════════════════════════════════════════════════════\n\n");
            log_line("warm-up over after %u s - counters zeroed, the test starts now",
                     loop_count);

            cmc_data_plane_reset(g_dp);
            cmc_pmm_reset(g_pmm);
            cmc_stats_view_reset(&view);

            warmup_complete = true;
            test_time = 0;
            sleep(2);
            continue;
        }

        if (warmup_complete)
            test_time += tick;

        redraw(c, &view, warmup_complete, warmup_complete ? test_time : loop_count);
    }

    log_line("test ran %u s in total, %u s of it after warm-up", loop_count, test_time);
}

/* ------------------------------------------------------------------ */

static void stop_threads_action(void *ctx)
{
    (void)ctx;
    g_stop = true;
}

unit_result_t cmc_test_run(void)
{
    const cmc_config_t *c = app_config_cmc();
    cmc_prbs_cache_t prbs = {0};
    unit_result_t result = UNIT_RESULT_ERROR;
    int stop_handle = -1;

    g_stop = false;

    /* The log is a transcript of the terminal, so it opens before anything is
     * printed about the run. */
    if (!log_open("CMC", "dataplane"))
        puts("Warning: could not open a log file; the run will not be recorded.");
    else
        printf("Logging to %s\n", log_path());

    print_plan(c);

    if (!prompt_yes_no("\nStart the test", false)) {
        log_close();
        return UNIT_RESULT_ABORTED;
    }

    /* The PRBS stream: a quarter of a gigabyte, generated once. Both networks
     * read the same one, as the reference's single server port does. */
    printf("\nGenerating the PRBS-31 stream (%zu MB) - this takes a moment...\n",
           CMC_PRBS_CACHE_SIZE / (1024 * 1024));
    if (!cmc_prbs_cache_init(&prbs, CMC_PRBS_INITIAL_STATE, prbs_progress)) {
        printf("\nCould not allocate the PRBS stream.\n");
        log_close();
        return UNIT_RESULT_ERROR;
    }
    printf("\r  PRBS-31: ready (%zu MB, initial state 0x%08X)        \n",
           CMC_PRBS_CACHE_SIZE / (1024 * 1024), prbs.initial_state);

    g_dp = cmc_data_plane_create(c, &prbs, &g_sink, &g_stop);
    g_pmm = cmc_pmm_create(c, &g_stop);
    if (!g_dp || !g_pmm)
        goto done;

    stop_handle = safe_shutdown_register("CMC threads", SHUTDOWN_PRIO_SOCKET,
                                        stop_threads_action, NULL);

    if (!cmc_data_plane_open(g_dp) || !cmc_pmm_open(g_pmm))
        goto done;

    /* The output directory is prepared now rather than when the handover starts,
     * so a failure to make it is an answer before the run instead of after it. */
    if (mmms_init(CMC_MMMS_OUTPUT_DIR) != 0)
        log_line("MMMS logs will not be collected: %s is not writable",
                 CMC_MMMS_OUTPUT_DIR);

    if (!cmc_data_plane_start(g_dp) || !cmc_pmm_start(g_pmm))
        goto done;

    run(c);
    result = UNIT_RESULT_PASS;

done:
    g_stop = true;
    if (g_dp)
        cmc_data_plane_stop(g_dp);
    if (g_pmm)
        cmc_pmm_stop(g_pmm);

    mmms_finalize();

    putchar('\n');
    log_line("CMC data plane:");
    for (uint8_t n = 0; n < c->net_count && g_dp; n++)
        cmc_stats_log_net(g_dp, c, n);
    for (uint8_t p = 0; p < c->pmm_count && g_pmm; p++) {
        const cmc_pmm_stats_t *st = cmc_pmm_stats(g_pmm, p);

        if (!st)
            continue;
        log_line("  %s on %s: %" PRIu64 " packet(s), CRC ok %" PRIu64
                 ", CRC failed %" PRIu64 ", lost %" PRIu64 ", last seq %" PRIu64,
                 c->pmms[p].label, c->pmms[p].iface, st->rx_pkts,
                 st->crc_ok, st->crc_fail, st->lost_pkts, st->last_seq);
    }
    if (g_pmm && cmc_pmm_crc_variant(g_pmm))
        log_line("  the PMM CRC variant in use was %s", cmc_pmm_crc_variant(g_pmm));

    safe_shutdown_unregister(stop_handle);
    cmc_data_plane_destroy(g_dp);
    cmc_pmm_destroy(g_pmm);
    g_dp = NULL;
    g_pmm = NULL;
    cmc_prbs_cache_free(&prbs);

    log_close();
    safe_shutdown_clear();
    return result;
}

/*
 * DTN acceleration test.
 *
 * The operator picks one of the three rounds; the profile becomes a VL table,
 * the VL table becomes configuration frames, and those go out of the copper
 * link. From then on the test watches the health-monitor stream until the
 * operator stops it.
 *
 * The unit is powered separately. What the test does care about is power
 * *dropping* mid-run: on a vibration rig that is a likely fault and probably
 * the most valuable thing the run can catch. When the health monitor goes
 * quiet and comes back, the DTN has rebooted and lost its configuration, so the
 * test re-sends it and records both the loss and the recovery.
 */

#include "units/DtnTest.h"

#include "AppConfig.h"
#include "DtnConfig.h"
#include "HealthMonitor.h"
#include "Log.h"
#include "Prompt.h"
#include "RawSocket.h"
#include "SafeShutdown.h"
#include "VlProfile.h"
#include "VlWatch.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define RX_BUFFER_SIZE 2048

static dtn_vl_t     g_records[VL_PROFILE_MAX_RECORDS];
static dtn_frame_t  g_frames[DTN_MAX_CONFIG_FRAMES];
static uint8_t      g_rx[RX_BUFFER_SIZE];
static raw_socket_t g_links[APP_MAX_COPPER_LINKS];
static vl_watch_t   g_watch;

static void sleep_ms(unsigned ms)
{
    struct timespec ts = {.tv_sec = ms / 1000, .tv_nsec = (long)(ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

static void close_socket_action(void *ctx)
{
    raw_socket_close((raw_socket_t *)ctx);
}

/* ------------------------------------------------------------------ */

static const vl_profile_t *select_profile(void)
{
    size_t count;
    const vl_profile_t *profiles = vl_profile_all(&count);

    puts("\nSelect configuration round");
    for (size_t i = 0; i < count; i++)
        printf("  %zu) %-8s %s\n", i + 1, profiles[i].name, profiles[i].description);
    puts("  0) Back");

    int choice = prompt_menu("Choice", 0, (int)count, 0);
    return choice == 0 ? NULL : &profiles[choice - 1];
}

/* Show the routing in three groups: the fibre links under test, the taps that
 * bring the fibre-side unit's health monitor out to copper, and the DTN's own
 * management path. Consecutive VLs with the same endpoints collapse into a run. */
static int first_destination(const dtn_vl_t *record)
{
    for (int p = 0; p < DTN_PORT_COUNT; p++)
        if (record->dest_mask >> p & 1)
            return p;
    return -1;
}

static bool is_management(const dtn_vl_t *record)
{
    return record->src_port >= 32 || first_destination(record) == 34;
}

static bool is_hm_tap(const dtn_vl_t *record)
{
    int dst = first_destination(record);
    return !is_management(record) && (dst == 32 || dst == 33);
}

static void print_group(const char *title, const dtn_vl_t *records, size_t count,
                        bool (*belongs)(const dtn_vl_t *), bool collapse)
{
    bool titled = false;

    for (size_t i = 0; i < count;) {
        if (!dtn_vl_enabled(&records[i]) || !belongs(&records[i])) {
            i++;
            continue;
        }
        size_t j = i + 1;
        if (collapse)
            while (j < count && dtn_vl_enabled(&records[j]) && belongs(&records[j]) &&
                   records[j].src_port  == records[i].src_port &&
                   records[j].dest_mask == records[i].dest_mask &&
                   records[j].vl_id     == records[j - 1].vl_id + 1)
                j++;

        if (!titled) {
            printf("\n  %s\n", title);
            titled = true;
        }
        printf("    port %2u -> %2d   VL %u", records[i].src_port,
               first_destination(&records[i]), records[i].vl_id);
        if (j - i > 1)
            printf("-%u  (%zu VLs)", records[j - 1].vl_id, j - i);
        putchar('\n');
        i = j;
    }
}

static bool is_fibre_link(const dtn_vl_t *record)
{
    return !is_management(record) && !is_hm_tap(record);
}

static void print_routing(const dtn_vl_t *records, size_t count)
{
    print_group("fibre links under test", records, count, is_fibre_link, true);
    print_group("health-monitor taps (fibre-side unit -> copper)",
                records, count, is_hm_tap, true);
    print_group("DTN management path (its own health monitor and status replies)",
                records, count, is_management, false);
}

/* ------------------------------------------------------------------ */

/**
 * @brief Wait until the unit starts talking, on whichever copper link.
 *
 * The power-up broadcast is not routed by the VL table, and the one place it
 * has been observed is the 1G link - so listen to both rather than assume.
 */
static bool wait_for_unit(size_t link_count, unsigned timeout_s)
{
    uint64_t deadline = hm_now_ms() + (uint64_t)timeout_s * 1000u;

    log_line("waiting for the unit on %s and %s (up to %u s)",
             g_links[0].name, g_links[1].name, timeout_s);
    while (hm_now_ms() < deadline) {
        if (safe_shutdown_requested())
            return false;

        size_t which = 0;
        int n = raw_socket_recv_any(g_links, link_count, g_rx, sizeof g_rx, 500, &which);
        if (n <= 0)
            continue;

        hm_frame_t frame;
        if (hm_classify(g_rx, (size_t)n, &frame)) {
            log_line("unit is up: %zu byte packet on %s, VL %u",
                     frame.payload_len, g_links[which].name, frame.vl_id);
            return true;
        }
    }
    log_line("no traffic from the unit within %u s", timeout_s);
    return false;
}

static bool send_configuration(raw_socket_t *config_sock, int frame_count, unsigned gap_ms)
{
    for (int i = 0; i < frame_count; i++) {
        if (!raw_socket_send(config_sock, g_frames[i].data, g_frames[i].len)) {
            log_line("configuration frame %d (seq %u) failed to send", i, g_frames[i].seq);
            return false;
        }
        log_file_only("sent seq %u, %u bytes, %s",
                      g_frames[i].seq, g_frames[i].len, g_frames[i].label);
        if (i + 1 < frame_count)
            sleep_ms(gap_ms);
    }
    log_line("configuration sent: %d frames on %s", frame_count, config_sock->name);
    return true;
}

/** Look for the device's answer to the trailing 0x52 status query. */
static bool wait_for_status_reply(size_t link_count, unsigned timeout_ms)
{
    uint64_t deadline = hm_now_ms() + timeout_ms;

    while (hm_now_ms() < deadline) {
        if (safe_shutdown_requested())
            return false;

        size_t which = 0;
        int n = raw_socket_recv_any(g_links, link_count, g_rx, sizeof g_rx, 200, &which);
        if (n <= 0)
            continue;

        hm_frame_t frame;
        if (hm_classify(g_rx, (size_t)n, &frame) && frame.payload_len >= 111) {
            log_line("status reply on %s: %zu bytes, VL %u, source 0x%02x",
                     g_links[which].name, frame.payload_len, frame.vl_id,
                     frame.status_enable);
            return true;
        }
    }
    return false;
}

/* ------------------------------------------------------------------ */

static void monitor_run(size_t link_count, raw_socket_t *config_sock,
                        int frame_count, const timing_config_t *timing,
                        const char *profile_name)
{
    const copper_link_t *copper = app_config_copper(NULL);
    hm_watch_t watch;
    uint64_t   started = hm_now_ms();
    uint64_t   next_draw = started;
    unsigned   interruptions = 0;

    hm_watch_init(&watch);
    watch.alive = true;

    log_line("monitoring - press Ctrl+C to end the test");

    while (!safe_shutdown_requested()) {
        size_t which = 0;
        int n = raw_socket_recv_any(g_links, link_count, g_rx, sizeof g_rx, 100, &which);
        if (n > 0) {
            hm_frame_t frame;
            if (hm_classify(g_rx, (size_t)n, &frame)) {
                hm_watch_saw_frame(&watch);
                vl_watch_saw(&g_watch, copper[which].dtn_port, frame.vl_id, (size_t)n);
            } else {
                vl_watch_unclassified(&g_watch);
            }
        }

        if (hm_watch_update(&watch, timing->heartbeat_timeout_ms)) {
            if (!watch.alive) {
                interruptions++;
                log_line("UNIT WENT QUIET - no traffic for %u ms (event %u)",
                         timing->heartbeat_timeout_ms, interruptions);
            } else {
                /* It rebooted, so its VL table is gone. Put it back. */
                log_line("unit is back - re-sending the configuration");
                if (send_configuration(config_sock, frame_count, timing->frame_gap_ms))
                    log_line("configuration restored after event %u", interruptions);
                else
                    log_line("configuration could NOT be restored after event %u",
                             interruptions);
            }
        }

        uint64_t now = hm_now_ms();
        if (now >= next_draw) {
            next_draw = now + timing->display_interval_ms;
            vl_watch_render(&g_watch, (now - started) / 1000, profile_name,
                            watch.alive, interruptions);
        }
    }

    uint64_t elapsed = (hm_now_ms() - started) / 1000;
    printf("\n");
    log_line("test stopped by the operator");
    log_line("elapsed %llus, %llu frames from the unit, %u interruption(s)",
             (unsigned long long)elapsed, (unsigned long long)watch.frames,
             interruptions);
    vl_watch_log_summary(&g_watch);
}

/* ------------------------------------------------------------------ */

unit_result_t dtn_test_run(void)
{
    const timing_config_t *timing = app_config_timing();
    const copper_link_t   *copper;
    const copper_link_t   *config_link = app_config_config_link();
    size_t link_count;
    int handles[APP_MAX_COPPER_LINKS];
    raw_socket_t *config_sock = NULL;
    unit_result_t result = UNIT_RESULT_ERROR;

    copper = app_config_copper(&link_count);
    for (size_t i = 0; i < link_count; i++) {
        g_links[i].fd = -1;
        handles[i] = -1;
    }

    const vl_profile_t *profile = select_profile();
    if (!profile)
        return UNIT_RESULT_ABORTED;

    int count = vl_profile_expand(profile, g_records, VL_PROFILE_MAX_RECORDS,
                                  app_config_dense_table());
    if (count < 0) {
        puts("Profile does not fit in the VL table.");
        return UNIT_RESULT_ERROR;
    }

    char reason[128];
    if (!vl_profile_validate(g_records, (size_t)count, reason, sizeof reason)) {
        printf("Profile is not usable: %s\n", reason);
        return UNIT_RESULT_ERROR;
    }

    /* Untagged: the workstation is wired straight to the DTN's copper
     * end-system ports, with no bridge in between to steer on a VLAN tag. */
    int frame_count = dtn_build_config_frames(g_records, (size_t)count, -1,
                                              g_frames, DTN_MAX_CONFIG_FRAMES);
    if (frame_count < 0) {
        puts("Could not build the configuration frames.");
        return UNIT_RESULT_ERROR;
    }

    size_t total = 0;
    for (int i = 0; i < frame_count; i++)
        total += g_frames[i].len;

    printf("\n  profile     : %s - %s\n", profile->name, profile->description);
    size_t enabled = vl_profile_enabled_count(g_records, (size_t)count);
    printf("  VL table    : %d records, %zu enabled  (VL %u..%u)\n", count, enabled,
           g_records[0].vl_id, g_records[count - 1].vl_id);
    printf("  frames      : %d, %zu bytes, untagged\n", frame_count, total);
    printf("  config out  : %s (DTN port %u)\n", config_link->iface, config_link->dtn_port);
    printf("  listening   :");
    for (size_t i = 0; i < link_count; i++)
        printf(" %s (DTN port %u)%s", copper[i].iface, copper[i].dtn_port,
               i + 1 < link_count ? "," : "\n");
    print_routing(g_records, (size_t)count);

    if (!prompt_yes_no("\nStart the test", false))
        return UNIT_RESULT_ABORTED;

    for (size_t i = 0; i < link_count; i++) {
        bool carrier = false;
        if (!raw_socket_link_up(copper[i].iface, &carrier)) {
            printf("%s is down. Bring it up first.\n", copper[i].iface);
            return UNIT_RESULT_ERROR;
        }
        if (!carrier)
            printf("Warning: %s has no carrier - is the cable connected?\n",
                   copper[i].iface);
    }

    if (!log_open("DTN", profile->name))
        puts("Warning: could not open a log file; the run will not be recorded.");
    else
        printf("Logging to %s\n\n", log_path());

    for (size_t i = 0; i < link_count; i++) {
        if (!raw_socket_open(&g_links[i], copper[i].iface, true))
            goto done;
        handles[i] = safe_shutdown_register(copper[i].iface, SHUTDOWN_PRIO_SOCKET,
                                            close_socket_action, &g_links[i]);
        if (copper[i].dtn_port == config_link->dtn_port)
            config_sock = &g_links[i];
    }
    if (!config_sock) {
        log_line("no socket for the configuration link %s", config_link->iface);
        goto done;
    }

    vl_watch_init(&g_watch, g_records, (size_t)count);
    log_line("profile %s, %d VL records (%zu enabled), %d frames",
             profile->name, count, vl_profile_enabled_count(g_records, (size_t)count),
             frame_count);

    if (!wait_for_unit(link_count, timing->device_ready_timeout_s)) {
        result = safe_shutdown_requested() ? UNIT_RESULT_ABORTED : UNIT_RESULT_ERROR;
        goto done;
    }

    if (!send_configuration(config_sock, frame_count, timing->frame_gap_ms))
        goto done;

    if (!wait_for_status_reply(link_count, timing->status_reply_timeout_ms))
        log_line("no status reply within %u ms - continuing, but the configuration "
                 "is unconfirmed", timing->status_reply_timeout_ms);

    monitor_run(link_count, config_sock, frame_count, timing, profile->name);
    result = UNIT_RESULT_PASS;

done:
    for (size_t i = 0; i < link_count; i++) {
        safe_shutdown_unregister(handles[i]);
        raw_socket_close(&g_links[i]);
    }
    log_close();
    safe_shutdown_clear();
    return result;
}

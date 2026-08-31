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

#include <stdio.h>
#include <string.h>
#include <time.h>

#define RX_BUFFER_SIZE 2048

static dtn_vl_t    g_records[VL_PROFILE_MAX_RECORDS];
static dtn_frame_t g_frames[DTN_MAX_CONFIG_FRAMES];
static uint8_t     g_rx[RX_BUFFER_SIZE];

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

/* Collapse the record list into "port A -> port B, VL x..y" runs. */
static void print_routing(const dtn_vl_t *records, size_t count)
{
    size_t i = 0;

    puts("\n  routing");
    while (i < count) {
        size_t j = i + 1;
        while (j < count &&
               records[j].src_port  == records[i].src_port &&
               records[j].dest_mask == records[i].dest_mask &&
               records[j].vl_id     == records[j - 1].vl_id + 1)
            j++;

        int dst = -1;
        for (int p = 0; p < DTN_PORT_COUNT; p++)
            if (records[i].dest_mask >> p & 1) {
                dst = p;
                break;
            }

        printf("    %-3s port %2u -> %2d   VL %u", (dst == 32 || dst == 33) ? "HM" : "",
               records[i].src_port, dst, records[i].vl_id);
        if (j - i > 1)
            printf("-%u  (%zu VLs)", records[j - 1].vl_id, j - i);
        putchar('\n');
        i = j;
    }
}

/* ------------------------------------------------------------------ */

/** Wait until the unit's health monitor appears. */
static bool wait_for_unit(raw_socket_t *monitor, unsigned timeout_s)
{
    uint64_t deadline = hm_now_ms() + (uint64_t)timeout_s * 1000u;

    log_line("waiting for the unit on %s (up to %u s)", monitor->name, timeout_s);
    while (hm_now_ms() < deadline) {
        if (safe_shutdown_requested())
            return false;

        int n = raw_socket_recv(monitor, g_rx, sizeof g_rx, 500);
        if (n <= 0)
            continue;

        hm_frame_t frame;
        if (hm_classify(g_rx, (size_t)n, &frame)) {
            log_line("unit is up: health monitor seen (%zu byte packet)", frame.payload_len);
            return true;
        }
    }
    log_line("no health monitor traffic within %u s", timeout_s);
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
static bool wait_for_status_reply(raw_socket_t *monitor, unsigned timeout_ms)
{
    uint64_t deadline = hm_now_ms() + timeout_ms;

    while (hm_now_ms() < deadline) {
        if (safe_shutdown_requested())
            return false;

        int n = raw_socket_recv(monitor, g_rx, sizeof g_rx, 200);
        if (n <= 0)
            continue;

        hm_frame_t frame;
        if (hm_classify(g_rx, (size_t)n, &frame) && frame.payload_len >= 111) {
            log_line("status reply received (%zu bytes, source 0x%02x)",
                     frame.payload_len, frame.status_enable);
            return true;
        }
    }
    return false;
}

/* ------------------------------------------------------------------ */

static void monitor_run(raw_socket_t *monitor, raw_socket_t *config_sock,
                        int frame_count, const net_config_t *net)
{
    hm_watch_t watch;
    uint64_t   started = hm_now_ms();
    uint64_t   next_tick = started + 1000;
    unsigned   power_cycles = 0;

    hm_watch_init(&watch);
    watch.alive = true;

    log_line("monitoring - press Ctrl+C to end the test");

    while (!safe_shutdown_requested()) {
        int n = raw_socket_recv(monitor, g_rx, sizeof g_rx, 200);
        if (n > 0) {
            hm_frame_t frame;
            if (hm_classify(g_rx, (size_t)n, &frame))
                hm_watch_saw_frame(&watch);
        }

        if (hm_watch_update(&watch, net->heartbeat_timeout_ms)) {
            if (!watch.alive) {
                power_cycles++;
                log_line("HEALTH MONITOR LOST - the unit went quiet (event %u)", power_cycles);
            } else {
                /* The unit rebooted, so its VL table is gone. Put it back. */
                log_line("unit is back - re-sending the configuration");
                if (send_configuration(config_sock, frame_count, net->frame_gap_ms))
                    log_line("configuration restored after event %u", power_cycles);
                else
                    log_line("configuration could NOT be restored after event %u", power_cycles);
            }
        }

        uint64_t now = hm_now_ms();
        if (now >= next_tick) {
            next_tick = now + 1000;
            log_file_only("elapsed %llus  frames %llu  state %s",
                          (unsigned long long)((now - started) / 1000),
                          (unsigned long long)watch.frames,
                          watch.alive ? "alive" : "lost");
        }
    }

    uint64_t elapsed = (hm_now_ms() - started) / 1000;
    log_line("test stopped by the operator");
    log_line("elapsed %llus, %llu health-monitor frames, %u power interruption(s)",
             (unsigned long long)elapsed, (unsigned long long)watch.frames, power_cycles);
}

/* ------------------------------------------------------------------ */

unit_result_t dtn_test_run(void)
{
    const net_config_t *net = app_config_net();
    raw_socket_t monitor = {.fd = -1}, config_sock = {.fd = -1};
    int monitor_handle = -1, config_handle = -1;
    unit_result_t result = UNIT_RESULT_ERROR;

    const vl_profile_t *profile = select_profile();
    if (!profile)
        return UNIT_RESULT_ABORTED;

    int count = vl_profile_expand(profile, g_records, VL_PROFILE_MAX_RECORDS);
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
    printf("  VL records  : %d  (VL %u..%u)\n", count,
           g_records[0].vl_id, g_records[count - 1].vl_id);
    printf("  frames      : %d, %zu bytes, untagged\n", frame_count, total);
    printf("  config out  : %s\n", net->config_interface);
    printf("  monitor in  : %s\n", net->monitor_interface);
    print_routing(g_records, (size_t)count);

    if (!prompt_yes_no("\nStart the test", false))
        return UNIT_RESULT_ABORTED;

    bool carrier = false;
    if (!raw_socket_link_up(net->config_interface, &carrier)) {
        printf("%s is down. Bring it up first.\n", net->config_interface);
        return UNIT_RESULT_ERROR;
    }
    if (!carrier)
        printf("Warning: %s has no carrier - is the cable connected?\n",
               net->config_interface);

    if (!log_open("DTN", profile->name))
        puts("Warning: could not open a log file; the run will not be recorded.");
    else
        printf("Logging to %s\n\n", log_path());

    if (!raw_socket_open(&config_sock, net->config_interface, false))
        goto done;
    config_handle = safe_shutdown_register("config socket", SHUTDOWN_PRIO_SOCKET,
                                           close_socket_action, &config_sock);

    if (strcmp(net->monitor_interface, net->config_interface) == 0) {
        /* One link carries both directions; a second socket on it would only
         * duplicate what we already receive. */
        if (!raw_socket_open(&monitor, net->monitor_interface, true))
            goto done;
    } else if (!raw_socket_open(&monitor, net->monitor_interface, true)) {
        goto done;
    }
    monitor_handle = safe_shutdown_register("monitor socket", SHUTDOWN_PRIO_SOCKET,
                                            close_socket_action, &monitor);

    log_line("profile %s, %d VL records, %d frames", profile->name, count, frame_count);

    if (!wait_for_unit(&monitor, net->device_ready_timeout_s)) {
        result = safe_shutdown_requested() ? UNIT_RESULT_ABORTED : UNIT_RESULT_ERROR;
        goto done;
    }

    if (!send_configuration(&config_sock, frame_count, net->frame_gap_ms))
        goto done;

    if (!wait_for_status_reply(&monitor, net->status_reply_timeout_ms))
        log_line("no status reply within %u ms - continuing, but the configuration "
                 "is unconfirmed", net->status_reply_timeout_ms);

    monitor_run(&monitor, &config_sock, frame_count, net);
    result = UNIT_RESULT_PASS;

done:
    safe_shutdown_unregister(monitor_handle);
    safe_shutdown_unregister(config_handle);
    raw_socket_close(&monitor);
    raw_socket_close(&config_sock);
    log_close();
    safe_shutdown_clear();
    return result;
}

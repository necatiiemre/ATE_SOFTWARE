/*
 * DTN acceleration test.
 *
 * The operator picks one of the three rounds; the profile becomes a VL table,
 * the VL table becomes configuration frames, and those go out of the copper
 * link. From then on the test watches the health-monitor stream until the
 * operator stops it.
 *
 * A round may also carry copper legs - config1 does - and then the test
 * generates traffic as well as watching it: PRBS out of a copper port, through
 * the DTN to the VMC, and back the same way. See DtnLegs.h.
 *
 * The unit is powered separately. What the test does care about is power
 * *dropping* mid-run: on a vibration rig that is a likely fault and probably
 * the most valuable thing the run can catch. When the health monitor goes
 * quiet and comes back, the DTN has rebooted and lost its configuration, so the
 * test re-sends it and records both the loss and the recovery.
 */

#include "DtnTest.h"

#include "AppConfig.h"
#include "AteHealth.h"
#include "DtnConfig.h"
#include "DtnLegs.h"
#include "HealthDecode.h"
#include "DtnHealthFrame.h"
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
static hd_state_t   g_health;
static dtn_legs_t  *g_legs;
static prbs31_cache_t g_prbs;
static volatile bool g_legs_stop;

static void sleep_ms(unsigned ms)
{
    struct timespec ts = {.tv_sec = ms / 1000, .tv_nsec = (long)(ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

static void prbs_progress(size_t done, size_t total)
{
    printf("\r  PRBS-31: %zu of %zu MB", done / (1024 * 1024), total / (1024 * 1024));
    fflush(stdout);
}

static void stop_legs_action(void *ctx)
{
    (void)ctx;
    g_legs_stop = true;
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

/* Show the routing in groups: the fibre links under test, the taps that bring
 * the VMC's health monitor out to copper, the copper legs of the
 * workstation - DTN - VMC path, and the DTN's own management path. Consecutive
 * VLs with the same endpoints collapse into a run.
 *
 * Which group a record belongs to is asked of the profile, not guessed from its
 * ports: a copper leg's return run is fibre in and copper out, which is exactly
 * what a tap looks like, and the two would be shown as one thing. */
static const vl_profile_t *g_display_profile;

static int first_destination(const dtn_vl_t *record)
{
    for (int p = 0; p < DTN_PORT_COUNT; p++)
        if (record->dest_mask >> p & 1)
            return p;
    return -1;
}

static vl_kind_t kind_of(const dtn_vl_t *record)
{
    if (!g_display_profile)
        return VL_KIND_UNKNOWN;
    return vl_profile_kind_of(g_display_profile, record->vl_id);
}

static bool is_management(const dtn_vl_t *record)
{
    return kind_of(record) == VL_KIND_MANAGEMENT ||
           kind_of(record) == VL_KIND_DTN_HM;
}

static bool is_hm_tap(const dtn_vl_t *record)
{
    return kind_of(record) == VL_KIND_HM_TAP;
}

static bool is_comm(const dtn_vl_t *record)
{
    return kind_of(record) == VL_KIND_COMM;
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
        int fanout = __builtin_popcountll(records[i].dest_mask);
        if (fanout > 1)
            printf("    port %2u -> %2d ports   VL %u", records[i].src_port, fanout,
                   records[i].vl_id);
        else
            printf("    port %2u -> %2d         VL %u", records[i].src_port,
                   first_destination(&records[i]), records[i].vl_id);
        if (j - i > 1)
            printf("-%u  (%zu VLs)", records[j - 1].vl_id, j - i);
        putchar('\n');
        i = j;
    }
}

static bool is_fibre_link(const dtn_vl_t *record)
{
    return kind_of(record) == VL_KIND_FIBRE ||
           kind_of(record) == VL_KIND_UNKNOWN;
}

static void print_routing(const vl_profile_t *profile,
                          const dtn_vl_t *records, size_t count)
{
    g_display_profile = profile;

    print_group("fibre links under test", records, count, is_fibre_link, true);
    print_group("health-monitor taps (VMC -> copper)",
                records, count, is_hm_tap, true);
    print_group("copper legs (workstation -> DTN -> VMC and back)",
                records, count, is_comm, true);
    print_group("DTN management path (its own health monitor and status replies)",
                records, count, is_management, false);
}

/* ------------------------------------------------------------------ */
/* The health data describes all 35 ports whichever round is running, but a
 * round uses twelve of them for links, two for taps and the copper pair for
 * management. A counter only means something next to what the port is supposed
 * to be carrying, so the table is split the same way the routing is. */

static hd_port_ref_t g_link_ports[DTN_PORT_COUNT];
static hd_port_ref_t g_tap_ports[VL_PROFILE_MAX_HM];
static hd_port_ref_t g_copper_ports[APP_MAX_COPPER_LINKS + 1];
static hd_port_ref_t g_other_ports[DTN_PORT_COUNT];
static char          g_notes[DTN_PORT_COUNT][16];
static hd_group_t    g_groups[4];
static size_t        g_group_count;


static void collect_ports(const dtn_vl_t *records, size_t count)
{
    /* Per fibre port: who it sends to, who it hears from. Every link in a round
     * is one port to one port, so one of each is enough. */
    int sends_to[DTN_PORT_COUNT], hears_from[DTN_PORT_COUNT];
    size_t links = 0, taps = 0, copper = 0, others = 0;

    for (int p = 0; p < DTN_PORT_COUNT; p++)
        sends_to[p] = hears_from[p] = -1;

    for (size_t i = 0; i < count; i++) {
        const dtn_vl_t *r = &records[i];
        int dst = first_destination(r);

        if (!dtn_vl_enabled(r) || dst < 0 || r->src_port >= DTN_PORT_COUNT)
            continue;
        if (r->src_port == DTN_PORT_MANAGEMENT || dst >= 32)
            continue;                       /* taps and management, handled below */
        sends_to[r->src_port] = dst;
        hears_from[dst] = r->src_port;
    }

    for (int p = 0; p < DTN_PORT_COUNT && links < DTN_PORT_COUNT; p++) {
        if (sends_to[p] < 0 && hears_from[p] < 0)
            continue;
        if (sends_to[p] >= 0 && hears_from[p] >= 0)
            snprintf(g_notes[p], sizeof g_notes[p], "<-> %d", sends_to[p]);
        else if (sends_to[p] >= 0)
            snprintf(g_notes[p], sizeof g_notes[p], " -> %d", sends_to[p]);
        else
            snprintf(g_notes[p], sizeof g_notes[p], " <- %d", hears_from[p]);
        g_link_ports[links].port = (uint8_t)p;
        g_link_ports[links].note = g_notes[p];
        links++;
    }

    /* The taps, and the DTN's own health monitor out of the management port. */
    for (size_t i = 0; i < count && taps < VL_PROFILE_MAX_HM; i++) {
        const dtn_vl_t *r = &records[i];
        int dst = first_destination(r);

        if (!dtn_vl_enabled(r) || dst < 32 || r->src_port >= DTN_PORT_COUNT)
            continue;
        if (r->src_port == DTN_PORT_MANAGEMENT)
            continue;
        snprintf(g_notes[r->src_port], sizeof g_notes[r->src_port],
                 "VL %u -> %d", r->vl_id, dst);
        g_tap_ports[taps].port = r->src_port;
        g_tap_ports[taps].note = g_notes[r->src_port];
        taps++;
    }

    const copper_link_t *links_cfg;
    size_t link_count;
    links_cfg = app_config_copper(&link_count);
    for (size_t i = 0; i < link_count; i++) {
        uint8_t p = links_cfg[i].dtn_port;
        snprintf(g_notes[p], sizeof g_notes[p], "%s", links_cfg[i].speed);
        g_copper_ports[copper].port = p;
        g_copper_ports[copper].note = g_notes[p];
        copper++;
    }
    snprintf(g_notes[DTN_PORT_MANAGEMENT], sizeof g_notes[DTN_PORT_MANAGEMENT],
             "VL %u -> %d", DTN_HEALTH_MONITOR_VL, DTN_HEALTH_MONITOR_PORT);
    g_copper_ports[copper].port = DTN_PORT_MANAGEMENT;
    g_copper_ports[copper].note = g_notes[DTN_PORT_MANAGEMENT];
    copper++;

    g_group_count = 0;
    g_groups[g_group_count++] = (hd_group_t){"fibre links under test", g_link_ports, links};
    g_groups[g_group_count++] = (hd_group_t){"health-monitor taps (fibre-side unit)",
                                             g_tap_ports, taps};
    g_groups[g_group_count++] = (hd_group_t){"copper end system and management",
                                             g_copper_ports, copper};

    if (!app_config_all_ports())
        return;

    /* Everything the round does not use, so a port the device does not report
     * at all is visible - which is a question a round that will not take
     * raises, and the health data answers whichever round is running. */
    for (int p = 0; p < DTN_PORT_COUNT; p++) {
        bool listed = false;
        for (size_t i = 0; i < links && !listed; i++)
            listed = g_link_ports[i].port == p;
        for (size_t i = 0; i < taps && !listed; i++)
            listed = g_tap_ports[i].port == p;
        for (size_t i = 0; i < copper && !listed; i++)
            listed = g_copper_ports[i].port == p;
        if (!listed)
            g_other_ports[others++] = (hd_port_ref_t){(uint8_t)p, NULL};
    }
    g_groups[g_group_count++] = (hd_group_t){"not used by this round",
                                             g_other_ports, others};
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

/**
 * Let the unit settle before configuring it.
 *
 * The health monitor appearing on copper says the DTN is alive; it does not say
 * it has finished coming up. The operator powers it by hand, so the test waits
 * a fixed while after that first packet rather than racing the end of the boot -
 * a configuration written into a device still starting is a configuration that
 * may not stick, and the failure looks like a device that ignored it.
 *
 * The wait keeps reading: the links carry the unit's health monitor throughout,
 * and letting it pile up in the socket buffer would only mean stale frames to
 * work through afterwards. It also counts what arrived, so a unit that says one
 * thing and then goes quiet again is visible before the configuration goes out
 * rather than after.
 *
 * @return false if the operator asked to stop during the wait
 */
static bool settle_before_config(size_t link_count, unsigned seconds)
{
    if (seconds == 0)
        return true;

    const uint64_t until = hm_now_ms() + (uint64_t)seconds * 1000u;
    uint64_t frames = 0;
    unsigned last_report = 0;

    log_line("letting the unit settle for %u s before configuring it", seconds);

    while (hm_now_ms() < until) {
        if (safe_shutdown_requested())
            return false;

        size_t which = 0;
        int n = raw_socket_recv_any(g_links, link_count, g_rx, sizeof g_rx, 200, &which);

        while (n > 0) {
            frames++;
            n = raw_socket_recv_nowait(&g_links[which], g_rx, sizeof g_rx);
        }

        /* A line a second, so a ten-second silence on the terminal is not
         * mistaken for the program having stopped. */
        const unsigned left = (unsigned)((until - hm_now_ms() + 999) / 1000);
        if (left != last_report) {
            last_report = left;
            printf("\r  settling: %2u s left, %llu frame(s) from the unit   ",
                   left, (unsigned long long)frames);
            fflush(stdout);
        }
    }
    putchar('\n');
    log_line("settled: %llu frame(s) from the unit in %u s",
             (unsigned long long)frames, seconds);
    return true;
}

static bool send_configuration(raw_socket_t *config_sock, int frame_count, unsigned gap_ms)
{
    /* Named on the terminal, not only in the log, and named before the frames go
     * rather than after: this is the line that says which interface to point a
     * capture at, and it is no use once the frames have already gone past. */
    log_line("configuring over %s now: %d frames, %u ms apart",
             config_sock->name, frame_count, gap_ms);

    for (int i = 0; i < frame_count; i++) {
        if (!raw_socket_send(config_sock, g_frames[i].data, g_frames[i].len)) {
            log_line("configuration frame %d (seq %u) failed to send", i, g_frames[i].seq);
            return false;
        }
        log_line("sent seq %u, %u bytes, %s",
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

/**
 * @brief Say it out loud when the DTN is throwing our traffic away.
 *
 * The device counts, per ingress port, every frame whose VL its table does not
 * define. On a copper port that counter can only be us: nothing else sends there.
 * So a copper port whose undefined-VL count keeps climbing means the switch table
 * the device is using is not the one this test wrote - and nothing else on the
 * screen says so. The loss column fills up, the legs report bad frames, the VL
 * watch shows the return VLs idle, and every one of those readings is what a
 * *wrong* VL table looks like too. One line here is the difference between that
 * and a rig session spent reading the tables.
 *
 * @param last per-link snapshot of the counter, so this only speaks up while the
 *        count is still growing
 */
static void warn_if_table_not_taken(size_t link_count, uint64_t *last)
{
    const copper_link_t *copper = app_config_copper(NULL);

    for (size_t i = 0; i < link_count; i++) {
        const uint8_t  port  = copper[i].dtn_port;
        const uint64_t drops = port < HD_MAX_PORTS ? g_health.ports[port].vlid_drop : 0;

        /* A handful can be a frame in flight while the table was being written;
         * a thousand and still counting cannot. */
        if (drops > last[i] && drops > 1000)
            printf("\n  ! DTN port %u has dropped %llu frame(s) as an undefined VL.\n"
                   "    The device is not using the VL table this test wrote. Check that\n"
                   "    the configuration went out of a cable that is plugged in (see\n"
                   "    \"config out\" above), then power the DTN off and on and retry.\n",
                   port, (unsigned long long)drops);
        last[i] = drops;
    }
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
    uint64_t   next_query = started;
    const bool poll_health = app_config_dtn_health_poll();
    unsigned   interruptions = 0;
    uint64_t   undef_seen[APP_MAX_COPPER_LINKS] = {0};

    hm_watch_init(&watch);
    watch.alive = true;

    log_line("monitoring - press Ctrl+C to end the test");

    while (!safe_shutdown_requested()) {
        size_t which = 0;
        int n = raw_socket_recv_any(g_links, link_count, g_rx, sizeof g_rx, 100, &which);

        /* Poll once, then drain. With the copper legs running this link carries
         * thousands of frames a second, and a poll for each of them would be half
         * the work this loop does. */
        while (n > 0) {
            const uint8_t port = copper[which].dtn_port;

            /* The legs first. Their frames are the bulk of the traffic and are
             * nothing to do with the health monitor, so they leave here rather
             * than being classified and then discarded. */
            if (g_legs && dtn_legs_ingest(g_legs, port, g_rx, (size_t)n)) {
                hm_watch_saw_frame(&watch);
                vl_watch_saw(&g_watch, port,
                             (uint16_t)((g_rx[4] << 8) | g_rx[5]), (size_t)n);
            } else {
                hm_frame_t frame;
                if (hm_classify(g_rx, (size_t)n, &frame)) {
                    hm_watch_saw_frame(&watch);
                    vl_watch_saw(&g_watch, port, frame.vl_id, (size_t)n);
                    hd_ingest(&g_health, frame.payload, frame.payload_len);

                    /* The ATE software's parser, which is the copy in dtn/ate/,
                     * is given the whole frame: it tells the six health packets
                     * apart by total length. Its own VL filter is not used - it
                     * tests for 0x1188 and this device answers on VL 38 - so the
                     * same check is made here against the VL we configured. */
                    if (frame.vl_id == DTN_HEALTH_MONITOR_VL)
                        ate_health_ingest(g_rx, (size_t)n);
                } else {
                    vl_watch_unclassified(&g_watch);
                }
            }
            n = raw_socket_recv_nowait(&g_links[which], g_rx, sizeof g_rx);
        }

        if (hm_watch_update(&watch, timing->heartbeat_timeout_ms)) {
            if (!watch.alive) {
                interruptions++;
                log_line("UNIT WENT QUIET - no traffic for %u ms (event %u)",
                         timing->heartbeat_timeout_ms, interruptions);
                /* Stop generating: the device has no VL table to carry it, and
                 * the configuration that is about to go out wants a quiet wire. */
                dtn_legs_pause(g_legs, true);
            } else {
                /* It rebooted, so its VL table is gone. Put it back - after the
                 * same settle the first configuration waits through, because it
                 * is the same situation: talking is not the same as ready. */
                log_line("unit is back");
                settle_before_config(link_count, timing->config_settle_s);
                log_line("re-sending the configuration");
                if (send_configuration(config_sock, frame_count, timing->frame_gap_ms))
                    log_line("configuration restored after event %u", interruptions);
                else
                    log_line("configuration could NOT be restored after event %u",
                             interruptions);
                dtn_legs_pause(g_legs, false);
            }
        }

        uint64_t now = hm_now_ms();

        /* The cycle boundary, on the ATE software's one-second beat, so the block
         * that gets printed covers the same second theirs does.
         *
         * Whether a query goes out with it is another matter. The ATE software
         * polls, and the poll is a VL 0 frame because that is how a management
         * frame is addressed - but VL 0 is a VL the VMC uses, so one of those a
         * second for the length of a vibration run is a frame turning up at a unit
         * that means something else by it. The DTN streams its health monitor
         * unprompted, so leaving the query off costs nothing. See
         * app_config_dtn_health_poll. */
        if (now >= next_query) {
            next_query = now + ATE_HEALTH_QUERY_INTERVAL_MS;
            ate_health_cycle();

            if (poll_health) {
                uint8_t query[ATE_HEALTH_QUERY_MAX];
                int qlen = ate_health_build_query(query, sizeof query);

                if (qlen > 0 && !raw_socket_send(config_sock, query, (size_t)qlen))
                    log_line("health query could not be sent on %s",
                             config_sock->name);
            }
        }

        if (now >= next_draw) {
            next_draw = now + timing->display_interval_ms;

            /* The ATE software's health block first, then this test's own
             * tables. The block is tall - both FPGAs, all 35 ports, the MCU - so
             * on a terminal that cannot hold everything, what stays on screen is
             * the bottom: the VL watch, the per-port summary and the legs, which
             * are what is read while the rig runs. The block is in the log either
             * way, because the log is a tee of stdout. */
            printf("\033[H\033[2J");
            ate_health_render();
            vl_watch_render(&g_watch, (now - started) / 1000, profile_name,
                            watch.alive, interruptions);
            hd_render(&g_health, g_groups, g_group_count);
            dtn_legs_print_table(g_legs);
            warn_if_table_not_taken(link_count, undef_seen);
            puts("\nCtrl+C to end the test");
            fflush(stdout);
        }
    }

    uint64_t elapsed = (hm_now_ms() - started) / 1000;
    printf("\n");
    log_line("test stopped by the operator");
    log_line("elapsed %llus, %llu frames from the unit, %u interruption(s)",
             (unsigned long long)elapsed, (unsigned long long)watch.frames,
             interruptions);
    /* The device's counters are absolute, so the two readings are what say what
     * happened during this run: the first cycle of the test beside the last. */
    ate_health_render_start_end();

    uint64_t queries = 0, short_cycles = 0;
    ate_health_counts(&queries, &short_cycles);
    log_line("health monitor: %llu queries, last cycle %u/%d packets, "
             "%llu cycle(s) came back short",
             (unsigned long long)queries, ate_health_responses(),
             ATE_HEALTH_EXPECTED_RESPONSES,
             (unsigned long long)short_cycles);
    vl_watch_log_summary(&g_watch);
    hd_log_summary(&g_health);
    dtn_legs_log_summary(g_legs);
}

/* ------------------------------------------------------------------ */

/**
 * @brief Report on the copper links and pick the one the configuration goes out
 *        of.
 *
 * app_config names the 100M link, because that is the proven management path -
 * where the reference status query goes and where the main ATE software polls the
 * device. But a configuration sent down a cable that is not plugged in is sent
 * nowhere, and nothing later in the run says so: a raw socket accepts the frames,
 * and the DTN carries on with whatever configuration it already had, health
 * monitor and all. The test then looks alive while the switch table it was told
 * to load never arrived - and the symptom is every frame we send counted as an
 * undefined VL, which is exactly what a *wrong* VL table looks like. One
 * unplugged cable is a rig session spent reading the wrong tables.
 *
 * So the link is chosen from the ones that actually have a carrier: the preferred
 * one when it is plugged in, otherwise whichever is. All of them dark is an
 * error - there is no way in to the device.
 *
 * @return the link to configure through, or NULL if none is usable
 */
static const copper_link_t *choose_config_link(const copper_link_t *copper,
                                               size_t link_count,
                                               const copper_link_t *preferred)
{
    const copper_link_t *chosen = NULL;

    puts("\n  copper links:");
    for (size_t i = 0; i < link_count; i++) {
        bool carrier = false;
        bool up = raw_socket_link_up(copper[i].iface, &carrier);

        printf("    %-10s DTN port %2u  %-5s %s\n", copper[i].iface,
               copper[i].dtn_port, copper[i].speed,
               !up ? "DOWN - ip link set up" : carrier ? "connected" : "no carrier");
        if (!up || !carrier)
            continue;
        if (copper[i].dtn_port == preferred->dtn_port)
            chosen = &copper[i];
        else if (!chosen)
            chosen = &copper[i];
    }
    return chosen;
}


unit_result_t dtn_test_run(void)
{
    const timing_config_t *timing = app_config_timing();
    const copper_link_t   *copper;
    const copper_link_t   *config_link;
    size_t link_count;
    int handles[APP_MAX_COPPER_LINKS];
    int legs_handle = -1;
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

    vl_profile_t round = *profile;
    round.management = app_config_management_vls();

    int count = vl_profile_expand(&round, g_records, VL_PROFILE_MAX_RECORDS);
    if (count < 0) {
        puts("Profile does not fit in the VL table.");
        return UNIT_RESULT_ERROR;
    }

    char reason[128];
    if (!vl_profile_validate(g_records, (size_t)count, reason, sizeof reason)) {
        printf("Profile is not usable: %s\n", reason);
        return UNIT_RESULT_ERROR;
    }

    config_link = choose_config_link(copper, link_count, app_config_config_link());
    if (!config_link) {
        puts("\nNo copper link is connected. The configuration has to reach the DTN\n"
             "over one of them, so there is nothing to run until a cable is in.");
        return UNIT_RESULT_ERROR;
    }
    if (config_link->dtn_port != app_config_config_link()->dtn_port)
        printf("    -> configuring over %s instead of %s, which is not connected\n",
               config_link->iface, app_config_config_link()->iface);

    /* Untagged: the workstation is wired straight to the DTN's copper
     * end-system ports, with no bridge in between to steer on a VLAN tag. */
    size_t protocol_len;
    const uint8_t *protocol_block = vl_profile_protocol_block(&protocol_len);
    int frame_count = dtn_build_config_frames(g_records, (size_t)count,
                                              protocol_block, protocol_len, -1,
                                              &DTN_CONFIG_DEFAULT,
                                              g_frames, DTN_MAX_CONFIG_FRAMES);
    if (frame_count < 0) {
        puts("Could not build the configuration frames.");
        return UNIT_RESULT_ERROR;
    }

    size_t total = 0;
    for (int i = 0; i < frame_count; i++)
        total += g_frames[i].len;

    /* Open the log before anything is printed about the run: the log is a
     * transcript of the terminal, so what it holds is decided by when it opens,
     * and the routing table below is the record of what the device was told. */
    if (!log_open("DTN", profile->name))
        puts("Warning: could not open a log file; the run will not be recorded.");
    else
        printf("Logging to %s\n", log_path());

    printf("\n  profile     : %s - %s\n", profile->name, profile->description);
    size_t enabled = vl_profile_enabled_count(g_records, (size_t)count);
    printf("  VL table    : %d records, %zu enabled%s\n", count, enabled,
           round.management ? "  (round + DTN management VLs)" : "  (round only)");
    printf("  frames      : %d, %zu bytes, untagged\n", frame_count, total);
    printf("  config out  : %s (DTN port %u, %s)\n", config_link->iface,
           config_link->dtn_port, config_link->speed);
    printf("  health mon  : the ATE software's, reading VL %u%s\n",
           DTN_HEALTH_MONITOR_VL,
           app_config_dtn_health_poll()
               ? " - polling once a second with a VL 0 query"
               : " - listening only, nothing is sent on VL 0 during the run");
    printf("  to capture   : tcpdump -i %s -nn -s0 'udp port 100'\n",
           config_link->iface);
    printf("  power       : switch the DTN on by hand; the test waits for its\n"
           "                health monitor, then %u s more before configuring\n",
           timing->config_settle_s);
    printf("  listening   :");
    for (size_t i = 0; i < link_count; i++)
        printf(" %s (DTN port %u)%s", copper[i].iface, copper[i].dtn_port,
               i + 1 < link_count ? "," : "\n");
    print_routing(&round, g_records, (size_t)count);

    if (!prompt_yes_no("\nStart the test", false)) {
        log_close();
        return UNIT_RESULT_ABORTED;
    }

    for (size_t i = 0; i < link_count; i++) {
        if (!raw_socket_open(&g_links[i], copper[i].iface, true))
            goto done;
        handles[i] = safe_shutdown_register(copper[i].iface, SHUTDOWN_PRIO_SOCKET,
                                            close_socket_action, &g_links[i]);

        /* These links both send and receive - the configuration, and the copper
         * legs' traffic - and a packet socket opened for every protocol is handed
         * outgoing frames as well as incoming ones. Without this the monitor loop
         * would be given back every frame this end sends: thousands a second on
         * an outbound VL, which the legs would not claim and the health-monitor
         * decoder would then be asked to make sense of. */
        if (!raw_socket_ignore_outgoing(&g_links[i]))
            log_line("%s: the kernel will keep showing us our own frames; "
                     "expect them in the unclassified count", copper[i].iface);

        /* Room for a scheduling hiccup. With the copper legs running each link
         * carries thousands of frames a second, and the default buffer is a few
         * hundred kilobytes - a pause on the reader would otherwise look exactly
         * like loss on the unit's side. */
        raw_socket_set_buffers(&g_links[i], 16 * 1024 * 1024, 4 * 1024 * 1024,
                              NULL, NULL);

        if (copper[i].dtn_port == config_link->dtn_port)
            config_sock = &g_links[i];
    }
    if (!config_sock) {
        log_line("no socket for the configuration link %s", config_link->iface);
        goto done;
    }

    vl_watch_init(&g_watch, g_records, (size_t)count);
    hd_init(&g_health);
    ate_health_reset();
    collect_ports(g_records, (size_t)count);

    /* The copper legs, if this round has any. The PRBS stream is generated once,
     * here, and read out of at an offset every sequence number decides - never
     * per frame. */
    if (round.comm_count > 0) {
        const dtn_leg_config_t *leg_cfg = app_config_dtn_legs();

        printf("\nGenerating the PRBS-31 stream (%zu MB) - this takes a moment...\n",
               PRBS31_CACHE_SIZE / (1024 * 1024));
        if (!prbs31_cache_init(&g_prbs, PRBS31_INITIAL_STATE,
                               dtn_legs_prbs_stride(leg_cfg), prbs_progress)) {
            printf("\nCould not allocate the PRBS stream.\n");
            goto done;
        }
        printf("\r  PRBS-31: ready (%zu MB, %zu bytes per frame)          \n",
               PRBS31_CACHE_SIZE / (1024 * 1024), g_prbs.stride);

        g_legs_stop = false;
        g_legs = dtn_legs_create(&round, leg_cfg, &g_prbs, &g_legs_stop);
        if (!g_legs) {
            log_line("this round declares copper legs but none of them could be "
                     "paired up - no traffic will be generated");
        } else {
            legs_handle = safe_shutdown_register("copper legs", SHUTDOWN_PRIO_SOCKET,
                                                stop_legs_action, NULL);
            for (size_t i = 0; i < link_count; i++)
                dtn_legs_bind(g_legs, copper[i].dtn_port, &g_links[i]);
        }
    }

    log_line("profile %s, %d VL records (%zu enabled), %d frames",
             profile->name, count, vl_profile_enabled_count(g_records, (size_t)count),
             frame_count);
    log_line("configuration goes out of %s (DTN port %u, %s)", config_link->iface,
             config_link->dtn_port, config_link->speed);

    if (wait_for_unit(link_count, timing->device_ready_timeout_s)) {
        if (!settle_before_config(link_count, timing->config_settle_s)) {
            result = UNIT_RESULT_ABORTED;
            goto done;
        }
    } else {
        /* The unit said nothing. Normally that means it is not powered and there
         * is nothing to configure - but it also means this is the one situation in
         * which the configuration never goes out at all, and a run that ends
         * without a single frame on the wire is impossible to tell from one where
         * the frames went somewhere unexpected. So it is offered rather than
         * decided: sending into silence is how the configuration path itself gets
         * looked at, with a capture running and the device's answer unknown. */
        if (safe_shutdown_requested()) {
            result = UNIT_RESULT_ABORTED;
            goto done;
        }
        puts("\nThe unit has not said anything, so there is nothing to configure -\n"
             "unless what you want to see is the configuration itself going out.");
        if (!prompt_yes_no("Send it anyway", false)) {
            result = UNIT_RESULT_ERROR;
            goto done;
        }
        log_line("sending the configuration into silence at the operator's request");
    }

    if (!send_configuration(config_sock, frame_count, timing->frame_gap_ms))
        goto done;

    if (!wait_for_status_reply(link_count, timing->status_reply_timeout_ms))
        log_line("no status reply within %u ms - continuing, but the configuration "
                 "is unconfirmed", timing->status_reply_timeout_ms);

    /* The senders start only now: before the VL table is in the device there is
     * nothing to carry the traffic, and the configuration wants a quiet wire. */
    if (g_legs && !dtn_legs_start(g_legs)) {
        log_line("the copper legs would not start");
        goto done;
    }

    monitor_run(link_count, config_sock, frame_count, timing, profile->name);
    result = UNIT_RESULT_PASS;

done:
    /* The senders go first: they hold the sockets the loop below closes. */
    g_legs_stop = true;
    dtn_legs_destroy(g_legs);
    g_legs = NULL;
    safe_shutdown_unregister(legs_handle);
    prbs31_cache_free(&g_prbs);

    for (size_t i = 0; i < link_count; i++) {
        safe_shutdown_unregister(handles[i]);
        raw_socket_close(&g_links[i]);
    }
    log_close();
    safe_shutdown_clear();
    return result;
}

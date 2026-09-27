#define _GNU_SOURCE             /* clock_nanosleep with TIMER_ABSTIME */

#include "CmcDataPlane.h"

#include "PayloadVerify.h"
#include "SplitmixVerify.h"
#include "Log.h"
#include "RawSocket.h"

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* A quarter of a second of traffic at the configured rate, give or take. The
 * data plane runs at tens of thousands of frames a second and the default
 * socket buffers are a few hundred kilobytes, so a scheduling hiccup on a
 * receiver would otherwise turn into a drop that looks exactly like loss on the
 * unit's side. */
#define CMC_SOCK_RCVBUF (16 * 1024 * 1024)
#define CMC_SOCK_SNDBUF (4 * 1024 * 1024)

/* Below this, waiting is spun rather than slept: a sleep that short costs more
 * in wake-up jitter than it saves in idle. */
#define CMC_SPIN_THRESHOLD_NS 60000

typedef struct {
    uint64_t max_seq;
    uint64_t expected_seq;
    uint64_t pkt_count;
    int      initialized;
} cmc_vl_tracker_t;

struct cmc_data_plane {
    const cmc_config_t     *config;
    const prbs31_cache_t *prbs;
    cmc_sink_t              sink;
    volatile bool          *stop;

    raw_socket_t     sock[APP_MAX_CMC_NETS];
    bool             opened[APP_MAX_CMC_NETS];
    cmc_net_stats_t  stats[APP_MAX_CMC_NETS];

    /* One tracker table per network: the two carry the same VL ids with the
     * same sequences, so a shared table would read the twin as a duplicate. */
    cmc_vl_tracker_t *tracker[APP_MAX_CMC_NETS];

    /* One sequence per VL id, shared by the networks - the twins carry the
     * same one, which is what makes them comparable. */
    uint64_t *tx_seq;

    size_t   payload_off;
    size_t   frame_len;

    pthread_t rx_thread[APP_MAX_CMC_NETS];
    pthread_t tx_thread;
    bool      rx_running[APP_MAX_CMC_NETS];
    bool      tx_running;

    volatile bool tx_paused;
    volatile bool own_stop;      /**< used when the caller passed no flag */
    bool          b_skipped_said;/**< the "A refused so B is skipped" line, once */
};

/* ------------------------------------------------------------------ */
/* Time                                                               */
/* ------------------------------------------------------------------ */

static uint64_t now_ns(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

/* Wait until @p target. The reference busy-waits the whole gap, which is free
 * on a dedicated DPDK lcore and rude on a shared one, so the bulk of the wait
 * is slept and only the last stretch is spun. The pacing this produces is the
 * reference's: one frame per slot, and a slot that has already passed is not
 * made up for, so falling behind never turns into a burst. */
static void wait_until(uint64_t target)
{
    for (;;) {
        const uint64_t now = now_ns();

        if (now >= target)
            return;
        if (target - now > CMC_SPIN_THRESHOLD_NS) {
            struct timespec ts = {
                .tv_sec  = (time_t)(target / 1000000000ull),
                .tv_nsec = (long)(target % 1000000000ull),
            };

            /* Absolute, so a late wake-up does not compound. */
            if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL) == EINTR)
                return;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Lifecycle                                                          */
/* ------------------------------------------------------------------ */

cmc_data_plane_t *cmc_data_plane_create(const cmc_config_t *config,
                                        const prbs31_cache_t *prbs,
                                        const cmc_sink_t *sink,
                                        volatile bool *stop)
{
    if (!config || config->net_count == 0 || config->net_count > APP_MAX_CMC_NETS)
        return NULL;

    cmc_data_plane_t *dp = calloc(1, sizeof *dp);
    if (!dp)
        return NULL;

    dp->config = config;
    dp->prbs   = prbs;
    if (sink)
        dp->sink = *sink;
    dp->stop = stop ? stop : &dp->own_stop;

    dp->payload_off = CMC_PAYLOAD_OFF(config->vlan_tagged);
    dp->frame_len   = CMC_FRAME_LEN(config->vlan_tagged);

    dp->tx_seq = calloc((size_t)CMC_MAX_VL_ID + 1, sizeof *dp->tx_seq);
    if (!dp->tx_seq) {
        free(dp);
        return NULL;
    }
    for (uint8_t n = 0; n < config->net_count; n++) {
        dp->tracker[n] = calloc((size_t)CMC_MAX_VL_ID + 1, sizeof *dp->tracker[n]);
        if (!dp->tracker[n]) {
            cmc_data_plane_destroy(dp);
            return NULL;
        }
    }
    return dp;
}

void cmc_data_plane_destroy(cmc_data_plane_t *dp)
{
    if (!dp)
        return;

    cmc_data_plane_stop(dp);
    for (uint8_t n = 0; n < APP_MAX_CMC_NETS; n++) {
        if (dp->opened[n])
            raw_socket_close(&dp->sock[n]);
        free(dp->tracker[n]);
    }
    free(dp->tx_seq);
    free(dp);
}

bool cmc_data_plane_open(cmc_data_plane_t *dp)
{
    for (uint8_t n = 0; n < dp->config->net_count; n++) {
        const cmc_net_link_t *link = &dp->config->nets[n];
        bool carrier = false;

        if (!raw_socket_link_up(link->iface, &carrier)) {
            log_line("[cmc] %s (%s) is not up - bring the interface up and try again",
                     link->iface, link->unit_label);
            return false;
        }
        if (!carrier)
            log_line("[cmc] %s (%s) is up but has no carrier - is the cable in?",
                     link->iface, link->unit_label);

        /* Promiscuous: the traffic is addressed to 03:00:00:00:xx:xx, which is
         * nobody's interface address. */
        if (!raw_socket_open(&dp->sock[n], link->iface, true)) {
            log_line("[cmc] cannot open %s (%s)", link->iface, link->unit_label);
            return false;
        }
        dp->opened[n] = true;

        int rcv = 0, snd = 0;
        raw_socket_set_buffers(&dp->sock[n], CMC_SOCK_RCVBUF, CMC_SOCK_SNDBUF,
                              &rcv, &snd);
        const bool bypass = raw_socket_bypass_qdisc(&dp->sock[n]);

        /* This link both sends and receives, so without this the receiver would
         * be handed every frame the sender puts out - one phantom arrival per
         * frame sent, landing in the unexpected-VL counter because the frames
         * this end sends carry the outbound VL range. */
        const bool own = raw_socket_ignore_outgoing(&dp->sock[n]);

        const unsigned mbps = raw_socket_link_mbps(link->iface);

        log_line("[cmc] %-8s %-5s %s  src MAC tail 0x%02X  link %s  rcvbuf %d KB  "
                 "sndbuf %d KB%s%s",
                 link->iface, link->unit_label, link->label, link->src_mac_tail,
                 mbps ? "" : "down", rcv / 1024, snd / 1024,
                 bypass ? "  qdisc bypassed" : "",
                 own ? "" : "  (kernel still shows us our own frames)");
        if (mbps)
            log_line("      %s is up at %u Mbit/s", link->iface, mbps);

        /* Asked before the run rather than diagnosed after it. A link that cannot
         * carry the rate refuses the frames, and a refused frame looks like a lost
         * one in every column of the table - on the row of the link that is
         * working, because the one at fault shows nothing at all. */
        const double want_mbps = cmc_data_plane_net_gbps(dp) * 1000.0;

        if (mbps && (double)mbps < want_mbps)
            log_line("      WARNING: this test paces %.0f Mbit/s onto it. The link "
                     "cannot carry that; lower the CMC target rate or fix the link.",
                     want_mbps);
    }
    return true;
}

double cmc_data_plane_net_gbps(const cmc_data_plane_t *dp)
{
    /* The configured rate is for the test as a whole and the sender emits one
     * frame per network per slot, so each network carries half of it - the
     * reference's TARGET_GBPS / NUM_TX_CORES, with the two networks being the
     * two queues. */
    return dp->config->target_gbps / (double)dp->config->net_count;
}

void cmc_data_plane_reset(cmc_data_plane_t *dp)
{
    for (uint8_t n = 0; n < dp->config->net_count; n++) {
        memset(&dp->stats[n], 0, sizeof dp->stats[n]);
        memset(dp->tracker[n], 0,
               ((size_t)CMC_MAX_VL_ID + 1) * sizeof *dp->tracker[n]);
    }
    /* The TX sequences go back to zero too, as the reference's
     * reset_tx_vl_sequences does: otherwise the first test-window frame carries
     * a sequence the receiver has already seen and is read as a duplicate. */
    memset(dp->tx_seq, 0, ((size_t)CMC_MAX_VL_ID + 1) * sizeof *dp->tx_seq);
}

const cmc_net_stats_t *cmc_data_plane_stats(const cmc_data_plane_t *dp, uint8_t net)
{
    if (!dp || net >= dp->config->net_count)
        return NULL;
    return &dp->stats[net];
}

raw_socket_t *cmc_data_plane_link(cmc_data_plane_t *dp, uint8_t net)
{
    if (!dp || net >= dp->config->net_count || !dp->opened[net])
        return NULL;
    return &dp->sock[net];
}

void cmc_data_plane_pause_tx(cmc_data_plane_t *dp, bool paused)
{
    dp->tx_paused = paused;
}

/* ------------------------------------------------------------------ */
/* Receive                                                           */
/* ------------------------------------------------------------------ */

/* The loss arithmetic, per (network, VL). Real-time gap detection against the
 * sequence expected next, plus a high-water mark, exactly as the reference
 * keeps them. A sequence below what is expected moves nothing: it is a
 * reordering, and counting it as loss would double-count the gap that produced
 * it. */
static uint64_t track_sequence(cmc_vl_tracker_t *t, uint64_t seq)
{
    uint64_t gap = 0;

    if (!t->initialized) {
        t->initialized  = 1;
        t->expected_seq = seq + 1;
    } else {
        if (seq > t->expected_seq)
            gap = seq - t->expected_seq;
        if (seq >= t->expected_seq)
            t->expected_seq = seq + 1;
    }
    if (seq > t->max_seq)
        t->max_seq = seq;
    t->pkt_count++;
    return gap;
}

bool cmc_data_plane_ingest(cmc_data_plane_t *dp, uint8_t net,
                           const uint8_t *frame, size_t len)
{
    cmc_net_stats_t *st = &dp->stats[net];

    st->frames++;
    st->frame_bytes += len;

    if (len < dp->payload_off) {
        st->short_pkts++;
        return false;
    }

    const uint16_t vl_id  = cmc_vl_id_of(frame);
    const uint8_t *payload = frame + dp->payload_off;
    const uint16_t pay_len = (uint16_t)(len - dp->payload_off);

    /* Health monitor first, before the length filter: its frames are nothing
     * like a data-plane frame's length and would be counted as short. */
    if (dp->sink.is_health_vl && dp->sink.is_health_vl(vl_id)) {
        st->health_frames++;
        if (dp->sink.health)
            dp->sink.health(vl_id, payload, pay_len);
        return false;
    }

    /* MMMS, likewise - its chunks are 136 or 1467 bytes - and only while the
     * handover is running, which is what the pause flag says. */
    if (dp->tx_paused && dp->sink.mmms_vl_id && vl_id == dp->sink.mmms_vl_id) {
        st->mmms_frames++;
        if (dp->sink.mmms)
            dp->sink.mmms(payload, pay_len);
        return false;
    }

    if (len < dp->frame_len) {
        st->short_pkts++;
        return false;
    }

    /* The return range is the only thing left that belongs here. Anything else
     * is named rather than silently dropped, because an unexpected VL id on
     * this link is a wiring or configuration answer, not noise. */
    const uint16_t rx_first = dp->config->rx_vl_start;
    const uint16_t rx_last  = (uint16_t)(rx_first + dp->config->vl_count - 1);

    if (vl_id < rx_first || vl_id > rx_last || vl_id > CMC_MAX_VL_ID) {
        st->other_vl++;
        st->last_other_vl = vl_id;
        return false;
    }

    const uint64_t seq = splitmix_payload_seq(payload);
    const uint64_t gap = track_sequence(&dp->tracker[net][vl_id], seq);

    if (gap)
        st->lost += gap;

    /* The CMC's layout: the XOR'd byte, and the trailing byte left out. */
    const splitmix_layout_t layout = SPLITMIX_LAYOUT_CMC(CMC_NUM_PRBS_BYTES);
    splitmix_result_t v;
    const bool ok = splitmix_verify(payload, prbs31_at(dp->prbs, seq), &layout, &v);

    st->total_rx_pkts++;
    if (ok) {
        st->good++;
    } else {
        st->bad++;
        if (!v.splitmix_ok) st->splitmix_fail++;
        if (!v.crc_ok)      st->crc32_fail++;
        if (!v.xor_ok)      st->xor_fail++;
        st->bit_errors += v.bit_errors;
    }
    return ok;
}

struct rx_arg {
    cmc_data_plane_t *dp;
    uint8_t           net;
};

static void *rx_thread_fn(void *arg)
{
    struct rx_arg *a = arg;
    cmc_data_plane_t *dp = a->dp;
    const uint8_t net = a->net;
    uint8_t *buf = malloc(CMC_FRAME_LEN_MAX + 64);

    free(a);
    if (!buf)
        return NULL;

    while (!*dp->stop) {
        /* Poll once, then drain: at tens of thousands of frames a second a poll
         * per frame is half the system calls this thread makes. */
        int n = raw_socket_recv(&dp->sock[net], buf, CMC_FRAME_LEN_MAX + 64, 200);

        while (n > 0) {
            cmc_data_plane_ingest(dp, net, buf, (size_t)n);
            n = raw_socket_recv_nowait(&dp->sock[net], buf, CMC_FRAME_LEN_MAX + 64);
        }
    }
    free(buf);
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Send                                                               */
/* ------------------------------------------------------------------ */


/**
 * @brief Say once why a link will not take our frames.
 *
 * ENOBUFS is the one worth spelling out. The qdisc is bypassed on these sockets,
 * so there is no queue to absorb anything: the kernel hands the frame to the
 * driver and the driver says no. That means the interface cannot transmit at the
 * rate being asked of it - no carrier at all, or a line slower than the rate. It
 * does not mean the CMC dropped anything, and without this line it reads exactly
 * as if it had: the loss column fills and the row that is really at fault is the
 * one showing nothing.
 */
static void report_refusal(const cmc_data_plane_t *dp, uint8_t net, int err)
{
    const cmc_net_link_t *link = &dp->config->nets[net];
    const unsigned mbps = raw_socket_link_mbps(link->iface);
    const double   want = cmc_data_plane_net_gbps(dp) * 1000.0;

    log_line("[cmc] %s (%s) will not take frames: %s", link->iface,
             link->unit_label, strerror(err));
    if (err != ENOBUFS)
        return;

    if (mbps == 0)
        log_line("      the link reports no speed, so it has no carrier - check the "
                 "cable and whether the %s side is powered", link->unit_label);
    else if ((double)mbps < want)
        log_line("      the link negotiated %u Mbit/s and this test asks %.0f "
                 "Mbit/s of it - lower cmc target_gbps or fix the link",
                 mbps, want);
    else
        log_line("      the link is up at %u Mbit/s, so the driver queue is "
                 "backing up rather than the line being too slow", mbps);
    log_line("      every frame refused from here on is counted, not printed; see "
             "the WARNINGS block under the tables");
}

static void *tx_thread_fn(void *arg)
{
    cmc_data_plane_t *dp = arg;
    const cmc_config_t *c = dp->config;
    const uint8_t nets = c->net_count;

    uint8_t frame[APP_MAX_CMC_NETS][CMC_FRAME_LEN_MAX];
    cmc_packet_config_t cfg[APP_MAX_CMC_NETS];

    for (uint8_t n = 0; n < nets; n++) {
        cmc_packet_config_init(&cfg[n]);
        cfg[n].vlan_tagged  = c->vlan_tagged;
        cfg[n].vlan_id      = c->nets[n].tx_vlan;
        cfg[n].src_mac[5]   = c->nets[n].src_mac_tail;
    }

    /* One slot per frame per network, from the per-network rate. The frame
     * length is the wire length, so the rate is the wire rate whether or not
     * the frames carry a tag. */
    const double gbps = cmc_data_plane_net_gbps(dp);
    const double pps  = gbps * 1e9 / 8.0 / (double)dp->frame_len;
    const uint64_t slot_ns = (pps > 0.0) ? (uint64_t)(1e9 / pps) : 1000000ull;

    log_line("[cmc] sending %u VL(s) per network at %.4f Gbps each "
             "(%.0f frame/s, one every %" PRIu64 " us)",
             c->vl_count, gbps, pps, slot_ns / 1000);

    uint64_t next = now_ns() + 5000000ull;      /* a moment for the receivers */
    uint16_t offset = 0;

    while (!*dp->stop) {
        if (dp->tx_paused) {
            /* The handover needs an otherwise idle wire. Keep the slot clock
             * in step with real time so resuming does not fire a burst. */
            struct timespec ts = {.tv_sec = 0, .tv_nsec = 1000000};

            nanosleep(&ts, NULL);
            next = now_ns();
            continue;
        }

        wait_until(next);

        const uint64_t now = now_ns();
        /* A slot already gone is not made up for - that is what would turn a
         * hiccup into a burst. */
        if (next + slot_ns < now)
            next = now;
        next += slot_ns;

        const uint16_t vl_id = (uint16_t)(c->tx_vl_start + offset);
        const uint64_t seq   = dp->tx_seq[vl_id];

        /* The twins: built separately so each carries its own source MAC, and
         * byte-identical everywhere else. */
        for (uint8_t n = 0; n < nets; n++) {
            cfg[n].vl_id = vl_id;
            cmc_packet_build(frame[n], &cfg[n]);
            cmc_packet_fill_payload(frame[n], c->vlan_tagged, dp->prbs, seq);
            cmc_packet_stamp_dtn_seq(frame[n], dp->frame_len, seq);
        }

        /* Network A decides whether the sequence advances, as the reference has
         * it: if A went out the sequence moves on, and a B that did not go out
         * leaves a gap its own receiver reports as loss. If A itself did not go
         * out, nothing moves and the same sequence is tried again. */
        bool first_sent = false;

        for (uint8_t n = 0; n < nets; n++) {
            if (n > 0 && !first_sent) {
                /* A refused, so the twins cannot be twins and B is not sent. Said
                 * once, because otherwise network B looks silent with nothing
                 * against its name: no frames, no refusals, no reason - and the
                 * reason is on the other row. */
                if (!dp->b_skipped_said) {
                    dp->b_skipped_said = true;
                    log_line("[cmc] %s went nowhere, so %s is not being sent either "
                             "- the twins have to go out together or they are not "
                             "twins. Fix %s first.",
                             dp->config->nets[0].iface, dp->config->nets[n].iface,
                             dp->config->nets[0].iface);
                }
                break;
            }
            if (raw_socket_send(&dp->sock[n], frame[n], dp->frame_len)) {
                dp->stats[n].tx_pkts++;
                dp->stats[n].tx_bytes += dp->frame_len;
                if (n == 0)
                    first_sent = true;
            } else {
                /* Said once, per link, and counted from then on. A refusal here
                 * is not rare when it happens at all - it is every frame, tens of
                 * thousands a second - so a line each would flood the terminal,
                 * bury the tables that explain it, and hold the sender at the
                 * speed of stdout. The count and the warnings block carry it. */
                if (dp->stats[n].tx_refused == 0) {
                    dp->stats[n].tx_errno = dp->sock[n].send_errno;
                    report_refusal(dp, n, dp->sock[n].send_errno);
                }
                dp->stats[n].tx_refused++;
            }
        }
        if (first_sent)
            dp->tx_seq[vl_id]++;

        offset = (uint16_t)((offset + 1) % c->vl_count);
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Start and stop                                                     */
/* ------------------------------------------------------------------ */

bool cmc_data_plane_start(cmc_data_plane_t *dp)
{
    /* Receivers first, so the first frame back has somewhere to land - the
     * order the reference starts its workers in, and for the same reason. */
    for (uint8_t n = 0; n < dp->config->net_count; n++) {
        struct rx_arg *a = malloc(sizeof *a);

        if (!a)
            return false;
        a->dp = dp;
        a->net = n;
        if (pthread_create(&dp->rx_thread[n], NULL, rx_thread_fn, a) != 0) {
            log_line("[cmc] cannot start the receiver for %s", dp->config->nets[n].iface);
            free(a);
            return false;
        }
        dp->rx_running[n] = true;
    }

    if (pthread_create(&dp->tx_thread, NULL, tx_thread_fn, dp) != 0) {
        log_line("[cmc] cannot start the sender");
        return false;
    }
    dp->tx_running = true;
    return true;
}

void cmc_data_plane_stop(cmc_data_plane_t *dp)
{
    if (dp->tx_running) {
        pthread_join(dp->tx_thread, NULL);
        dp->tx_running = false;
    }
    for (uint8_t n = 0; n < APP_MAX_CMC_NETS; n++) {
        if (dp->rx_running[n]) {
            pthread_join(dp->rx_thread[n], NULL);
            dp->rx_running[n] = false;
        }
    }
}

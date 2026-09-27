#define _GNU_SOURCE             /* clock_nanosleep with TIMER_ABSTIME */

#include "DtnLegs.h"

#include "DtnConfig.h"
#include "Log.h"

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Headers before the payload, and the AFDX sequence byte after it. */
#define LEG_HDR_BYTES   42
#define LEG_TRAILER     1
#define LEG_SEQ_BYTES   8

/* BAG is 1 ms on every VL record, so one frame per VL per millisecond is the
 * ceiling the DTN polices against. */
#define LEG_BAG_FPS     1000.0

/* Below this, waiting is spun rather than slept: a sleep that short costs more
 * in wake-up jitter than it saves in idle. */
#define LEG_SPIN_NS     60000

/* The return-VL lookup. VL ids on this rig reach 6083; a flat table is two bytes
 * a VL and turns the receive path's question into one load. */
#define LEG_VL_TABLE    8192
#define LEG_NONE        0xFF

typedef struct {
    uint64_t expected_seq;
    uint64_t max_seq;
    bool     initialized;
} leg_rx_vl_t;

typedef struct {
    uint8_t  copper_port;
    uint8_t  fibre_port;
    uint16_t tx_first;      /**< first outbound VL */
    uint16_t rx_first;      /**< first return VL */
    uint16_t vl_count;
    const char *label;

    raw_socket_t *sock;

    uint64_t    tx_seq[DTN_LEG_MAX_VLS];    /**< per VL, the sender's */
    uint8_t     afdx_seq[DTN_LEG_MAX_VLS];  /**< per VL, the trailing byte */
    leg_rx_vl_t rx[DTN_LEG_MAX_VLS];

    dtn_leg_stats_t stats;

    pthread_t thread;
    bool      running;
} leg_t;

struct dtn_legs {
    dtn_leg_config_t       config;
    const prbs31_cache_t  *prbs;
    volatile bool         *stop;
    volatile bool          own_stop;
    volatile bool          paused;

    leg_t   leg[DTN_LEG_MAX];
    uint8_t count;

    size_t  payload_bytes;   /**< sequence and PRBS */
    size_t  prbs_bytes;      /**< payload minus the sequence */

    /* VL id -> which leg and which of its VLs, for return frames. */
    uint8_t vl_leg[LEG_VL_TABLE];
    uint8_t vl_off[LEG_VL_TABLE];
};

/* ------------------------------------------------------------------ */

static uint64_t now_ns(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

/* Sleep most of the gap, spin the last stretch. Absolute, so a late wake-up does
 * not compound, and a slot already gone is not made up for - which is what would
 * turn a hiccup into a burst the DTN would police. */
static void wait_until(uint64_t target)
{
    for (;;) {
        const uint64_t now = now_ns();

        if (now >= target)
            return;
        if (target - now > LEG_SPIN_NS) {
            struct timespec ts = {
                .tv_sec  = (time_t)(target / 1000000000ull),
                .tv_nsec = (long)(target % 1000000000ull),
            };

            if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL) == EINTR)
                return;
        }
    }
}

static void put_be64(uint8_t *p, uint64_t v)
{
    for (int i = 0; i < 8; i++)
        p[i] = (uint8_t)(v >> (8 * (7 - i)));
}

static uint64_t rd_be64(const uint8_t *p)
{
    uint64_t v = 0;

    for (int i = 0; i < 8; i++)
        v = (v << 8) | p[i];
    return v;
}

size_t dtn_legs_prbs_stride(const dtn_leg_config_t *config)
{
    return (size_t)config->frame_bytes - LEG_HDR_BYTES - LEG_TRAILER - LEG_SEQ_BYTES;
}

/* ------------------------------------------------------------------ */
/* Reading the legs out of a profile                                  */
/* ------------------------------------------------------------------ */

/* A leg is a run whose source is a copper port, together with the run that comes
 * back between the same two ports. Paired by their ports rather than by their
 * order in the table, so reordering the profile cannot silently pair the wrong
 * two runs. */
static bool build_legs(dtn_legs_t *legs, const vl_profile_t *profile)
{
    for (uint8_t i = 0; i < profile->comm_count; i++) {
        const vl_run_t *out = &profile->comms[i];

        if (out->src_port < 32)
            continue;                       /* a return run; picked up below */

        const vl_run_t *back = NULL;
        for (uint8_t k = 0; k < profile->comm_count; k++)
            if (profile->comms[k].src_port == out->dst_port &&
                profile->comms[k].dst_port == out->src_port)
                back = &profile->comms[k];

        if (!back) {
            log_line("[legs] VL %u-%u goes out of copper %u to port %u with no way "
                     "back - not tested", out->vl_first,
                     out->vl_first + out->vl_count - 1, out->src_port, out->dst_port);
            continue;
        }
        if (back->vl_count != out->vl_count) {
            log_line("[legs] copper %u <-> port %u has %u VLs out and %u back; "
                     "not a matched pair", out->src_port, out->dst_port,
                     out->vl_count, back->vl_count);
            return false;
        }
        if (out->vl_count > DTN_LEG_MAX_VLS) {
            log_line("[legs] copper %u <-> port %u wants %u VLs, the most is %d",
                     out->src_port, out->dst_port, out->vl_count, DTN_LEG_MAX_VLS);
            return false;
        }
        if (legs->count == DTN_LEG_MAX) {
            log_line("[legs] more legs than the %d this build carries", DTN_LEG_MAX);
            return false;
        }

        leg_t *leg = &legs->leg[legs->count];

        leg->copper_port = out->src_port;
        leg->fibre_port  = out->dst_port;
        leg->tx_first    = out->vl_first;
        leg->rx_first    = back->vl_first;
        leg->vl_count    = out->vl_count;
        leg->label       = out->label;

        for (uint16_t k = 0; k < leg->vl_count; k++) {
            const uint16_t vl = (uint16_t)(leg->rx_first + k);

            if (vl >= LEG_VL_TABLE) {
                log_line("[legs] return VL %u is past the %d this build indexes",
                         vl, LEG_VL_TABLE);
                return false;
            }
            legs->vl_leg[vl] = legs->count;
            legs->vl_off[vl] = (uint8_t)k;
        }
        legs->count++;
    }
    return legs->count > 0;
}

dtn_legs_t *dtn_legs_create(const vl_profile_t *profile,
                            const dtn_leg_config_t *config,
                            const prbs31_cache_t *prbs,
                            volatile bool *stop)
{
    if (!profile || profile->comm_count == 0)
        return NULL;

    dtn_legs_t *legs = calloc(1, sizeof *legs);
    if (!legs)
        return NULL;

    legs->config = *config;
    legs->prbs   = prbs;
    legs->stop   = stop ? stop : &legs->own_stop;
    memset(legs->vl_leg, LEG_NONE, sizeof legs->vl_leg);

    legs->prbs_bytes    = dtn_legs_prbs_stride(config);
    legs->payload_bytes = legs->prbs_bytes + LEG_SEQ_BYTES;

    if (!build_legs(legs, profile)) {
        free(legs);
        return NULL;
    }
    return legs;
}

void dtn_legs_destroy(dtn_legs_t *legs)
{
    if (!legs)
        return;
    dtn_legs_stop(legs);
    free(legs);
}

bool dtn_legs_bind(dtn_legs_t *legs, uint8_t copper_port, raw_socket_t *sock)
{
    for (uint8_t l = 0; l < legs->count; l++)
        if (legs->leg[l].copper_port == copper_port) {
            legs->leg[l].sock = sock;
            return true;
        }
    return false;
}

uint8_t dtn_legs_count(const dtn_legs_t *legs)
{
    return legs ? legs->count : 0;
}

const dtn_leg_stats_t *dtn_legs_stats(const dtn_legs_t *legs, uint8_t leg)
{
    if (!legs || leg >= legs->count)
        return NULL;
    return &legs->leg[leg].stats;
}

void dtn_legs_pause(dtn_legs_t *legs, bool paused)
{
    if (legs)
        legs->paused = paused;
}

void dtn_legs_reset(dtn_legs_t *legs)
{
    for (uint8_t l = 0; l < legs->count; l++) {
        leg_t *leg = &legs->leg[l];

        memset(&leg->stats, 0, sizeof leg->stats);
        memset(leg->tx_seq, 0, sizeof leg->tx_seq);
        memset(leg->afdx_seq, 0, sizeof leg->afdx_seq);
        memset(leg->rx, 0, sizeof leg->rx);
    }
}

bool dtn_legs_rate_plan(const dtn_legs_t *legs, double *fps_per_leg,
                        double *fps_per_vl)
{
    const double fps = legs->config.target_mbps * 1e6 / 8.0 /
                       (double)legs->config.frame_bytes;
    double per_vl = fps;

    if (legs->count && legs->leg[0].vl_count)
        per_vl = fps / (double)legs->leg[0].vl_count;

    if (fps_per_leg) *fps_per_leg = fps;
    if (fps_per_vl)  *fps_per_vl  = per_vl;
    return per_vl <= LEG_BAG_FPS;
}

/* ------------------------------------------------------------------ */
/* Receive                                                            */
/* ------------------------------------------------------------------ */

/* Gaps count once, at the frame that reveals them. A frame behind what is
 * expected moves nothing: it is a reordering, and counting it as loss would
 * double-count the gap that produced it. */
static uint64_t track(leg_rx_vl_t *vl, uint64_t seq, bool *late)
{
    uint64_t gap = 0;

    *late = false;
    if (!vl->initialized) {
        vl->initialized  = true;
        vl->expected_seq = seq + 1;
    } else if (seq > vl->expected_seq) {
        gap = seq - vl->expected_seq;
        vl->expected_seq = seq + 1;
    } else if (seq == vl->expected_seq) {
        vl->expected_seq = seq + 1;
    } else {
        *late = true;
    }
    if (seq > vl->max_seq)
        vl->max_seq = seq;
    return gap;
}

static uint64_t bit_errors(const uint8_t *a, const uint8_t *b, size_t len)
{
    uint64_t bits = 0;

    for (size_t i = 0; i < len; i++) {
        const uint8_t diff = (uint8_t)(a[i] ^ b[i]);

        if (diff)
            bits += (uint64_t)__builtin_popcount(diff);
    }
    return bits;
}

bool dtn_legs_ingest(dtn_legs_t *legs, uint8_t copper_port,
                     const uint8_t *frame, size_t len)
{
    if (!legs || len < LEG_HDR_BYTES + LEG_SEQ_BYTES)
        return false;

    const uint16_t vl_id = (uint16_t)((frame[4] << 8) | frame[5]);

    if (vl_id >= LEG_VL_TABLE || legs->vl_leg[vl_id] == LEG_NONE)
        return false;

    leg_t *leg = &legs->leg[legs->vl_leg[vl_id]];

    /* A return VL that arrived on the wrong copper link is not this leg's, and
     * saying so is more useful than counting it: it is what a swapped pair of
     * cables looks like. */
    if (leg->copper_port != copper_port)
        return false;

    const uint8_t off = legs->vl_off[vl_id];

    leg->stats.rx_frames++;
    leg->stats.rx_bytes += len;

    /* The frame is headers, payload and one AFDX byte. Anything else on a return
     * VL is not one of ours - named rather than verified, because verifying it
     * would compare the wrong bytes and report a unit fault. */
    if (len != (size_t)LEG_HDR_BYTES + legs->payload_bytes + LEG_TRAILER) {
        leg->stats.wrong_length++;
        return true;
    }

    const uint8_t *payload = frame + LEG_HDR_BYTES;
    const uint64_t seq = rd_be64(payload);
    bool late = false;
    const uint64_t gap = track(&leg->rx[off], seq, &late);

    if (gap)
        leg->stats.lost += gap;
    if (late)
        leg->stats.late++;

    const uint8_t *want = prbs31_at(legs->prbs, seq);

    if (want && memcmp(payload + LEG_SEQ_BYTES, want, legs->prbs_bytes) == 0) {
        leg->stats.good++;
    } else {
        leg->stats.bad++;
        if (want)
            leg->stats.bit_errors += bit_errors(payload + LEG_SEQ_BYTES, want,
                                                legs->prbs_bytes);
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* Send                                                               */
/* ------------------------------------------------------------------ */

/* The sender needs both the leg and its container, so they travel together. */
struct leg_arg {
    dtn_legs_t *legs;
    leg_t      *leg;
};

static void *leg_sender(void *arg)
{
    struct leg_arg *a = arg;
    dtn_legs_t *legs = a->legs;
    leg_t *leg = a->leg;
    uint8_t payload[2048];
    uint8_t frame[DTN_MAX_FRAME];

    free(a);

    double fps = 0.0;
    (void)dtn_legs_rate_plan(legs, &fps, NULL);

    const uint64_t slot_ns = (fps > 0.0) ? (uint64_t)(1e9 / fps) : 1000000ull;
    uint64_t next = now_ns() + 5000000ull;      /* a moment for the receiver */
    uint16_t off = 0;

    while (!*legs->stop) {
        if (legs->paused) {
            struct timespec ts = {.tv_sec = 0, .tv_nsec = 1000000};

            nanosleep(&ts, NULL);
            next = now_ns();
            continue;
        }

        wait_until(next);

        const uint64_t now = now_ns();
        if (next + slot_ns < now)
            next = now;
        next += slot_ns;

        const uint16_t vl_id = (uint16_t)(leg->tx_first + off);
        const uint64_t seq = leg->tx_seq[off];
        const uint8_t *prbs = prbs31_at(legs->prbs, seq);

        if (!prbs)
            break;

        put_be64(payload, seq);
        memcpy(payload + LEG_SEQ_BYTES, prbs, legs->prbs_bytes);

        leg->afdx_seq[off] = (seq == 0) ? 0 : dtn_next_seq(leg->afdx_seq[off]);

        const int n = dtn_build_frame_from(leg->copper_port, payload,
                                          legs->payload_bytes, leg->afdx_seq[off],
                                          vl_id, -1, DTN_NET_A,
                                          frame, sizeof frame);
        if (n > 0 && raw_socket_send(leg->sock, frame, (size_t)n)) {
            leg->stats.tx_frames++;
            leg->stats.tx_bytes += (uint64_t)n;
            leg->tx_seq[off]++;
        } else {
            leg->stats.tx_refused++;
        }

        off = (uint16_t)((off + 1) % leg->vl_count);
    }
    return NULL;
}

bool dtn_legs_start(dtn_legs_t *legs)
{
    double fps = 0.0, per_vl = 0.0;
    const bool within_bag = dtn_legs_rate_plan(legs, &fps, &per_vl);

    for (uint8_t l = 0; l < legs->count; l++) {
        leg_t *leg = &legs->leg[l];

        if (!leg->sock) {
            log_line("[legs] copper %u has no socket bound", leg->copper_port);
            return false;
        }
        log_line("[legs] %s: copper %u <-> port %u, VL %u-%u out, %u-%u back, "
                 "%u byte frames at %.1f Mbit/s (%.0f frame/s, %.0f per VL)",
                 leg->label ? leg->label : "leg", leg->copper_port, leg->fibre_port,
                 leg->tx_first, leg->tx_first + leg->vl_count - 1,
                 leg->rx_first, leg->rx_first + leg->vl_count - 1,
                 legs->config.frame_bytes, legs->config.target_mbps, fps, per_vl);
    }

    if (!within_bag)
        log_line("[legs] WARNING: %.0f frame/s per VL is more than BAG 1 ms allows "
                 "(%.0f); the DTN will police the excess and it will look like loss",
                 per_vl, LEG_BAG_FPS);

    for (uint8_t l = 0; l < legs->count; l++) {
        struct leg_arg *a = malloc(sizeof *a);

        if (!a)
            return false;
        a->legs = legs;
        a->leg  = &legs->leg[l];
        if (pthread_create(&legs->leg[l].thread, NULL, leg_sender, a) != 0) {
            log_line("[legs] cannot start the sender for copper %u",
                     legs->leg[l].copper_port);
            free(a);
            return false;
        }
        legs->leg[l].running = true;
    }
    return true;
}

void dtn_legs_stop(dtn_legs_t *legs)
{
    for (uint8_t l = 0; l < DTN_LEG_MAX; l++)
        if (legs->leg[l].running) {
            pthread_join(legs->leg[l].thread, NULL);
            legs->leg[l].running = false;
        }
}

/* ------------------------------------------------------------------ */
/* The table                                                          */
/* ------------------------------------------------------------------ */

void dtn_legs_print_table(dtn_legs_t *legs)
{
    if (!legs || legs->count == 0)
        return;

    printf("\n  copper legs (workstation -> DTN -> VMC -> DTN -> workstation)\n");
    printf("  ┌────────┬───────────────┬─────────────────────┬─────────────────────┬─────────────────────┬─────────────────────┬─────────────────────┬─────────────────────┬─────────────┐\n");
    printf("  │ copper │   VL out /    │        Sent         │      Returned       │        Good         │         Bad         │        Lost         │      Bit Error      │     BER     │\n");
    printf("  │  port  │   VL back     │                     │                     │                     │                     │                     │                     │             │\n");
    printf("  ├────────┼───────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────┤\n");

    for (uint8_t l = 0; l < legs->count; l++) {
        const leg_t *leg = &legs->leg[l];
        const dtn_leg_stats_t *st = &leg->stats;
        char range[16];

        snprintf(range, sizeof range, "%u/%u", leg->tx_first, leg->rx_first);

        const uint64_t lost_bits = st->lost * (uint64_t)legs->config.frame_bytes * 8;
        const uint64_t errors = st->bit_errors + lost_bits;
        const uint64_t total_bits = st->rx_bytes * 8 + lost_bits;
        const double ber = total_bits ? (double)errors / (double)total_bits : 0.0;

        printf("  │   %2u   │ %-13s │ %19" PRIu64 " │ %19" PRIu64 " │ %19" PRIu64
               " │ %19" PRIu64 " │ %19" PRIu64 " │ %19" PRIu64 " │ %11.2e │\n",
               leg->copper_port, range, st->tx_frames, st->rx_frames,
               st->good, st->bad, st->lost, errors, ber);
    }
    printf("  └────────┴───────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────┘\n");

    /* What is wrong, in words, under the numbers. */
    for (uint8_t l = 0; l < legs->count; l++) {
        const leg_t *leg = &legs->leg[l];
        const dtn_leg_stats_t *st = &leg->stats;

        if (st->tx_frames && st->rx_frames == 0)
            printf("      copper %u: %" PRIu64 " frame(s) sent, nothing back - the "
                   "DTN is not forwarding this leg, or the VMC is not answering\n",
                   leg->copper_port, st->tx_frames);
        if (st->bad)
            printf("      copper %u: %" PRIu64 " frame(s) came back with the wrong "
                   "payload\n", leg->copper_port, st->bad);
        if (st->lost)
            printf("      copper %u: %" PRIu64 " frame(s) lost\n",
                   leg->copper_port, st->lost);
        if (st->late)
            printf("      copper %u: %" PRIu64 " frame(s) arrived out of order "
                   "(not counted as loss)\n", leg->copper_port, st->late);
        if (st->wrong_length)
            printf("      copper %u: %" PRIu64 " frame(s) on a return VL were not "
                   "this test's\n", leg->copper_port, st->wrong_length);
        if (st->tx_refused)
            printf("      copper %u: %" PRIu64 " frame(s) the link would not take - "
                   "that is this end, not the unit\n",
                   leg->copper_port, st->tx_refused);
    }
}

void dtn_legs_log_summary(const dtn_legs_t *legs)
{
    if (!legs || legs->count == 0)
        return;

    log_line("copper legs:");
    for (uint8_t l = 0; l < legs->count; l++) {
        const leg_t *leg = &legs->leg[l];
        const dtn_leg_stats_t *st = &leg->stats;

        log_line("  copper %u <-> port %u (VL %u-%u out, %u-%u back)",
                 leg->copper_port, leg->fibre_port,
                 leg->tx_first, leg->tx_first + leg->vl_count - 1,
                 leg->rx_first, leg->rx_first + leg->vl_count - 1);
        log_line("    sent %" PRIu64 " / %" PRIu64 " byte(s), returned %" PRIu64
                 " / %" PRIu64, st->tx_frames, st->tx_bytes, st->rx_frames,
                 st->rx_bytes);
        log_line("    good %" PRIu64 ", bad %" PRIu64 ", lost %" PRIu64
                 ", out of order %" PRIu64 ", bit errors %" PRIu64,
                 st->good, st->bad, st->lost, st->late, st->bit_errors);
        if (st->wrong_length)
            log_line("    %" PRIu64 " frame(s) on a return VL were not this test's",
                     st->wrong_length);
        if (st->tx_refused)
            log_line("    %" PRIu64 " frame(s) the link refused", st->tx_refused);
    }
}

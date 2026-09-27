/**
 * @file smoke_dtn_legs.c
 * @brief The copper legs on a real socket, with a stand-in for the DTN and the VMC.
 *
 * test_dtn_legs hands frames to a function. This one puts them on a wire: the
 * real senders against the loopback interface, with a thread standing in for
 * everything past the copper port - it reads what the sender emits on an outbound
 * VL, rewrites the VL id into the return range the way the DTN's table does, and
 * sends it back - and then checks the receive path verified it.
 *
 * What that proves is the plumbing: the sender paces and actually transmits, the
 * frame it builds is the frame the receiver expects, the sequence survives the
 * round trip, and the threads start and join. What it cannot prove is anything
 * about the DTN or the VMC.
 *
 * Not part of `make test`: it needs a raw socket, so it needs root. `make smoke`.
 *
 * One leg, not two. Both would share the loopback interface and each would see
 * the other's frames, and the copper port is what tells the legs apart - so
 * aliasing them defeats the thing being tested.
 */

#define _GNU_SOURCE

#include "AppConfig.h"
#include "DtnConfig.h"
#include "DtnLegs.h"
#include "Log.h"
#include "PayloadVerify.h"
#include "SplitmixVerify.h"
#include "RawSocket.h"
#include "VlProfile.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures;

static void check(bool ok, const char *what)
{
    printf("%s %s\n", ok ? "[ OK ]" : "[FAIL]", what);
    if (!ok)
        failures++;
}

#define SMOKE_IFACE   "lo"
#define SMOKE_SECONDS 2

/* A profile with one leg on the loopback interface. Small: four VLs, so each
 * one's sequence advances fast enough to see, and a rate that fills a couple of
 * seconds with a few thousand frames rather than a few hundred thousand. */
static const vl_link_t g_no_links[] = {{0, 1}};

static vl_profile_t smoke_profile(void)
{
    vl_profile_t p = {0};

    p.name = "smoke";
    p.description = "one copper leg on the loopback interface";
    p.group_count = 1;
    p.groups[0] = (vl_link_group_t){.vl_base = 1024, .vls_per_link = 1,
                                   .link_count = 1, .links = g_no_links};
    p.comm_count = 2;
    p.comms[0] = (vl_run_t){.vl_first = 3024, .vl_count = 4, .src_port = 32,
                           .dst_port = 8, .label = "out"};
    p.comms[1] = (vl_run_t){.vl_first = 4024, .vl_count = 4, .src_port = 8,
                           .dst_port = 32, .label = "back"};
    p.dtn_health_monitor = false;
    p.dtn_hm_port = 32;
    return p;
}

/* ------------------------------------------------------------------ */
/* The stand-in for the DTN and the VMC                               */
/* ------------------------------------------------------------------ */

struct unit_ctx {
    raw_socket_t          sock;
    volatile bool        *stop;
    uint16_t              tx_first;
    uint16_t              rx_first;
    uint16_t              vl_count;
    size_t                prbs_bytes;
    const prbs31_cache_t *prbs;     /**< its own copy, so an offset disagreement shows */
    uint64_t              echoed;
};

static void *unit_thread_fn(void *arg)
{
    struct unit_ctx *u = arg;
    uint8_t *buf = malloc(DTN_MAX_FRAME + 64);

    if (!buf)
        return NULL;

    while (!*u->stop) {
        int n = raw_socket_recv(&u->sock, buf, DTN_MAX_FRAME + 64, 200);

        while (n > 0) {
            const uint16_t vl = (uint16_t)((buf[4] << 8) | buf[5]);

            if (vl >= u->tx_first && vl < u->tx_first + u->vl_count) {
                const uint16_t back = (uint16_t)(u->rx_first + (vl - u->tx_first));

                /* The VL id, in both the places a frame carries it, and the IP
                 * checksum after it. Then the VMC's rewrite of the payload:
                 * SplitMix64 over the sequence, a CRC32C over both, and the rest
                 * left as PRBS - dpdk_vmc's transform, written out here rather
                 * than called from the verifier so the two are checked against
                 * each other. */
                buf[4] = (uint8_t)(back >> 8);
                buf[5] = (uint8_t)back;

                uint8_t *ip = buf + 14;
                ip[18] = (uint8_t)(back >> 8);
                ip[19] = (uint8_t)back;
                ip[10] = ip[11] = 0;
                uint32_t sum = 0;
                for (int i = 0; i < 20; i += 2)
                    sum += (uint32_t)((ip[i] << 8) | ip[i + 1]);
                while (sum >> 16)
                    sum = (sum & 0xFFFF) + (sum >> 16);
                const uint16_t csum = (uint16_t)~sum;
                ip[10] = (uint8_t)(csum >> 8);
                ip[11] = (uint8_t)csum;

                uint8_t *payload = buf + 42;
                uint64_t seq;
                memcpy(&seq, payload, sizeof seq);

                const uint8_t *prbs = prbs31_at(u->prbs, seq);
                const uint64_t seq_be = __builtin_bswap64(seq);

                for (int blk = 0; blk < SPLITMIX_XOR_BYTES / 8; blk++) {
                    const uint64_t sm =
                        __builtin_bswap64(splitmix64(8 * seq_be + (uint64_t)blk));
                    uint64_t orig;

                    memcpy(&orig, prbs + blk * 8, 8);
                    const uint64_t xored = orig ^ sm;
                    memcpy(payload + 8 + blk * 8, &xored, 8);
                }
                const uint32_t crc = sw_crc32c(payload, 8 + SPLITMIX_XOR_BYTES);
                const uint32_t be = __builtin_bswap32(crc);
                memcpy(payload + 8 + SPLITMIX_XOR_BYTES, &be, sizeof be);

                /* And the byte the DTN would overwrite on the way through, so the
                 * round trip is as unkind as the real one. */
                buf[n - 1] = (uint8_t)(seq & 0xFF);

                if (raw_socket_send(&u->sock, buf, (size_t)n))
                    u->echoed++;
            }
            n = raw_socket_recv_nowait(&u->sock, buf, DTN_MAX_FRAME + 64);
        }
    }
    free(buf);
    return NULL;
}

/* ------------------------------------------------------------------ */

int main(void)
{
    const vl_profile_t profile = smoke_profile();
    const dtn_leg_config_t *base = app_config_dtn_legs();
    dtn_leg_config_t cfg = *base;
    prbs31_cache_t prbs = {0};
    prbs31_cache_t unit_prbs = {0};
    volatile bool stop = false;
    struct unit_ctx unit = {0};
    pthread_t unit_thread;
    raw_socket_t sock;
    dtn_legs_t *legs = NULL;
    uint8_t *buf = malloc(DTN_MAX_FRAME + 64);
    int rc = 1;

    /* A tenth of the rig's rate: the loopback interface is not the point and a
     * few thousand frames is plenty to see the round trip working. */
    cfg.target_mbps = 10.0;

    printf("DTN copper-leg smoke test: %s, 4 VLs, %.1f Mbit/s, %d s\n",
           SMOKE_IFACE, cfg.target_mbps, SMOKE_SECONDS);

    if (!buf) {
        puts("[FAIL] out of memory");
        return 1;
    }

    printf("Generating the PRBS-31 stream twice (%zu MB each - one for each end, "
           "so an offset disagreement would show)...\n",
           PRBS31_CACHE_SIZE / (1024 * 1024));
    if (!prbs31_cache_init(&prbs, PRBS31_INITIAL_STATE,
                           dtn_legs_prbs_stride(&cfg), NULL) ||
        !prbs31_cache_init(&unit_prbs, PRBS31_INITIAL_STATE,
                           dtn_legs_prbs_stride(&cfg), NULL)) {
        puts("[FAIL] the PRBS stream would not allocate");
        prbs31_cache_free(&prbs);
        prbs31_cache_free(&unit_prbs);
        free(buf);
        return 1;
    }

    /* The stand-in comes up first, so nothing the sender emits is missed. */
    unit.stop = &stop;
    unit.tx_first = 3024;
    unit.rx_first = 4024;
    unit.vl_count = 4;
    unit.prbs = &unit_prbs;
    unit.prbs_bytes = dtn_legs_prbs_stride(&cfg);
    if (!raw_socket_open(&unit.sock, SMOKE_IFACE, true)) {
        puts("[FAIL] could not open a raw socket on " SMOKE_IFACE
             " - this test needs root");
        goto out;
    }
    raw_socket_set_buffers(&unit.sock, 8 * 1024 * 1024, 4 * 1024 * 1024, NULL, NULL);
    /* The stand-in needs this as much as the leg does. On the loopback interface
     * a packet socket is handed both the transmit-side copy of a frame and the
     * looped-back one, so without it the stand-in would see every outbound frame
     * twice and echo it twice - and the leg would report half its traffic as
     * arriving out of order, for a reason that is the test's and not the code's. */
    (void)raw_socket_ignore_outgoing(&unit.sock);
    if (pthread_create(&unit_thread, NULL, unit_thread_fn, &unit) != 0) {
        puts("[FAIL] could not start the stand-in");
        raw_socket_close(&unit.sock);
        goto out;
    }

    /* The leg's own socket. Ignoring our own transmissions matters here for the
     * same reason it does on the CMC: without it the receive path would be handed
     * every frame the sender puts out, and each would arrive on an outbound VL. */
    if (!raw_socket_open(&sock, SMOKE_IFACE, true)) {
        puts("[FAIL] could not open the leg's socket");
        goto join;
    }
    raw_socket_set_buffers(&sock, 8 * 1024 * 1024, 4 * 1024 * 1024, NULL, NULL);
    (void)raw_socket_ignore_outgoing(&sock);

    legs = dtn_legs_create(&profile, &cfg, &prbs, &stop);
    check(legs != NULL, "the leg is read out of the profile");
    if (!legs)
        goto join;
    check(dtn_legs_count(legs) == 1, "one leg");
    check(dtn_legs_bind(legs, 32, &sock), "and its socket binds to copper 32");
    check(dtn_legs_start(legs), "the sender starts");

    /* The receive path, driven here the way the test's monitor loop drives it. */
    const uint64_t deadline_s = (uint64_t)SMOKE_SECONDS;
    uint64_t ticks = 0;

    for (uint64_t start = (uint64_t)time(NULL);
         (uint64_t)time(NULL) - start < deadline_s;) {
        int n = raw_socket_recv(&sock, buf, DTN_MAX_FRAME + 64, 100);

        while (n > 0) {
            dtn_legs_ingest(legs, 32, buf, (size_t)n);
            ticks++;
            n = raw_socket_recv_nowait(&sock, buf, DTN_MAX_FRAME + 64);
        }
    }

    /* Stop sending, drain what is in flight, then stop everything - otherwise the
     * frames in the air at that instant are counted sent but never verified. */
    dtn_legs_pause(legs, true);
    for (int i = 0; i < 40; i++) {
        int n = raw_socket_recv(&sock, buf, DTN_MAX_FRAME + 64, 5);

        while (n > 0) {
            dtn_legs_ingest(legs, 32, buf, (size_t)n);
            n = raw_socket_recv_nowait(&sock, buf, DTN_MAX_FRAME + 64);
        }
    }

    stop = true;
    dtn_legs_stop(legs);
    pthread_join(unit_thread, NULL);

    const dtn_leg_stats_t *st = dtn_legs_stats(legs, 0);

    printf("\n  sent %" PRIu64 ", echoed by the stand-in %" PRIu64
           ", returned %" PRIu64 ", verified %" PRIu64 " good / %" PRIu64 " bad\n",
           st->tx_frames, unit.echoed, st->rx_frames, st->good, st->bad);
    printf("  lost %" PRIu64 ", out of order %" PRIu64 ", wrong length %" PRIu64
           ", refused %" PRIu64 "\n\n",
           st->lost, st->late, st->wrong_length, st->tx_refused);

    check(st->tx_frames > 0, "the sender put frames on the wire");
    check(unit.echoed > 0, "the stand-in received them and sent them back");
    check(st->good > 0, "and the receive path verified what came back");
    check(st->bad == 0, "with nothing failing verification");
    check(st->splitmix_fail == 0 && st->crc_fail == 0,
          "the SplitMix zone and its CRC both regenerate");
    check(st->lost == 0, "and no gaps in any VL's sequence");
    check(st->wrong_length == 0, "nothing arrived the wrong length");
    check(st->late == 0, "and nothing arrived twice or out of order");
    check(st->tx_refused == 0, "the link took every frame");

    /* Exactly once each: fewer means the receive path dropped some, more means it
     * counted some twice - which is what a socket handed its own transmissions
     * does, and why this one ignores them. A few frames of slack for whatever was
     * in flight when the threads stopped. */
    check(st->good <= unit.echoed && st->good + 8 >= unit.echoed,
          "every frame that came back was verified exactly once");

    /* The rate. Loopback is not a wire and this is not a measurement of the
     * pacing: three threads share these cores, one of them running SplitMix64
     * and a CRC over every frame, so the sender loses slots to contention that
     * it would not lose on the rig. Measured on its own the loop keeps 826
     * frames a second against 828 asked, and 8176 against 8264 at the slot the
     * rig actually uses. What is checked here is only that pacing produced
     * something in the region rather than a burst or a trickle. */
    double fps = 0.0;
    (void)dtn_legs_rate_plan(legs, &fps, NULL);
    const double want = fps * SMOKE_SECONDS;
    printf("  paced %" PRIu64 " frames in %d s, asked for about %.0f\n",
           st->tx_frames, SMOKE_SECONDS, want);
    check((double)st->tx_frames > want / 2.0 && (double)st->tx_frames < want * 2.0,
          "the pacing is in the region it was asked for");

    dtn_legs_print_table(legs);
    rc = failures ? 1 : 0;

join:
    stop = true;
    if (legs)
        dtn_legs_stop(legs);
    raw_socket_close(&unit.sock);
out:
    dtn_legs_destroy(legs);
    prbs31_cache_free(&prbs);
    prbs31_cache_free(&unit_prbs);
    free(buf);

    if (rc == 0)
        puts("\nPASS: a copper leg runs on a real socket, end to end");
    else
        printf("\nFAILED: %d check(s)\n", failures ? failures : 1);
    return rc;
}

/*
 * The copper legs: what the sender builds, and what the receiver makes of what
 * comes back.
 *
 * The legs are read out of the profile rather than written into the test, so
 * these checks are about the machinery: that a leg is paired with the run that
 * comes back between the same two ports, that a returned frame verifies against
 * a stream regenerated from the sequence inside it alone, that a gap is loss
 * once and a late frame is not loss at all, and that traffic which is not a
 * leg's is left for the health monitor rather than claimed.
 */

#include "AppConfig.h"
#include "DtnConfig.h"
#include "DtnLegs.h"
#include "VlProfile.h"

#include <stdio.h>
#include <string.h>

static int failures;

static void check(bool ok, const char *what)
{
    if (!ok) {
        printf("[FAIL] %s\n", what);
        failures++;
    }
}

/* A stream long enough for the sequences this test uses. The real one is the
 * whole PRBS period; nothing here needs a quarter of a gigabyte. */
#define TEST_SEQS 16
static uint8_t g_prbs_bytes[2048 * (TEST_SEQS + 1)];
static prbs31_cache_t g_prbs;
static uint8_t g_frame[DTN_MAX_FRAME];

static const vl_profile_t *config1(void)
{
    size_t n;
    const vl_profile_t *all = vl_profile_all(&n);

    for (size_t i = 0; i < n; i++)
        if (strcmp(all[i].name, "config1") == 0)
            return &all[i];
    return NULL;
}

static void put_be64(uint8_t *p, uint64_t v)
{
    for (int i = 0; i < 8; i++)
        p[i] = (uint8_t)(v >> (8 * (7 - i)));
}

/* One frame as it comes back: sent from the fibre port on the return VL, with
 * the payload the workstation put in it. */
static size_t returned_frame(const dtn_leg_config_t *cfg, uint8_t fibre_port,
                            uint16_t rx_vl, uint64_t seq)
{
    const size_t prbs_len = dtn_legs_prbs_stride(cfg);
    uint8_t payload[2048];

    put_be64(payload, seq);
    memcpy(payload + 8, prbs31_at(&g_prbs, seq), prbs_len);

    int n = dtn_build_frame_from(fibre_port, payload, prbs_len + 8, 1, rx_vl,
                                -1, DTN_NET_A, g_frame, sizeof g_frame);
    return n > 0 ? (size_t)n : 0;
}

/* The legs config1 declares, read back out of the machinery rather than
 * restated: what is checked is that the pairing found the right runs. */
static void test_legs_from_profile(const vl_profile_t *p, const dtn_leg_config_t *cfg)
{
    dtn_legs_t *legs = dtn_legs_create(p, cfg, &g_prbs, NULL);

    check(legs != NULL, "config1's legs are read out of its profile");
    if (!legs)
        return;
    check(dtn_legs_count(legs) == 2, "two legs, one per copper link");

    /* The frame the sender will build, and the rate it works out to. */
    double fps = 0.0, per_vl = 0.0;
    const bool ok = dtn_legs_rate_plan(legs, &fps, &per_vl);

    check(fps > 8000.0 && fps < 8500.0,
          "100 Mbit/s of 1513-byte frames is about 8260 frames a second");
    check(per_vl > 130.0 && per_vl < 145.0, "which is about 138 a second per VL");
    check(ok, "and that is inside what BAG 1 ms allows");

    /* A rate that would break BAG has to be reported, not silently policed by
     * the device into something that looks like loss. */
    dtn_leg_config_t fast = *cfg;
    fast.target_mbps = 2000.0;
    dtn_legs_t *hot = dtn_legs_create(p, &fast, &g_prbs, NULL);
    check(hot && !dtn_legs_rate_plan(hot, NULL, &per_vl),
          "a rate past BAG is refused rather than left to the device");
    dtn_legs_destroy(hot);

    dtn_legs_destroy(legs);
    printf("[ OK ] the legs, and the rate they work out to\n");
}

static void test_good_and_bad(const vl_profile_t *p, const dtn_leg_config_t *cfg)
{
    dtn_legs_t *legs = dtn_legs_create(p, cfg, &g_prbs, NULL);
    size_t len;

    /* Leg 0 is copper 32 out of fibre port 8, returning on VL 4024. */
    len = returned_frame(cfg, 8, 4024, 0);
    check(len > 0, "a returned frame is built");
    check(dtn_legs_ingest(legs, 32, g_frame, len), "and the leg claims it");

    const dtn_leg_stats_t *st = dtn_legs_stats(legs, 0);
    check(st->rx_frames == 1 && st->good == 1 && st->bad == 0, "it verifies");
    check(st->lost == 0, "the first frame of a VL is never loss");
    check(st->bit_errors == 0, "and a good frame contributes no bit errors");
    check(dtn_legs_stats(legs, 1)->rx_frames == 0, "nothing landed on the other leg");

    /* One flipped bit in the payload. */
    len = returned_frame(cfg, 8, 4025, 0);
    g_frame[42 + 8 + 100] ^= 0x01;
    dtn_legs_ingest(legs, 32, g_frame, len);
    check(st->bad == 1, "a corrupted payload is bad");
    check(st->bit_errors == 1, "and one flipped bit is one bit error");

    dtn_legs_destroy(legs);
    printf("[ OK ] a returned frame verifies, a corrupted one does not\n");
}

static void test_loss_and_order(const vl_profile_t *p, const dtn_leg_config_t *cfg)
{
    dtn_legs_t *legs = dtn_legs_create(p, cfg, &g_prbs, NULL);
    const dtn_leg_stats_t *st = dtn_legs_stats(legs, 0);
    size_t len;

#define FEED(vl, seq) do { \
        len = returned_frame(cfg, 8, (vl), (seq)); \
        dtn_legs_ingest(legs, 32, g_frame, len); \
    } while (0)

    FEED(4024, 0);
    FEED(4024, 1);
    check(st->lost == 0 && st->late == 0, "a continuous stream is clean");

    FEED(4024, 4);                    /* 2 and 3 never arrived */
    check(st->lost == 2, "a gap of two is two lost");

    FEED(4024, 5);
    check(st->lost == 2, "and is not counted twice");

    FEED(4024, 3);                    /* the late one */
    check(st->lost == 2, "a late frame is not loss");
    check(st->late == 1, "it is counted as out of order");

    FEED(4024, 6);
    check(st->lost == 2, "and does not make the next one look like a fresh gap");

    /* Each VL keeps its own sequence, so another VL starting at 0 is not a gap. */
    FEED(4030, 0);
    check(st->lost == 2, "another VL starts its own sequence");
    check(st->good == 7, "every one of the seven verified");

#undef FEED
    dtn_legs_destroy(legs);
    printf("[ OK ] loss counts once, reordering is not loss\n");
}

/* What is not a leg's. The health monitor shares these links, so a frame the
 * legs do not claim has to come back unclaimed - otherwise the health monitor
 * loses it. */
static void test_not_ours(const vl_profile_t *p, const dtn_leg_config_t *cfg)
{
    dtn_legs_t *legs = dtn_legs_create(p, cfg, &g_prbs, NULL);
    const dtn_leg_stats_t *st = dtn_legs_stats(legs, 0);
    size_t len;

    /* The DTN's own health monitor, and the VMC's. */
    len = returned_frame(cfg, 34, DTN_HEALTH_MONITOR_VL, 0);
    check(!dtn_legs_ingest(legs, 32, g_frame, len),
          "the DTN's own health monitor is not a leg's");
    len = returned_frame(cfg, 8, 100, 0);
    check(!dtn_legs_ingest(legs, 32, g_frame, len),
          "nor is the VMC's on VL 100");

    /* An outbound VL coming back at us - our own frame, looped somewhere. It is
     * not a return VL, so it is not claimed. */
    len = returned_frame(cfg, 32, 3024, 0);
    check(!dtn_legs_ingest(legs, 32, g_frame, len),
          "nor is an outbound VL");

    /* A return VL arriving on the wrong copper link: that is what a swapped pair
     * of cables looks like, and claiming it would file it under the wrong leg. */
    len = returned_frame(cfg, 8, 4024, 0);
    check(!dtn_legs_ingest(legs, 33, g_frame, len),
          "a return VL on the wrong copper link is not claimed");
    check(st->rx_frames == 0, "and leaves the leg's counters alone");

    /* A return VL with something else in it. Claimed - it is this leg's VL - but
     * counted rather than verified, because verifying it would compare the wrong
     * bytes and report a fault in the unit. */
    len = returned_frame(cfg, 8, 4024, 0);
    check(dtn_legs_ingest(legs, 32, g_frame, len - 200),
          "a short frame on a return VL is still the leg's");
    check(st->wrong_length == 1, "and is counted as the wrong length");
    check(st->good == 0 && st->bad == 0, "not verified either way");

    dtn_legs_destroy(legs);
    printf("[ OK ] health-monitor, outbound and misrouted frames are left alone\n");
}

/* The frame on the wire. The sequence is big-endian so a capture reads
 * sensibly, the AFDX byte sits outside the IP length as everything else on this
 * rig does, and the source IP names the port it came from. */
static void test_frame_shape(const dtn_leg_config_t *cfg)
{
    const size_t prbs_len = dtn_legs_prbs_stride(cfg);
    uint8_t payload[2048];

    check(prbs_len == (size_t)cfg->frame_bytes - 42 - 1 - 8,
          "the PRBS fills what is left after the headers, the sequence and the "
          "AFDX byte");

    put_be64(payload, 0x0102030405060708ull);
    memset(payload + 8, 0xA5, prbs_len);

    int n = dtn_build_frame_from(32, payload, prbs_len + 8, 7, 3024, -1,
                                DTN_NET_A, g_frame, sizeof g_frame);

    check(n == cfg->frame_bytes, "the frame is the configured length");
    check(g_frame[0] == 0x03 && g_frame[4] == (3024 >> 8) &&
          g_frame[5] == (3024 & 0xFF), "the VL id is in the destination MAC");
    check(g_frame[14 + 12] == 10 && g_frame[14 + 13] == 1 &&
          g_frame[14 + 14] == 32 && g_frame[14 + 15] == 1,
          "the source IP names the copper port it came from");
    const uint16_t ip_len = (uint16_t)((g_frame[16] << 8) | g_frame[17]);
    check(ip_len == 20 + 8 + prbs_len + 8,
          "the IP length counts the payload and not the AFDX byte");
    check((size_t)n == 42 + prbs_len + 8 + 1,
          "which is why the frame is one byte longer than the IP says");
    check(g_frame[n - 1] == 7, "the AFDX sequence byte is last");
    check(memcmp(g_frame + 42, payload, prbs_len + 8) == 0,
          "and the payload goes on the wire as it was built");

    /* The configuration path must keep the reference's source IP: it is part of
     * the only frame sequence real hardware is known to have accepted. */
    n = dtn_build_frame(payload, 16, 1, 0x2600, -1, DTN_NET_A, g_frame, sizeof g_frame);
    check(n > 0 && g_frame[14 + 14] == 33,
          "the configuration path still sources from 10.1.33.1");
    printf("[ OK ] the frame on the wire\n");
}

int main(void)
{
    const vl_profile_t *p = config1();
    const dtn_leg_config_t *cfg = app_config_dtn_legs();

    if (!p) {
        puts("[FAIL] there is no config1 profile");
        return 1;
    }
    if (p->comm_count == 0) {
        puts("[FAIL] config1 declares no copper legs");
        return 1;
    }

    prbs31_fill(g_prbs_bytes, sizeof g_prbs_bytes, PRBS31_INITIAL_STATE);
    g_prbs.bytes = g_prbs_bytes;
    g_prbs.stride = dtn_legs_prbs_stride(cfg);
    g_prbs.initial_state = PRBS31_INITIAL_STATE;
    g_prbs.ready = true;

    test_frame_shape(cfg);
    test_legs_from_profile(p, cfg);
    test_good_and_bad(p, cfg);
    test_loss_and_order(p, cfg);
    test_not_ours(p, cfg);

    if (failures) {
        printf("FAILED: %d check(s)\n", failures);
        return 1;
    }
    puts("PASS: the copper legs send what they should and account for what returns");
    return 0;
}

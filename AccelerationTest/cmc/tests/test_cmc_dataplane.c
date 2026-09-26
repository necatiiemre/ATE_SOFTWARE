/**
 * @file test_cmc_dataplane.c
 * @brief The receive path, driven with frames instead of a wire.
 *
 * The test builds what the CMC sends back - the transform is written out here
 * rather than called, so that the verifier is checked against an independent
 * statement of what the unit does rather than against itself - and then feeds it
 * in and checks the accounting: a good packet, each of the four ways one can go
 * bad, a gap counted as loss once, a reordering counted as nothing, and the
 * traffic that is not data-plane traffic going where it belongs.
 */

#include "AppConfig.h"
#include "CmcDataPlane.h"
#include "CmcPacket.h"
#include "CmcPayloadVerify.h"
#include "CmcStats.h"
#include "CmcVerify.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

static void check(bool ok, const char *what)
{
    if (!ok) {
        printf("[FAIL] %s\n", what);
        failures++;
    }
}

/* A cache covering the first few sequences, so the test does not generate a
 * quarter of a gigabyte to look at four packets. */
#define TEST_SEQS 8
static uint8_t g_prbs_bytes[CMC_NUM_PRBS_BYTES * (TEST_SEQS + 1)];
static cmc_prbs_cache_t g_prbs;

/* The health-monitor and MMMS sink, counting rather than decoding. */
static unsigned g_hm_calls, g_mmms_calls;
static uint16_t g_hm_last_vl;
static uint16_t g_hm_last_len;

#define TEST_HM_VL   8009
#define TEST_MMMS_VL 8010

static bool is_hm_vl(uint16_t vl_id) { return vl_id == TEST_HM_VL; }
static void on_hm(uint16_t vl_id, const uint8_t *payload, uint16_t len)
{
    (void)payload;
    g_hm_calls++;
    g_hm_last_vl = vl_id;
    g_hm_last_len = len;
}
static void on_mmms(const uint8_t *payload, uint16_t len)
{
    (void)payload; (void)len;
    g_mmms_calls++;
}

/* ------------------------------------------------------------------ */

/* What the CMC does to the payload it was given, written out from the spec in
 * CmcVerify.h. If this and the verifier ever disagree, one of them is wrong and
 * the test says so - which is the whole point of not reusing the verifier's
 * own helpers for the CRC and the XOR. */
static void apply_cmc_transform(uint8_t *payload, const uint8_t *prbs_exp,
                                uint64_t seq)
{
    uint8_t sm[SPLITMIX_XOR_BYTES];

    build_expected_splitmix(sm, seq, prbs_exp);
    memcpy(payload + CMC_SEQ_BYTES, sm, SPLITMIX_XOR_BYTES);

    const uint32_t crc = sw_crc32c(payload, CMC_SEQ_BYTES + SPLITMIX_XOR_BYTES);
    const uint32_t be  = __builtin_bswap32(crc);

    memcpy(payload + CMC_SEQ_BYTES + SPLITMIX_XOR_BYTES, &be, sizeof be);

    /* The chain {6,7,8,13,15}, applied one constant at a time, so that the
     * folded mask the verifier uses is checked rather than assumed. */
    static const uint8_t chain[] = {6, 7, 8, 13, 15};
    uint8_t *xor_byte = payload + CMC_SEQ_BYTES + SPLITMIX_XOR_BYTES + SPLITMIX_CRC_BYTES;
    for (size_t i = 0; i < sizeof chain; i++)
        *xor_byte ^= chain[i];
}

/* One frame as it comes back from the CMC: the VL id shifted into the return
 * range, the payload transformed, the DTN_SEQ byte left where the sender put
 * it. */
static size_t build_returned_frame(uint8_t *frame, const cmc_config_t *c,
                                   uint16_t vl_offset, uint64_t seq)
{
    cmc_packet_config_t cfg;

    cmc_packet_config_init(&cfg);
    cfg.vlan_tagged = c->vlan_tagged;
    cfg.vl_id = (uint16_t)(c->rx_vl_start + vl_offset);

    const size_t len = cmc_packet_build(frame, &cfg);

    cmc_packet_fill_payload(frame, c->vlan_tagged, &g_prbs, seq);
    cmc_packet_stamp_dtn_seq(frame, len, seq);
    apply_cmc_transform(frame + CMC_PAYLOAD_OFF(c->vlan_tagged),
                        cmc_prbs_at(&g_prbs, seq), seq);
    return len;
}

/* A frame on the same link that is not data-plane traffic. */
static size_t build_other_frame(uint8_t *frame, const cmc_config_t *c,
                                uint16_t vl_id, size_t payload_len)
{
    const size_t off = CMC_PAYLOAD_OFF(c->vlan_tagged);

    memset(frame, 0, off + payload_len);
    frame[0] = 0x03;
    frame[4] = (uint8_t)(vl_id >> 8);
    frame[5] = (uint8_t)vl_id;
    if (c->vlan_tagged) {
        frame[12] = 0x81; frame[13] = 0x00;
        frame[16] = 0x08; frame[17] = 0x00;
    } else {
        frame[12] = 0x08; frame[13] = 0x00;
    }
    return off + payload_len;
}

/* ------------------------------------------------------------------ */

static void test_good_packet(cmc_data_plane_t *dp, const cmc_config_t *c)
{
    uint8_t frame[CMC_FRAME_LEN_MAX];
    const size_t len = build_returned_frame(frame, c, 0, 0);

    check(cmc_data_plane_ingest(dp, 0, frame, len), "a returned frame verifies");

    const cmc_net_stats_t *st = cmc_data_plane_stats(dp, 0);
    check(st->good == 1 && st->bad == 0, "and is counted good");
    check(st->total_rx_pkts == 1, "once");
    check(st->frames == 1 && st->frame_bytes == len,
          "and its bytes land in the link counters");
    check(st->lost == 0, "the first packet of a VL is never loss");
    check(st->bit_errors == 0, "and a good packet contributes no bit errors");

    /* The other network has its own everything. */
    check(cmc_data_plane_stats(dp, 1)->frames == 0,
          "nothing landed on the other network");
    printf("[ OK ] a returned frame verifies and is counted once\n");
}

/* Each of the four zones, broken on its own, has to be named on its own -
 * that is what the SplitMix64 / CRC32 / XOR Fail columns are for, and a
 * mislabelled failure sends somebody looking at the wrong firmware. */
static void test_each_failure(const cmc_config_t *c)
{
    struct {
        const char *what;
        size_t      offset;      /* into the payload */
        bool        expect_sm, expect_crc, expect_xor, expect_prbs;
    } cases[] = {
        {"the SplitMix zone", CMC_SEQ_BYTES + 3,                     false, false, true,  true },
        {"the CRC",           CMC_SEQ_BYTES + SPLITMIX_XOR_BYTES + 1, true,  false, true,  true },
        {"the XOR byte",      CMC_SEQ_BYTES + SPLITMIX_XOR_BYTES
                              + SPLITMIX_CRC_BYTES,                   true,  true,  false, true },
        {"the PRBS zone",     CMC_SEQ_BYTES + SPLITMIX_TOTAL_OVERHEAD + 40,
                                                                      true,  true,  true,  false},
    };

    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        uint8_t frame[CMC_FRAME_LEN_MAX];
        const size_t len = build_returned_frame(frame, c, 1, 1);
        uint8_t *payload = frame + CMC_PAYLOAD_OFF(c->vlan_tagged);

        (void)len;
        payload[cases[i].offset] ^= 0x01;

        cmc_verify_t v;
        const bool ok = cmc_verify_payload(payload, cmc_prbs_at(&g_prbs, 1), &v);

        check(!ok, "a corrupted payload fails");
        check(v.splitmix_ok == cases[i].expect_sm &&
              v.crc_ok == cases[i].expect_crc &&
              v.xor_ok == cases[i].expect_xor &&
              v.prbs_ok == cases[i].expect_prbs,
              cases[i].what);
        check(v.bit_errors >= 1, "and one flipped bit is counted as at least one");
    }
    printf("[ OK ] each zone's failure is named on its own\n");
}

/* The loss arithmetic. A gap is counted once, at the packet that reveals it;
 * the packet that arrives late afterwards is not loss and must not undo it. */
static void test_loss_and_reordering(const cmc_config_t *c)
{
    cmc_sink_t sink = {0};
    cmc_data_plane_t *dp = cmc_data_plane_create(c, &g_prbs, &sink, NULL);
    uint8_t frame[CMC_FRAME_LEN_MAX];
    size_t len;

    check(dp != NULL, "a second data plane for the loss cases");

    len = build_returned_frame(frame, c, 0, 0);
    cmc_data_plane_ingest(dp, 0, frame, len);
    check(cmc_data_plane_stats(dp, 0)->lost == 0, "sequence 0 is not loss");

    len = build_returned_frame(frame, c, 0, 1);
    cmc_data_plane_ingest(dp, 0, frame, len);
    check(cmc_data_plane_stats(dp, 0)->lost == 0, "nor is the one after it");

    /* Sequence 2 and 3 never arrive. */
    len = build_returned_frame(frame, c, 0, 4);
    cmc_data_plane_ingest(dp, 0, frame, len);
    check(cmc_data_plane_stats(dp, 0)->lost == 2, "a gap of two is two lost");

    len = build_returned_frame(frame, c, 0, 5);
    cmc_data_plane_ingest(dp, 0, frame, len);
    check(cmc_data_plane_stats(dp, 0)->lost == 2, "and is not counted twice");

    /* The late one. It is behind what is expected, so it adds nothing - and it
     * must not push the expectation backwards either, or the next packet would
     * be read as a fresh gap. */
    len = build_returned_frame(frame, c, 0, 3);
    cmc_data_plane_ingest(dp, 0, frame, len);
    check(cmc_data_plane_stats(dp, 0)->lost == 2, "a late packet is not loss");

    len = build_returned_frame(frame, c, 0, 6);
    cmc_data_plane_ingest(dp, 0, frame, len);
    check(cmc_data_plane_stats(dp, 0)->lost == 2,
          "and does not make the next one look like one");
    check(cmc_data_plane_stats(dp, 0)->good == 6, "all six verified");

    /* Each VL keeps its own sequence, so a different VL starting at 0 is not a
     * gap of six. */
    len = build_returned_frame(frame, c, 7, 0);
    cmc_data_plane_ingest(dp, 0, frame, len);
    check(cmc_data_plane_stats(dp, 0)->lost == 2,
          "another VL starts its own sequence");

    cmc_data_plane_destroy(dp);
    printf("[ OK ] loss is counted once and reordering is not loss\n");
}

/* What shares the link. The health monitor's frames are far shorter than a
 * data-plane frame, so they have to leave the path before the length filter -
 * getting that order wrong is how a health monitor disappears into a
 * short-packet counter. */
static void test_other_traffic(const cmc_config_t *c)
{
    cmc_sink_t sink = {
        .is_health_vl = is_hm_vl,
        .health = on_hm,
        .mmms_vl_id = TEST_MMMS_VL,
        .mmms = on_mmms,
    };
    cmc_data_plane_t *dp = cmc_data_plane_create(c, &g_prbs, &sink, NULL);
    uint8_t frame[CMC_FRAME_LEN_MAX];
    size_t len;

    g_hm_calls = g_mmms_calls = 0;

    len = build_other_frame(frame, c, TEST_HM_VL, 130);
    check(!cmc_data_plane_ingest(dp, 0, frame, len),
          "a health-monitor frame is not a data-plane packet");
    check(g_hm_calls == 1 && g_hm_last_vl == TEST_HM_VL && g_hm_last_len == 130,
          "and goes to the health monitor with its payload length");
    check(cmc_data_plane_stats(dp, 0)->short_pkts == 0,
          "not into the short-packet counter");
    check(cmc_data_plane_stats(dp, 0)->health_frames == 1, "counted as its own thing");

    /* MMMS only while the handover is running. Before it, the same frame is
     * just a short frame - which is the reference's behaviour too, its MMMS
     * branch being guarded by the same flag - so it lands in the short-packet
     * counter rather than being dispatched early. */
    len = build_other_frame(frame, c, TEST_MMMS_VL, 136);
    cmc_data_plane_ingest(dp, 0, frame, len);
    check(g_mmms_calls == 0, "MMMS traffic is ignored while the test is running");
    check(cmc_data_plane_stats(dp, 0)->short_pkts == 1,
          "and is counted as a short frame instead");

    cmc_data_plane_pause_tx(dp, true);
    cmc_data_plane_ingest(dp, 0, frame, len);
    check(g_mmms_calls == 1, "and taken once the handover starts");
    check(cmc_data_plane_stats(dp, 0)->short_pkts == 1,
          "without being counted short as well");
    cmc_data_plane_pause_tx(dp, false);

    /* A frame too short to be anything. */
    len = build_other_frame(frame, c, (uint16_t)(c->rx_vl_start), 64);
    cmc_data_plane_ingest(dp, 0, frame, len);
    check(cmc_data_plane_stats(dp, 0)->short_pkts == 2, "a short frame is short");

    /* A VL id that belongs to nothing on this link. */
    len = build_returned_frame(frame, c, 0, 0);
    frame[4] = 0x44; frame[5] = 0x44;
    cmc_data_plane_ingest(dp, 0, frame, len);
    check(cmc_data_plane_stats(dp, 0)->other_vl == 1 &&
          cmc_data_plane_stats(dp, 0)->last_other_vl == 0x4444,
          "an unexpected VL id is named, not silently dropped");
    check(cmc_data_plane_stats(dp, 0)->total_rx_pkts == 0,
          "and none of it reached verification");

    cmc_data_plane_destroy(dp);
    printf("[ OK ] health-monitor, MMMS, short and unexpected frames\n");
}

/* The rate split: the configured target is for the test, and each network
 * carries half, because the sender emits one frame per network per slot. */
static void test_rate(const cmc_config_t *c)
{
    cmc_sink_t sink = {0};
    cmc_data_plane_t *dp = cmc_data_plane_create(c, &g_prbs, &sink, NULL);

    check(cmc_data_plane_net_gbps(dp) == c->target_gbps / 2.0,
          "each network is paced at half the configured target");
    cmc_data_plane_destroy(dp);
    printf("[ OK ] the rate split\n");
}

/* The tables, over a state with something in every column. What they print is
 * for the eye on the rig; what is checked here is that they run, which is what
 * a format string one argument short does not. */
static void test_tables(const cmc_config_t *c)
{
    cmc_sink_t sink = {0};
    cmc_data_plane_t *dp = cmc_data_plane_create(c, &g_prbs, &sink, NULL);
    cmc_stats_view_t view;
    uint8_t frame[CMC_FRAME_LEN_MAX];

    cmc_stats_view_reset(&view);

    for (uint8_t n = 0; n < c->net_count; n++) {
        size_t len = build_returned_frame(frame, c, 0, 0);

        cmc_data_plane_ingest(dp, n, frame, len);

        /* A gap, and a packet the unit got wrong, so loss, bad, the three fail
         * columns and the BER all have something in them. */
        len = build_returned_frame(frame, c, 0, 3);
        frame[CMC_PAYLOAD_OFF(c->vlan_tagged) + CMC_SEQ_BYTES + 2] ^= 0xFF;
        cmc_data_plane_ingest(dp, n, frame, len);
    }

    cmc_stats_print_banner(c, false, 17);
    cmc_stats_print_all(&view, dp, c);
    cmc_stats_print_banner(c, true, 240);
    cmc_stats_print_all(&view, dp, c);
    cmc_stats_print_warnings(dp, c);
    for (uint8_t n = 0; n < c->net_count; n++)
        cmc_stats_log_net(dp, c, n);

    check(cmc_data_plane_stats(dp, 0)->lost == 2 &&
          cmc_data_plane_stats(dp, 0)->bad == 1,
          "the state the tables were printed over is the one intended");

    cmc_data_plane_destroy(dp);
    printf("[ OK ] the loss tables run over a filled state\n");
}

int main(void)
{
    const cmc_config_t *c = app_config_cmc();

    cmc_prbs_fill(g_prbs_bytes, sizeof g_prbs_bytes, CMC_PRBS_INITIAL_STATE);
    g_prbs.bytes = g_prbs_bytes;
    g_prbs.initial_state = CMC_PRBS_INITIAL_STATE;
    g_prbs.ready = true;

    cmc_sink_t sink = {0};
    cmc_data_plane_t *dp = cmc_data_plane_create(c, &g_prbs, &sink, NULL);

    if (!dp) {
        puts("[FAIL] the data plane would not allocate");
        return 1;
    }

    test_good_packet(dp, c);
    test_each_failure(c);
    test_loss_and_reordering(c);
    test_other_traffic(c);
    test_rate(c);
    test_tables(c);

    cmc_data_plane_destroy(dp);

    if (failures) {
        printf("FAILED: %d check(s)\n", failures);
        return 1;
    }
    puts("PASS: the CMC receive path accounts for what comes back");
    return 0;
}

/**
 * @file test_cmc_packet.c
 * @brief The CMC data-plane frame, field by field, and the PRBS stream.
 *
 * The frame is the one thing in this test that the unit has to recognise, so
 * every field is pinned rather than described: a change here is a change the
 * CMC would see. The PRBS stream is pinned at its first bytes for the same
 * reason - it is the shared secret between the two ends, and if it drifts, the
 * loss tables fill with bad packets that are not the unit's fault.
 */

#include "AppConfig.h"
#include "CmcPacket.h"
#include "CmcPayloadVerify.h"

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

static uint16_t be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }
static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

/* The first sixteen bytes of PRBS-31 from the reference's initial state. The
 * generator walks one bit at a time and packs eight to a byte most significant
 * first; get either of those backwards and these bytes change. */
static void test_prbs(void)
{
    static const uint8_t expected[16] = {
        0xf0, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x0f,
        0xc0, 0x00, 0x00, 0xe3, 0x80, 0x00, 0x0f, 0xff,
    };
    uint8_t got[16];

    uint32_t after = prbs31_fill(got, sizeof got, CMC_PRBS_INITIAL_STATE);

    check(memcmp(got, expected, sizeof got) == 0,
          "PRBS-31 from state 0x0F, byte for byte");
    check(after == 0x70070000u, "and the state it leaves behind");

    /* Chunking must not change the stream: the cache is generated in pieces
     * with the state carried across, and that has to be the same stream. */
    uint8_t half_a[8], half_b[8];
    uint32_t mid = prbs31_fill(half_a, sizeof half_a, CMC_PRBS_INITIAL_STATE);
    prbs31_fill(half_b, sizeof half_b, mid);
    check(memcmp(half_a, expected, 8) == 0 && memcmp(half_b, expected + 8, 8) == 0,
          "generating it in two pieces gives the same bytes as one");
    printf("[ OK ] the PRBS-31 stream\n");
}

/* The sizes are what make the direct-cable frame the frame the unit already
 * answers: the payload keeps the length the reference builds for a tagged
 * frame, so once the tag is off the wire the frame is 1509 bytes - which is
 * exactly what the switch hands the CMC today. */
static void test_sizes(void)
{
    check(CMC_PAYLOAD_SIZE == 1467, "the payload is 1467 bytes");
    check(CMC_NUM_PRBS_BYTES == 1459, "of which 1459 are PRBS");
    check(CMC_FRAME_LEN_UNTAGGED == 1509, "an untagged frame is 1509 bytes");
    check(CMC_FRAME_LEN_TAGGED == 1513, "a tagged one is 1513");
    check(CMC_PAYLOAD_OFF(false) == 42 && CMC_PAYLOAD_OFF(true) == 46,
          "and the payload starts at 42 or 46");
    check(SPLITMIX_MIN_PAYLOAD == 77,
          "the CMC's return-path overhead ends at byte 77");
    printf("[ OK ] the sizes\n");
}

static void test_frame(void)
{
    cmc_packet_config_t cfg;
    uint8_t frame[CMC_FRAME_LEN_MAX];

    cmc_packet_config_init(&cfg);
    check(cfg.src_mac[0] == 0x02 && cfg.src_mac[1] == 0 && cfg.src_mac[2] == 0 &&
          cfg.src_mac[3] == 0 && cfg.src_mac[4] == 0 && cfg.src_mac[5] == 0x20,
          "the source MAC is 02:00:00:00:00:20");
    check(cfg.src_ip == 0x0A000000u, "the source IP is 10.0.0.0");
    check(cfg.ttl == 1 && cfg.tos == 0, "TTL 1, TOS 0");
    check(cfg.src_port == 100 && cfg.dst_port == 100, "UDP 100 to 100");

    cfg.vl_id = 10001;
    size_t len = cmc_packet_build(frame, &cfg);

    check(len == CMC_FRAME_LEN_UNTAGGED, "an untagged frame comes out 1509 bytes");
    check(frame[0] == 0x03 && frame[1] == 0 && frame[2] == 0 && frame[3] == 0,
          "the destination MAC starts 03:00:00:00");
    check(frame[4] == (10001 >> 8) && frame[5] == (10001 & 0xFF),
          "and ends with the VL id");
    check(cmc_vl_id_of(frame) == 10001, "which reads back out of the frame");
    check(memcmp(frame + 6, cfg.src_mac, 6) == 0, "the source MAC is where it goes");
    check(be16(frame + 12) == 0x0800, "no tag, so the ethertype is IPv4");

    const uint8_t *ip = frame + 14;
    check(ip[0] == 0x45, "IPv4, 20-byte header");
    check(be16(ip + 2) == 20 + 8 + CMC_PAYLOAD_SIZE,
          "the IP length counts the reference's payload, tag or no tag");
    check(ip[8] == 1 && ip[9] == 17, "TTL 1 and protocol UDP");
    check(be32(ip + 12) == 0x0A000000u, "source 10.0.0.0");
    check(be32(ip + 16) == ((224u << 24) | (224u << 16) | (10001u & 0xFFFF)),
          "destination 224.224.<VL>");

    /* A correct header sums to zero over its sixteen-bit words, checksum
     * included - that is what a receiver checks, so it is what the test does. */
    uint32_t sum = 0;
    for (int i = 0; i < 20; i += 2)
        sum += be16(ip + i);
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    check(sum == 0xFFFF, "the IP checksum is right");

    const uint8_t *udp = ip + 20;
    check(be16(udp + 0) == 100 && be16(udp + 2) == 100, "UDP 100 to 100 on the wire");
    check(be16(udp + 4) == 8 + CMC_PAYLOAD_SIZE, "the UDP length");
    check(be16(udp + 6) == 0, "and no UDP checksum, as the reference leaves it");

    /* With a tag the same fields move four bytes along and nothing else
     * changes - the payload keeps its length, so the frame grows by the tag. */
    cfg.vlan_tagged = true;
    cfg.vlan_id = 97;
    len = cmc_packet_build(frame, &cfg);
    check(len == CMC_FRAME_LEN_TAGGED, "a tagged frame is four bytes longer");
    check(be16(frame + 12) == 0x8100 && (be16(frame + 14) & 0x0FFF) == 97 &&
          be16(frame + 16) == 0x0800, "with the 802.1Q tag in between");
    check(be16(frame + 18 + 2) == 20 + 8 + CMC_PAYLOAD_SIZE,
          "and the same IP length");
    printf("[ OK ] the frame, field by field\n");
}

static void test_payload(void)
{
    prbs31_cache_t cache = {0};
    cmc_packet_config_t cfg;
    uint8_t frame[CMC_FRAME_LEN_MAX];
    uint8_t prbs[CMC_NUM_PRBS_BYTES];

    /* The real cache is the whole PRBS period, a quarter of a gigabyte the test
     * has no reason to generate. A short one with the same first bytes is
     * enough to check that the payload is assembled out of it correctly; the
     * offset arithmetic is checked separately below. */
    static uint8_t small[CMC_NUM_PRBS_BYTES * 2];
    prbs31_fill(small, sizeof small, CMC_PRBS_INITIAL_STATE);
    cache.bytes = small;
    cache.initial_state = CMC_PRBS_INITIAL_STATE;
    cache.stride = CMC_PRBS_STRIDE;
    cache.ready = true;

    cmc_packet_config_init(&cfg);
    cfg.vl_id = 10001;
    size_t len = cmc_packet_build(frame, &cfg);

    cmc_packet_fill_payload(frame, false, &cache, 0);
    const uint8_t *payload = frame + CMC_PAYLOAD_OFF(false);

    uint64_t seq_back;
    memcpy(&seq_back, payload, sizeof seq_back);
    check(seq_back == 0, "the sequence goes in as a raw 64-bit word");

    prbs31_fill(prbs, sizeof prbs, CMC_PRBS_INITIAL_STATE);
    check(memcmp(payload + CMC_SEQ_BYTES, prbs, CMC_NUM_PRBS_BYTES) == 0,
          "and sequence 0 carries the stream from its start");

    /* DTN_SEQ: zero only ever for the first packet, then 1..255 round and
     * round, never back to zero. */
    check(cmc_dtn_seq(0) == 0, "DTN_SEQ 0 for sequence 0");
    check(cmc_dtn_seq(1) == 1 && cmc_dtn_seq(255) == 255,
          "then it follows the sequence");
    check(cmc_dtn_seq(256) == 1 && cmc_dtn_seq(257) == 2,
          "and wraps to 1, not to 0");
    check(cmc_dtn_seq(510) == 255 && cmc_dtn_seq(511) == 1, "every 255, not 256");

    cmc_packet_stamp_dtn_seq(frame, len, 256);
    check(frame[len - 1] == 1, "the stamp lands on the last payload byte");
    printf("[ OK ] the payload and its DTN_SEQ byte\n");
}

/* The offset a sequence reads the stream at. The receiver recomputes this from
 * the sequence alone, so the two sides agree only if the arithmetic matches -
 * including the wrap, which the cache's repeated tail is there to absorb. */
static void test_prbs_offset(void)
{
    prbs31_cache_t cache = {0};
    /* Only the pointer arithmetic is under test here, never the bytes, so the
     * buffer is sized for the arithmetic rather than for a read. Big enough that
     * the compiler does not warn about offsets it can see are past the end. */
    static uint8_t small[CMC_NUM_PRBS_BYTES * 2];

    cache.bytes = small;
    cache.stride = CMC_PRBS_STRIDE;
    cache.ready = true;

    check(prbs31_at(&cache, 0) == small, "sequence 0 reads from the start");
    check(prbs31_at(&cache, 1) == small + CMC_NUM_PRBS_BYTES,
          "and each sequence one packet further along");

    /* The offset wraps with the period, which is not a whole number of
     * packets, so the last packet of a period starts inside it and runs past
     * its end. That overrun is what the repeated tail absorbs: the worst case
     * is the highest offset the modulo can give, and it has to still leave a
     * packet's worth of buffer behind it. */
    const uint64_t per_period = (uint64_t)CMC_PRBS_CACHE_SIZE / CMC_NUM_PRBS_BYTES;
    const uint64_t off_last = (per_period * CMC_NUM_PRBS_BYTES) % CMC_PRBS_CACHE_SIZE;
    check(prbs31_at(&cache, per_period) == small + off_last,
          "the offset wraps with the period, not with the buffer");
    check(off_last + CMC_NUM_PRBS_BYTES > CMC_PRBS_CACHE_SIZE,
          "the last packet of a period does run off the end");
    check((CMC_PRBS_CACHE_SIZE - 1) + CMC_NUM_PRBS_BYTES <= (PRBS31_CACHE_SIZE + CMC_PRBS_STRIDE),
          "and the buffer covers the worst offset the modulo can give");
    printf("[ OK ] the PRBS offset a sequence reads at\n");
}

/* The return path's own arithmetic, taken from the reference's own captured
 * packet: a real SMMM frame whose sequence bytes were 00 00 00 00 00 01 63 01
 * over 1024 bytes of 0x01 carried CRC C0 76 40 C6, and this table is the table
 * that produces it. If this check fails the copy has been "fixed" into
 * standard CRC-32C and both the data plane and the PMM lines are broken. */
static void test_crc(void)
{
    uint8_t msg[8 + 1024];

    memset(msg, 0, 8);
    msg[5] = 0x01; msg[6] = 0x63; msg[7] = 0x01;
    memset(msg + 8, 0x01, 1024);

    check(sw_crc32c(msg, sizeof msg) == 0xC07640C6u,
          "the CRC table is the unit's, not the standard one");
    check(sw_crc32c("123456789", 9) == 0x2E6C10CCu,
          "and it gives the reference's check value, not 0xE3069283");
    check(XOR_ZONE_MASK == 0x0B, "the XOR-zone chain folds to 0x0B");
    printf("[ OK ] the CRC and XOR-zone constants\n");
}

/* The config is the one place the rig lives, so the test reads it rather than
 * repeating it: what is checked is that the four interfaces are there, in the
 * order the cables are in, and that the two networks are told apart by the
 * source MAC tail the CMC expects. */
static void test_config(void)
{
    const cmc_config_t *c = app_config_cmc();

    check(c->net_count == 2 && c->pmm_count == 2,
          "two DSM networks and two PMM lines");
    check(strcmp(c->nets[0].unit_label, "DSMA") == 0 &&
          strcmp(c->nets[1].unit_label, "DSMB") == 0,
          "the first interface is DSM-A and the second DSM-B");
    check(strcmp(c->pmms[0].label, "PMM1") == 0 &&
          strcmp(c->pmms[1].label, "PMM2") == 0,
          "then PMM1 and PMM2");
    check(c->nets[0].src_mac_tail == 0x20 && c->nets[1].src_mac_tail == 0x40,
          "network A is source MAC tail 0x20 and B is 0x40");
    check(c->tx_vl_start == 10001 && c->rx_vl_start == 10521 && c->vl_count == 104,
          "104 VLs, 10001 out and 10521 back");
    check(!c->vlan_tagged, "and the frames are untagged, because the cables are direct");
    printf("[ OK ] the rig comes from AppConfig\n");
}

int main(void)
{
    test_prbs();
    test_sizes();
    test_frame();
    test_payload();
    test_prbs_offset();
    test_crc();
    test_config();

    if (failures) {
        printf("FAILED: %d check(s)\n", failures);
        return 1;
    }
    puts("PASS: the CMC frame is the one dpdk_cmc puts on the wire");
    return 0;
}

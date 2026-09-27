/**
 * @file test_cmc_pmm.c
 * @brief The PMM lines, driven with frames.
 *
 * The line is listen-only, so everything that can go wrong with it is a
 * misreading of what arrived: the sequence read from the wrong end, a CRC
 * variant guessed rather than found, a restart counted as a billion lost
 * packets. Each of those is a check here.
 */

#include "AppConfig.h"
#include "PayloadVerify.h"
#include "CmcPmm.h"
#include "CmcStats.h"

#include <arpa/inet.h>
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

static void put_be16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }

static uint16_t ip_checksum(const uint8_t *ip)
{
    uint32_t sum = 0;

    for (int i = 0; i < 20; i += 2)
        sum += (uint32_t)((ip[i] << 8) | ip[i + 1]);
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

/* One SMMM frame as it reaches a PMM interface. The CRC is written the way the
 * verified variant has it - CRC32C over the sequence and the data, big-endian in
 * the field - so that the detector has to land on that variant rather than on
 * one of the seven others. */
static size_t build_frame(uint8_t *frame, const cmc_pmm_link_t *link,
                         uint64_t seq, bool with_hdr, bool good_crc,
                         const char *src_ip_override)
{
    const size_t payload_len = with_hdr ? SMMM_UDP_PAYLOAD_WITH_HDR
                                        : SMMM_UDP_PAYLOAD_NO_HDR;
    const size_t len = 42 + payload_len;
    struct in_addr a;

    memset(frame, 0, len);

    frame[0] = 0x02;
    frame[6] = 0x02;
    put_be16(frame + 12, 0x0800);

    uint8_t *ip = frame + 14;
    ip[0] = 0x45;
    put_be16(ip + 2, (uint16_t)(20 + 8 + payload_len));
    ip[8] = 64;
    ip[9] = 17;
    inet_pton(AF_INET, src_ip_override ? src_ip_override : link->unit_ip, &a);
    memcpy(ip + 12, &a.s_addr, 4);
    inet_pton(AF_INET, link->local_ip, &a);
    memcpy(ip + 16, &a.s_addr, 4);
    put_be16(ip + 10, ip_checksum(ip));

    uint8_t *udp = ip + 20;
    put_be16(udp + 0, link->unit_port);
    put_be16(udp + 2, link->local_port);
    put_be16(udp + 4, (uint16_t)(8 + payload_len));

    uint8_t *payload = udp + 8;
    if (with_hdr) {
        payload[0] = SMMM_PKT_TYPE_IPPP_TO_PMM;
        payload[1] = SMMM_ENC_PLAINTEXT;
        payload += SMMM_UDP_SMMM_HDR_LEN;
    }

    smmm_msg_t msg;
    memset(&msg, 0, sizeof msg);
    msg.seq_num = __builtin_bswap64(seq);           /* big-endian on the wire */
    memset(msg.data, 0x01, sizeof msg.data);

    const uint32_t crc = sw_crc32c(&msg, SMMM_SEQ_NUM_BYTE + SMMM_DATA_BYTE);
    msg.data_crc = __builtin_bswap32(good_crc ? crc : crc ^ 0xFFu);

    memcpy(payload, &msg, sizeof msg);
    return len;
}

/* The one captured packet the CRC variant was established from. If the
 * detector's list is reordered or the table is "corrected", this is what
 * notices. */
static void test_crc_variant(const cmc_config_t *c)
{
    cmc_pmm_t *pmm = cmc_pmm_create(c, NULL);
    uint8_t frame[2048];

    check(pmm != NULL, "the PMM channel allocates");
    check(cmc_pmm_crc_variant(pmm) == NULL, "with no CRC variant yet");

    /* 0x016301 over 1024 bytes of 0x01 is the captured packet; its field held
     * C0 76 40 C6, which is what build_frame computes. */
    const size_t len = build_frame(frame, &c->pmms[0], 0x016301ull, false, true, NULL);

    cmc_pmm_ingest(pmm, 0, frame, len);

    const char *variant = cmc_pmm_crc_variant(pmm);
    check(variant != NULL && strcmp(variant, "CRC32C  seq+data    BE") == 0,
          "the first packet locks onto CRC32C over seq+data, big-endian");

    const cmc_pmm_stats_t *st = cmc_pmm_stats(pmm, 0);
    check(st->rx_pkts == 1 && st->crc_ok == 1 && st->crc_fail == 0,
          "and the packet passes");
    check(st->last_seq == 0x016301ull,
          "the sequence reads as 90881, not as 10^16");
    check(st->addr_mismatch == 0, "its addresses are the ones configured");
    check(st->hdr_present_pkts == 0, "and it carried no SMMM header");
    check(st->lost_pkts == 0, "the first packet of a line is never loss");

    cmc_pmm_destroy(pmm);
    printf("[ OK ] the CRC variant is found, not assumed\n");
}

static void test_sequence(const cmc_config_t *c)
{
    cmc_pmm_t *pmm = cmc_pmm_create(c, NULL);
    uint8_t frame[2048];
    size_t len;

#define FEED(seq) do { \
        len = build_frame(frame, &c->pmms[0], (seq), false, true, NULL); \
        cmc_pmm_ingest(pmm, 0, frame, len); \
    } while (0)

    FEED(100);
    FEED(101);
    const cmc_pmm_stats_t *st = cmc_pmm_stats(pmm, 0);
    check(st->lost_pkts == 0 && st->out_of_order_pkts == 0 && st->duplicate_pkts == 0,
          "a continuous stream is clean");

    FEED(105);
    check(st->lost_pkts == 3, "a gap of three is three lost");

    FEED(105);
    check(st->duplicate_pkts == 1, "the same sequence again is a duplicate");

    FEED(103);
    check(st->out_of_order_pkts == 1, "an earlier one is out of order");
    check(st->lost_pkts == 3, "and neither adds to loss");

    /* A restart. Counting the jump as loss would put a million packets in the
     * column and hide everything real, so it is a resync instead. */
    FEED(50000000ull);
    check(st->resync_events == 1, "a jump too big to be loss is a resync");
    check(st->lost_pkts == 3, "and does not touch the loss column");

    FEED(0);
    check(st->resync_events == 2, "so is a jump that big backwards");

    cmc_pmm_destroy(pmm);
#undef FEED
    printf("[ OK ] loss, reordering, duplicates and restarts\n");
}

static void test_malformed(const cmc_config_t *c)
{
    cmc_pmm_t *pmm = cmc_pmm_create(c, NULL);
    uint8_t frame[2048];
    size_t len;

    /* Lock the variant first, so a CRC failure below is a failure and not a
     * packet that arrived while the variant was still unknown. */
    len = build_frame(frame, &c->pmms[0], 1, false, true, NULL);
    cmc_pmm_ingest(pmm, 0, frame, len);

    const cmc_pmm_stats_t *st = cmc_pmm_stats(pmm, 0);

    len = build_frame(frame, &c->pmms[0], 2, false, false, NULL);
    cmc_pmm_ingest(pmm, 0, frame, len);
    check(st->crc_fail == 1, "a wrong CRC is a CRC failure");

    len = build_frame(frame, &c->pmms[0], 3, true, true, NULL);
    cmc_pmm_ingest(pmm, 0, frame, len);
    check(st->hdr_present_pkts == 1, "the SMMM 2-byte header is recognised");
    check(st->crc_ok == 2, "and the MSG behind it still verifies");

    /* The addresses are a consistency check, not a filter: the packet is still
     * counted, so a wrong assumption in AppConfig.c shows up in the table
     * instead of throwing the line's traffic away. */
    const uint64_t rx_before = st->rx_pkts;
    len = build_frame(frame, &c->pmms[0], 4, false, true, "10.9.9.9");
    cmc_pmm_ingest(pmm, 0, frame, len);
    check(st->addr_mismatch == 1, "a wrong source address is counted");
    check(st->rx_pkts == rx_before + 1 && st->crc_ok == 3,
          "and the packet is still taken");

    /* A payload that is neither 1036 nor 1038. */
    len = build_frame(frame, &c->pmms[0], 5, false, true, NULL);
    put_be16(frame + 14 + 20 + 4, (uint16_t)(8 + 999));
    cmc_pmm_ingest(pmm, 0, frame, len);
    check(st->bad_len_pkts == 1, "a payload of the wrong length is refused");

    /* Not IPv4 at all. */
    len = build_frame(frame, &c->pmms[0], 6, false, true, NULL);
    frame[12] = 0x86; frame[13] = 0xDD;
    const uint64_t rx_then = st->rx_pkts;
    cmc_pmm_ingest(pmm, 0, frame, len);
    check(st->rx_pkts == rx_then, "a non-IPv4 frame is not the line's traffic");

    /* The second line has its own everything. */
    check(cmc_pmm_stats(pmm, 1)->rx_pkts == 0, "and PMM2 saw none of it");

    cmc_pmm_destroy(pmm);
    printf("[ OK ] wrong CRC, wrong length, wrong address, wrong ethertype\n");
}

/* The printers, over a state that has something in every column. Not checking
 * what they print - that is for the eye on the rig - only that they run, which
 * is what a format string with the wrong number of arguments fails. */
static void test_printers(const cmc_config_t *c)
{
    cmc_pmm_t *pmm = cmc_pmm_create(c, NULL);
    uint8_t frame[2048];

    cmc_pmm_print_table(pmm);        /* before anything arrives */

    for (uint8_t l = 0; l < c->pmm_count; l++) {
        size_t len = build_frame(frame, &c->pmms[l], 10, false, true, NULL);
        cmc_pmm_ingest(pmm, l, frame, len);
        len = build_frame(frame, &c->pmms[l], 20, false, false, NULL);
        cmc_pmm_ingest(pmm, l, frame, len);
    }
    cmc_pmm_print_table(pmm);        /* with loss, CRC failures and a variant */

    cmc_pmm_destroy(pmm);
    printf("[ OK ] the PMM table runs over a filled state\n");
}

int main(void)
{
    const cmc_config_t *c = app_config_cmc();

    test_crc_variant(c);
    test_sequence(c);
    test_malformed(c);
    test_printers(c);

    if (failures) {
        printf("FAILED: %d check(s)\n", failures);
        return 1;
    }
    puts("PASS: the PMM lines are counted the way the reference counts them");
    return 0;
}

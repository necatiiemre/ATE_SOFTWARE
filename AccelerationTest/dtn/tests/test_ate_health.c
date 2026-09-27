/*
 * The ATE software's DTN health monitor, as this test drives it.
 *
 * Two things are worth pinning down. The query has to be the ATE software's byte
 * for byte, because it is the frame the device answers; and a cycle's six packets
 * have to land in the right FPGA and the right port slots, because the copy tells
 * them apart by length alone and gets the FPGA of the shorter ones from the last
 * long one it saw. Feed them out of order or drop the long one and the port table
 * is quietly wrong, which is the failure that would not look like a failure.
 *
 * The rendering is not compared against anything: it is the ATE software's own
 * code, and `make ate-diff` is what checks that. What is checked here is that it
 * runs over a filled cycle without reading off the end of it.
 */

#include "AteHealth.h"
#include "DtnConfig.h"
#include "HealthMonitor.h"
#include "HealthTypes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

static void check(bool ok, const char *what)
{
    printf("[%s] %s\n", ok ? " OK " : "FAIL", what);
    if (!ok)
        failures++;
}

static void put_be16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }

static void put_be48(uint8_t *p, uint64_t v)
{
    for (int i = 0; i < 6; i++)
        p[i] = (uint8_t)(v >> (8 * (5 - i)));
}

/* Two bytes follow the last port record in every size the device sends: 1187 is
 * 42 + 111 + 8*129 + 2, 1083 is 42 + 7 + 8*129 + 2, 438 is 42 + 7 + 3*129 + 2, and
 * the 94-byte MCU packet has the same two over its fields. The parser reads by
 * offset and never looks at them, but the lengths are how it tells the packets
 * apart, so a builder that leaves them off builds packets it misreads - which is
 * what this constant is here to stop happening again. */
#define HEALTH_PKT_TAIL 2

/* One health response, built the way the device sends it: Ethernet/IP/UDP, then
 * either a device header or the 7-byte mini header, then 129 bytes per port. */
static size_t build_fpga(uint8_t *frame, size_t cap, bool with_header,
                         uint8_t status_enable, int ports, uint16_t first_port)
{
    const size_t hdr = with_header ? HEALTH_DEVICE_HEADER_SIZE : HEALTH_MINI_HEADER_SIZE;
    const size_t len = HEALTH_UDP_PAYLOAD_OFFSET + hdr
                       + (size_t)ports * HEALTH_PORT_DATA_SIZE + HEALTH_PKT_TAIL;

    if (len > cap)
        abort();
    memset(frame, 0, len);
    /* The VL id lives in the last two bytes of the destination MAC, which is what
     * the caller filters on before handing the frame over. */
    frame[0] = 0x03;
    put_be16(frame + 4, DTN_HEALTH_MONITOR_VL);

    uint8_t *payload = frame + HEALTH_UDP_PAYLOAD_OFFSET;
    put_be16(payload + DEV_OFF_DEVICE_ID, DTN_LRU_ID);
    if (with_header)
        payload[DEV_OFF_STATUS_ENABLE] = status_enable;

    uint8_t *port = payload + hdr;
    for (int i = 0; i < ports; i++) {
        put_be16(port + PORT_OFF_PORT_NUMBER, (uint16_t)(first_port + i));
        put_be48(port + PORT_OFF_RX_COUNT, 1000u + first_port + (unsigned)i);
        put_be48(port + PORT_OFF_VLID_DROP, first_port + (unsigned)i);
        port += HEALTH_PORT_DATA_SIZE;
    }
    return len;
}

static size_t build_mcu(uint8_t *frame, size_t cap)
{
    /* The MCU packet is not identified by an exact length - anything that is not
     * one of the three FPGA sizes and long enough to hold the fields is MCU - so
     * the length the device sends is used. */
    if (HEALTH_PKT_SIZE_MCU > (int)cap)
        abort();
    memset(frame, 0, HEALTH_PKT_SIZE_MCU);
    frame[0] = 0x03;
    put_be16(frame + 4, DTN_HEALTH_MONITOR_VL);
    put_be16(frame + HEALTH_UDP_PAYLOAD_OFFSET + MCU_OFF_DEVICE_ID, DTN_LRU_ID);
    return HEALTH_PKT_SIZE_MCU;
}

static void test_query(void)
{
    uint8_t a[ATE_HEALTH_QUERY_MAX], b[ATE_HEALTH_QUERY_MAX];

    ate_health_reset();
    check(ate_health_build_query(a, sizeof a) == ATE_HEALTH_QUERY_MAX,
          "the query is the ATE software's 64 bytes");
    check(ate_health_build_query(a, 8) < 0, "a buffer too small is refused");

    /* 10.1.33.1 -> 224.224.0.0, UDP 100 -> 100, LRU 0x2600 with a 0x52 read. */
    check(a[12] == 0x08 && a[13] == 0x00, "untagged IPv4");
    check(a[26] == 10 && a[27] == 1 && a[28] == 33 && a[29] == 1, "source 10.1.33.1");
    check(a[30] == 224 && a[31] == 224, "destination 224.224.0.0");
    check(a[34] == 0 && a[35] == 100 && a[36] == 0 && a[37] == 100, "UDP 100 -> 100");
    check(a[42] == 0x26 && a[43] == 0x00 && a[44] == DTN_OP_READ,
          "LRU 0x2600, operation 0x52");
    /* IP total length 50 and UDP length 30 both count the 22-byte payload, so the
     * sequence in the last byte is inside the length - unlike a config frame. */
    check(((a[16] << 8) | a[17]) == 50 && ((a[38] << 8) | a[39]) == 30,
          "the sequence byte is inside the UDP length");

    check(a[ATE_HEALTH_QUERY_MAX - 1] == 0x2F, "the first sequence is 0x2F");
    ate_health_build_query(b, sizeof b);
    check(b[ATE_HEALTH_QUERY_MAX - 1] == 0x30, "and then it counts up");
    check(memcmp(a, b, ATE_HEALTH_QUERY_MAX - 1) == 0,
          "nothing but the sequence changes between queries");

    /* 255 -> 1, never back to 0. */
    ate_health_reset();
    uint8_t seen_zero = 0;
    for (int i = 0; i < 600; i++) {
        ate_health_build_query(b, sizeof b);
        if (b[ATE_HEALTH_QUERY_MAX - 1] == 0 && i > 0)
            seen_zero = 1;
    }
    check(!seen_zero, "0 is only ever the value it starts from");

    uint64_t queries = 0, shorts = 0;
    ate_health_counts(&queries, &shorts);
    check(queries == 600, "every query is counted");
}

static void test_cycle(void)
{
    uint8_t frame[2048];
    size_t len;

    ate_health_reset();
    check(!ate_health_render(), "nothing is drawn before the device has answered");
    check(ate_health_responses() == 0, "and no packets are claimed");

    /* A full cycle, in the order the device sends it: the assistant's two, the
     * manager's three, then the MCU. */
    len = build_fpga(frame, sizeof frame, true, STATUS_ENABLE_ASSISTANT, 8, 0);
    check(len == HEALTH_PKT_SIZE_WITH_HEADER, "a device-header packet is 1187 bytes");
    ate_health_ingest(frame, len);

    len = build_fpga(frame, sizeof frame, false, 0, 8, 8);
    check(len == HEALTH_PKT_SIZE_8_PORTS, "a mini-header packet of 8 ports is 1083");
    ate_health_ingest(frame, len);

    len = build_fpga(frame, sizeof frame, true, STATUS_ENABLE_MANAGER, 8, 16);
    ate_health_ingest(frame, len);
    len = build_fpga(frame, sizeof frame, false, 0, 8, 24);
    ate_health_ingest(frame, len);
    len = build_fpga(frame, sizeof frame, false, 0, 3, 32);
    check(len == HEALTH_PKT_SIZE_3_PORTS, "a mini-header packet of 3 ports is 438");
    ate_health_ingest(frame, len);

    len = build_mcu(frame, sizeof frame);
    ate_health_ingest(frame, len);

    /* Still nothing: the cycle has not closed. */
    check(!ate_health_render(), "a cycle is only shown once it has closed");

    ate_health_cycle();
    check(ate_health_responses() == ATE_HEALTH_EXPECTED_RESPONSES,
          "a full cycle is six packets");

    uint64_t queries = 0, shorts = 0;
    ate_health_counts(&queries, &shorts);
    check(shorts == 0, "and is not counted short");

    /* A silent second rolls over to an empty block rather than leaving the last
     * good one on the screen. */
    ate_health_cycle();
    check(ate_health_responses() == 0, "silence shows as silence");
    ate_health_counts(&queries, &shorts);
    check(shorts == 1, "and is counted as a short cycle");
}

/* The device's counters are absolute, so the baseline is what makes them mean
 * anything. It has to be the first cycle of the run, and it has to survive the
 * cycles after it without moving. */
static void test_baseline(void)
{
    uint8_t frame[2048];

    ate_health_reset();

    /* First cycle: port 0 has carried 1000 frames, port 16 carries 1016. */
    ate_health_ingest(frame, build_fpga(frame, sizeof frame, true,
                                        STATUS_ENABLE_ASSISTANT, 8, 0));
    ate_health_ingest(frame, build_fpga(frame, sizeof frame, true,
                                        STATUS_ENABLE_MANAGER, 8, 16));
    ate_health_cycle();

    uint64_t tx = 0, rx = 0;
    check(health_monitor_get_port_delta(0, &tx, &rx) && rx == 0,
          "the first cycle is the baseline, so nothing has happened yet");

    /* A later cycle, 500 frames further on. */
    size_t len = build_fpga(frame, sizeof frame, true, STATUS_ENABLE_ASSISTANT, 8, 0);
    put_be48(frame + HEALTH_UDP_PAYLOAD_OFFSET + HEALTH_DEVICE_HEADER_SIZE
             + PORT_OFF_RX_COUNT, 1500);
    ate_health_ingest(frame, len);
    ate_health_cycle();

    check(health_monitor_get_port_delta(0, &tx, &rx) && rx == 500,
          "and the delta from it is what the device carried during the run");

    uint64_t tx_base = 0, rx_base = 0, tx_now = 0, rx_now = 0;
    check(health_monitor_get_port_readings(0, &tx_base, &rx_base, &tx_now, &rx_now)
          && rx_base == 1000 && rx_now == 1500,
          "both readings are kept, so either can be matched against a live table");

    puts("\n--- the start and end port tables ---");
    ate_health_render_start_end();
    puts("--- end ---\n");

    ate_health_reset();
    check(!health_monitor_get_port_delta(0, &tx, &rx),
          "a reset forgets the baseline, so a second run does not inherit it");
}

static void test_render(void)
{
    uint8_t frame[2048];

    ate_health_reset();
    ate_health_ingest(frame, build_fpga(frame, sizeof frame, true,
                                        STATUS_ENABLE_ASSISTANT, 8, 0));
    ate_health_ingest(frame, build_fpga(frame, sizeof frame, false, 0, 8, 8));
    ate_health_ingest(frame, build_fpga(frame, sizeof frame, true,
                                        STATUS_ENABLE_MANAGER, 8, 16));
    ate_health_ingest(frame, build_fpga(frame, sizeof frame, false, 0, 8, 24));
    ate_health_ingest(frame, build_fpga(frame, sizeof frame, false, 0, 3, 32));
    ate_health_ingest(frame, build_mcu(frame, sizeof frame));
    ate_health_cycle();

    puts("\n--- what the ATE software prints, over a filled cycle ---");
    check(ate_health_render(), "the block is drawn");
    puts("--- end ---\n");
}

int main(void)
{
    test_query();
    test_cycle();
    test_baseline();
    test_render();

    if (failures) {
        printf("FAILED: %d check(s)\n", failures);
        return 1;
    }
    puts("PASS: the ATE software's health monitor is driven the way it expects");
    return 0;
}

/*
 * The PBIT request frame.
 *
 * This is the only thing the VMC test transmits, and it is
 * Test_Starters/vmc/src/main.c's send_pbit_request. What matters is that the
 * bytes on the wire are that frame's: the VL id in the destination MAC and
 * again in the destination IP, the 11-byte cmsw header with its identifier,
 * length and sequence byte, and the padding to the Ethernet minimum.
 *
 * The frame is checked field by field rather than against a captured blob,
 * because there is no capture - the starter's path is exercised on the main rig
 * and not here.
 *
 * It carries no VLAN tag. The starter tags its requests because it reaches the
 * VMC through the Mellanox switch and the tag is what steers them there; this
 * test is cabled straight to the VMC, so there is nothing to steer.
 */

#include "VmcPbitRequest.h"

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

static uint8_t g_frame[128];
static const uint8_t SRC_MAC[6] = {0xc4, 0x70, 0xbd, 0x92, 0xe5, 0x1e};

static uint16_t be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }

static uint16_t ip_checksum(const uint8_t *ip)
{
    uint32_t sum = 0;

    for (int i = 0; i < 20; i += 2)
        sum += ((uint32_t)ip[i] << 8) | ip[i + 1];
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

static void test_sequence(void)
{
    /* 0 once, then 1..255 cycling - never 0 again. */
    check(vmc_pbit_request_seq(0) == 0, "the first request carries 0");
    check(vmc_pbit_request_seq(1) == 1 && vmc_pbit_request_seq(255) == 255,
          "then 1 through 255");
    check(vmc_pbit_request_seq(256) == 1, "and wraps to 1, not to 0");
    for (uint64_t i = 1; i < 2000; i++)
        if (vmc_pbit_request_seq(i) == 0) {
            printf("[FAIL] request %llu carried 0\n", (unsigned long long)i);
            failures++;
            break;
        }
    printf("[ OK ] the sequence byte\n");
}

static void test_frame(const vmc_config_t *c)
{
    size_t len = vmc_pbit_request_build(g_frame, SRC_MAC,
                                        c->flcs_pbit_request, c->msg_pbit_request, 7);

    check(len == VMC_PBIT_REQ_FRAME_LEN, "padded to the Ethernet minimum");
    check(g_frame[0] == 0x03 && g_frame[1] == 0 && g_frame[2] == 0 && g_frame[3] == 0,
          "the destination MAC prefix");
    check(be16(g_frame + 4) == c->flcs_pbit_request, "the VL id in the MAC");
    check(memcmp(g_frame + 6, SRC_MAC, 6) == 0, "our own address as the sender");
    check(g_frame[12] == 0x08 && g_frame[13] == 0x00,
          "IPv4 straight after the addresses - no VLAN tag");

    const uint8_t *ip = g_frame + 14;
    check(ip[0] == 0x45 && ip[8] == 1 && ip[9] == 17, "IPv4, TTL 1, UDP");
    check(be16(ip + 2) == 20 + 8 + VMC_PBIT_REQ_PAYLOAD_LEN,
          "the IP length covers the payload and no padding");
    check(ip[12] == 10 && ip[13] == 0 && ip[14] == 0 && ip[15] == 0, "source 10.0.0.0");
    check(ip[16] == 224 && ip[17] == 224 && be16(ip + 18) == c->flcs_pbit_request,
          "the VL id again in the destination address");
    check(ip_checksum(ip) == 0, "the header checksum is right");

    const uint8_t *udp = ip + 20;
    check(be16(udp) == 100 && be16(udp + 2) == 100, "UDP 100 to 100");
    check(be16(udp + 4) == 8 + VMC_PBIT_REQ_PAYLOAD_LEN, "the UDP length");

    const uint8_t *payload = udp + 8;
    check(payload[0] == c->msg_pbit_request, "the message identifier");
    check(be16(payload + 1) == VMC_PBIT_REQ_PAYLOAD_LEN,
          "a message length of 11, big-endian");
    check(payload[VMC_PBIT_REQ_PAYLOAD_LEN - 1] == vmc_pbit_request_seq(7),
          "the sequence byte last, over the timestamp's last byte");
    for (int i = 3; i < VMC_PBIT_REQ_PAYLOAD_LEN - 1; i++)
        check(payload[i] == 0, "the timestamp is left zero");
    for (size_t i = 14 + 20 + 8 + VMC_PBIT_REQ_PAYLOAD_LEN; i < len; i++)
        check(g_frame[i] == 0, "the padding is zero");
    printf("[ OK ] the request frame\n");
}

/* The sequence byte is the only thing that changes between requests, so it is
 * worth seeing that it does, and that nothing else moves with it. */
static void test_sequence_on_the_wire(const vmc_config_t *c)
{
    uint8_t first[VMC_PBIT_REQ_FRAME_LEN];
    const size_t seq_offset = 14 + 20 + 8 + VMC_PBIT_REQ_PAYLOAD_LEN - 1;

    vmc_pbit_request_build(first, SRC_MAC, c->vs_pbit_request, c->msg_pbit_request, 0);
    check(first[seq_offset] == 0, "the first request carries 0 on the wire");

    for (uint64_t n = 1; n < 5; n++) {
        vmc_pbit_request_build(g_frame, SRC_MAC, c->vs_pbit_request,
                               c->msg_pbit_request, n);
        check(g_frame[seq_offset] == vmc_pbit_request_seq(n),
              "each request carries its own sequence byte");
        g_frame[seq_offset] = first[seq_offset];
        check(memcmp(g_frame, first, VMC_PBIT_REQ_FRAME_LEN) == 0,
              "and nothing else in the frame changes with it");
    }
    printf("[ OK ] the sequence byte is the only thing that moves\n");
}

static void test_config(const vmc_config_t *c)
{
    /* The request VLs and the response VLs are different, and both are in the
     * configuration - a rig that renumbers them is one edit. */
    check(c->flcs_pbit_request != c->flcs_pbit_response &&
          c->vs_pbit_request != c->vs_pbit_response,
          "a side asks on one VL and is answered on another");
    check(c->msg_pbit_request != c->msg_pbit_response,
          "the request and the answer carry different message identifiers");
    check(c->pbit_resend_interval_s > 0, "the request is repeated");
    check(c->net_type_es != c->net_type_sw_es,
          "the two end-system network types are distinguishable");
    printf("[ OK ] the request settings come from AppConfig\n");
}

int main(void)
{
    const vmc_config_t *c = app_config_vmc();

    test_sequence();
    test_config(c);
    test_frame(c);
    test_sequence_on_the_wire(c);

    if (failures) {
        printf("FAILED: %d check(s)\n", failures);
        return 1;
    }
    puts("PASS: the PBIT request frame is the starter's, field for field");
    return 0;
}

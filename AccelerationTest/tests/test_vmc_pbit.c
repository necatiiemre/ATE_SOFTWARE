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

static void test_untagged(const vmc_config_t *c)
{
    size_t len = vmc_pbit_request_build(g_frame, SRC_MAC, -1,
                                        c->flcs_pbit_request, c->msg_pbit_request, 7);

    check(len == VMC_PBIT_REQ_FRAME_LEN, "padded to the Ethernet minimum");
    check(g_frame[0] == 0x03 && g_frame[1] == 0 && g_frame[2] == 0 && g_frame[3] == 0,
          "the destination MAC prefix");
    check(be16(g_frame + 4) == c->flcs_pbit_request, "the VL id in the MAC");
    check(memcmp(g_frame + 6, SRC_MAC, 6) == 0, "our own address as the sender");
    check(g_frame[12] == 0x08 && g_frame[13] == 0x00, "IPv4 straight after, untagged");

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
    printf("[ OK ] the untagged request frame\n");
}

/* The rig is cabled straight to the VMC and sends untagged, but the tagged form
 * is still built - the starter on the main rig needs it, and a move back to a
 * switched path is one edit. So it is tested with an explicit VLAN rather than
 * with whatever the configuration currently holds. */
#define A_VLAN 97

static void test_tagged(const vmc_config_t *c)
{
    size_t len = vmc_pbit_request_build(g_frame, SRC_MAC, A_VLAN,
                                        c->vs_pbit_request, c->msg_pbit_request, 0);

    check(len == VMC_PBIT_REQ_FRAME_LEN, "the tag does not change the frame length");
    check(g_frame[12] == 0x81 && g_frame[13] == 0x00, "an 802.1Q tag");
    check((be16(g_frame + 14) & 0x0FFF) == A_VLAN, "carrying the VLAN id");
    check(g_frame[16] == 0x08 && g_frame[17] == 0x00, "then IPv4");

    /* Everything after the tag shifts by four, and has to still be right. */
    const uint8_t *ip = g_frame + 18;
    check(ip[0] == 0x45 && ip_checksum(ip) == 0, "the IP header moved intact");
    check(be16(ip + 18) == c->vs_pbit_request, "the VS request VL");
    check(ip[16] == 224 && ip[17] == 224, "still the same destination prefix");

    const uint8_t *payload = ip + 20 + 8;
    check(payload[0] == c->msg_pbit_request &&
          payload[VMC_PBIT_REQ_PAYLOAD_LEN - 1] == 0,
          "the payload, with the first request's sequence byte of 0");
    printf("[ OK ] the tagged request frame\n");
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
    /* Straight cable to the VMC: nothing steers the frame, so nothing tags it. */
    check(c->request_vlan_flcs < 0 && c->request_vlan_vs < 0,
          "the requests go out untagged");
    printf("[ OK ] the request settings come from AppConfig\n");
}

int main(void)
{
    const vmc_config_t *c = app_config_vmc();

    test_sequence();
    test_config(c);
    test_untagged(c);
    test_tagged(c);

    if (failures) {
        printf("FAILED: %d check(s)\n", failures);
        return 1;
    }
    puts("PASS: the PBIT request frame is the starter's, field for field");
    return 0;
}

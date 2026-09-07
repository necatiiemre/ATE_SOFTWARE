/*
 * Health-monitor decode test.
 *
 * The offsets come from dpdk/include/HealthTypes.h and there is no capture to
 * check them against, so what this pins is that the decoder reads the fields it
 * claims to: build a packet of each of the three shapes with a distinct value
 * at every documented offset, decode it, and require the values back.
 *
 * It also holds the sizes, which is the part that was got wrong once already.
 * Every packet carries a byte or two past its last port block, and how many
 * depends on whether the AFDX sequence byte falls inside the UDP length. Both
 * answers are fed in here, because the decoder must not depend on which is
 * right - and a decoder that parses "as many blocks as fit" reads the spare
 * bytes as a port.
 */

#include "HealthDecode.h"

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

static void put_be48(uint8_t *p, uint64_t v)
{
    for (int i = 0; i < 6; i++)
        p[i] = (uint8_t)(v >> (8 * (5 - i)));
}

/* One port block, every counter set to a value derived from the port number so
 * a field read from the wrong offset shows up as the wrong number. */
static void fill_port(uint8_t *b, uint16_t port)
{
    memset(b, 0, HD_PORT_BLOCK_LEN);
    put_be16(b + 0, port);
    b[2] = 0x11;
    put_be48(b +   3, 1000u + port);   /* CRC errors */
    put_be48(b +  27, 2000u + port);   /* Lmin */
    put_be48(b +  33, 3000u + port);   /* Lmax */
    put_be48(b +  45, 4000u + port);   /* policy drop */
    put_be48(b +  57, 5000u + port);   /* TX */
    put_be48(b +  63, 6000u + port);   /* RX */
    put_be48(b +  69, 7000u + port);   /* wrong source */
    put_be48(b +  81, 8000u + port);   /* queue overflow */
    put_be48(b +  87, 9000u + port);   /* undefined VL */
    put_be48(b +  93, 10000u + port);  /* undefined MAC */
    put_be48(b + 123, 2);              /* 100M */
}

static void fill_device(uint8_t *p, uint8_t status_enable, uint16_t config_id)
{
    memset(p, 0, HD_DEVICE_HEADER_LEN);
    p[6]  = status_enable;
    put_be48(p +  9, 111111);      /* TX total */
    put_be48(p + 15, 222222);      /* RX total */
    put_be48(p + 21, 333);         /* TX errors */
    put_be48(p + 27, 444);         /* RX errors */
    p[33] = 0x5a;                  /* heartbeat */
    p[36] = 35;                    /* port count */
    p[38] = 1;                     /* switch mode */
    put_be48(p + 81, 7);           /* wrong device id */
    put_be48(p + 87, 8);           /* wrong operation */
    put_be48(p + 93, 9);           /* wrong type */
    put_be16(p + 105, config_id);
}

static uint8_t g_packet[2048];

static void test_with_header(hd_state_t *state)
{
    memset(g_packet, 0xEE, sizeof g_packet);   /* the two spare bytes are noise */
    fill_device(g_packet, 0x03, 0xBEEF);       /* assistant */
    for (uint16_t i = 0; i < 8; i++)
        fill_port(g_packet + HD_DEVICE_HEADER_LEN + i * HD_PORT_BLOCK_LEN, i);

    check(hd_ingest(state, g_packet, HD_BODY_WITH_HEADER + 1), "1187-byte packet decodes");
    check(state->assistant.valid && !state->manager.valid,
          "status_enable 0x03 lands on the assistant");
    check(state->assistant.config_id == 0xBEEF, "config id");
    check(state->assistant.heartbeat == 0x5a, "heartbeat");
    check(state->assistant.port_count == 35, "port count");
    check(state->assistant.sw_mode == 1, "switch mode");
    check(state->assistant.tx_total == 111111 && state->assistant.rx_total == 222222,
          "TX/RX totals");
    check(state->assistant.tx_err_total == 333 && state->assistant.rx_err_total == 444,
          "TX/RX error totals");
    check(state->assistant.eth_wrong_dev_cnt == 7 &&
          state->assistant.eth_wrong_op_cnt == 8 &&
          state->assistant.eth_wrong_type_cnt == 9,
          "the three configuration-rejection counters");

    for (uint16_t i = 0; i < 8; i++) {
        const hd_port_t *p = &state->ports[i];
        check(p->valid && p->rx_count == 6000u + i && p->tx_count == 5000u + i &&
              p->vlid_drop == 9000u + i && p->vl_source_err == 7000u + i &&
              p->vl_min_err == 2000u + i && p->vl_max_err == 3000u + i &&
              p->crc_err == 1000u + i && p->undef_mac == 10000u + i &&
              p->queue_overflow == 8000u + i && p->policy_drop == 4000u + i &&
              p->speed == 2 && p->bit_status == 0x11,
              "port block fields");
    }
    printf("[ OK ] 1187-byte packet: device header and 8 port blocks\n");
}

/* Both readings of where the AFDX byte falls, so the decoder cannot depend on
 * one of them being right. */
static void test_both_trailer_conventions(hd_state_t *state)
{
    for (int spare = 1; spare <= 2; spare++) {
        memset(g_packet, 0xEE, sizeof g_packet);
        fill_device(g_packet, 0x01, (uint16_t)(0x1000 + spare));   /* manager */
        for (uint16_t i = 0; i < 8; i++)
            fill_port(g_packet + HD_DEVICE_HEADER_LEN + i * HD_PORT_BLOCK_LEN,
                      (uint16_t)(16 + i));
        check(hd_ingest(state, g_packet, (size_t)(HD_BODY_WITH_HEADER + spare)),
              "the device packet decodes whether 1 or 2 bytes are spare");
        check(state->manager.config_id == 0x1000 + spare, "and reads the same fields");
    }
    printf("[ OK ] both trailer conventions decode identically\n");
}

static void test_mini_header(hd_state_t *state)
{
    /* Ports place themselves by the number inside the block, so a mini-header
     * packet needs no memory of which FPGA sent the one before it. */
    memset(g_packet, 0xEE, sizeof g_packet);
    for (uint16_t i = 0; i < 8; i++)
        fill_port(g_packet + HD_MINI_HEADER_LEN + i * HD_PORT_BLOCK_LEN, (uint16_t)(8 + i));
    check(hd_ingest(state, g_packet, HD_BODY_8_PORTS + 1), "1083-byte packet decodes");
    for (uint16_t i = 8; i < 16; i++)
        check(state->ports[i].valid && state->ports[i].rx_count == 6000u + i,
              "ports 8-15 from the mini-header packet");

    memset(g_packet, 0xEE, sizeof g_packet);
    for (uint16_t i = 0; i < 3; i++)
        fill_port(g_packet + HD_MINI_HEADER_LEN + i * HD_PORT_BLOCK_LEN, (uint16_t)(32 + i));
    check(hd_ingest(state, g_packet, HD_BODY_3_PORTS + 2), "438-byte packet decodes");
    for (uint16_t i = 32; i < 35; i++)
        check(state->ports[i].valid && state->ports[i].rx_count == 6000u + i,
              "ports 32-34 from the 3-port packet");
    printf("[ OK ] 1083- and 438-byte packets: port blocks by their own number\n");
}

static void test_rejects(hd_state_t *state)
{
    uint64_t before = state->undecoded;

    memset(g_packet, 0x00, sizeof g_packet);
    check(!hd_ingest(state, g_packet, 94 - 43), "the MCU packet carries no ports");
    check(!hd_ingest(state, g_packet, HD_BODY_WITH_HEADER - 1), "one byte short is refused");
    check(!hd_ingest(state, g_packet, HD_BODY_8_PORTS + HD_SIZE_ALLOWANCE),
          "past the allowance is refused");
    fill_device(g_packet, 0x05, 1);
    check(!hd_ingest(state, g_packet, HD_BODY_WITH_HEADER + 1),
          "a header that is neither FPGA is refused");
    check(state->undecoded == before + 4, "each refusal is counted");
    printf("[ OK ] anything that is not one of the three shapes is refused\n");
}

int main(void)
{
    hd_state_t state;

    hd_init(&state);
    test_with_header(&state);
    test_both_trailer_conventions(&state);
    test_mini_header(&state);
    test_rejects(&state);

    /* Ports nothing reported must stay unreported rather than read as zeros. */
    check(!state.ports[28].valid, "a port no packet described is not valid");
    check(hd_latest_device(&state) == &state.manager, "the latest device header");

    if (failures) {
        printf("FAILED: %d check(s)\n", failures);
        return 1;
    }
    puts("PASS: the health decoder reads the fields it documents");
    return 0;
}

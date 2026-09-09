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

/* Time of day is a 5-byte field, not 6 - writing it as 6 walks into the next
 * one and reads back shifted. */
static void put_be40(uint8_t *p, uint64_t v)
{
    for (int i = 0; i < 5; i++)
        p[i] = (uint8_t)(v >> (8 * (4 - i)));
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
    put_be16(p + 0, 0x2600);
    p[2]  = 0x53;                  /* operation */
    p[3]  = 0x44;                  /* config type */
    put_be16(p + 4, 1187);
    p[6]  = status_enable;
    put_be16(p + 7, 0x0044);       /* status address */
    put_be16(p + 34, 0x2600);      /* device id, again */
    p[37] = 2;                     /* token bucket */
    p[42] = 9;                     /* vendor */
    p[43] = 1;                     /* auto MAC update */
    p[44] = 3;                     /* upstream mode */
    put_be16(p + 45, 2); put_be16(p + 47, 1); put_be16(p + 49, 7);  /* switch core */
    p[51 + 3] = 1; p[51 + 4] = 0; p[51 + 5] = 4;                    /* end system core */
    put_be16(p + 57, 100); put_be16(p + 59, 200); put_be16(p + 61, 300);
    put_be16(p + 63, 400); put_be16(p + 65, 500); put_be16(p + 67, 600);
    put_be40(p + 70, 123456789);   /* time of day, ns - a 5-byte field */
    put_be40(p + 76, 1700000000);  /* time of day, s  - likewise */
    put_be16(p + 101, (3300u << 3) | 2u);            /* 3.3002 V */
    put_be16(p + 103, (uint16_t)((320u << 4) | 5u)); /* 320.5 K */
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
    check(state->assistant.device_id == 0x2600 && state->assistant.device_id2 == 0x2600 &&
          state->assistant.operation_type == 0x53 && state->assistant.config_type == 0x44 &&
          state->assistant.frame_length == 1187 && state->assistant.status_addr == 0x0044,
          "the header fields");
    check(state->assistant.token_bucket == 2 && state->assistant.vendor_id == 9 &&
          state->assistant.auto_mac_update == 1 && state->assistant.upstream_mode == 3,
          "the mode bytes");
    check(state->assistant.sw_ip[0] == 2 && state->assistant.sw_ip[1] == 1 &&
          state->assistant.sw_ip[2] == 7 && state->assistant.es_ip[0] == 1 &&
          state->assistant.es_ip[1] == 0 && state->assistant.es_ip[2] == 4,
          "the two core versions, which are not encoded the same way");
    check(state->assistant.sw_input_fifo == 100 && state->assistant.pkt_pro_fifo == 200 &&
          state->assistant.sw_output_fifo == 300 && state->assistant.hp_fifo == 400 &&
          state->assistant.lp_fifo == 500 && state->assistant.be_fifo == 600,
          "the FIFO sizes");
    check(state->assistant.tod_ns == 123456789 && state->assistant.tod_sec == 1700000000,
          "time of day, both 5-byte fields");

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

/* The two FPGA readings are bit-packed rather than plain numbers, and the
 * temperature is in Kelvin with a divisor that depends on the fraction. */
static void test_conversions(hd_state_t *state)
{
    check(state->assistant.fpga_voltage_raw == ((3300u << 3) | 2u), "the raw voltage word");
    double volts = hd_fpga_volts(state->assistant.fpga_voltage_raw);
    check(volts > 3.3001 && volts < 3.3003, "3300.2 mV reads as 3.3002 V");

    double celsius = hd_fpga_celsius(state->assistant.fpga_temp_raw);
    check(celsius > 47.34 && celsius < 47.36, "320.5 K reads as 47.35 C");

    /* A fraction of 10 or more is hundredths, not tenths. */
    double hundredths = hd_fpga_celsius((int16_t)((320u << 4) | 12u));
    check(hundredths > 46.97 && hundredths < 46.99, "a fraction past 9 is hundredths");

    check(strcmp(hd_speed_name(0), "1G") == 0 && strcmp(hd_speed_name(2), "100M") == 0 &&
          strcmp(hd_speed_name(9), "?") == 0, "the speed names");
    printf("[ OK ] the bit-packed voltage and temperature readings\n");
}

static void test_mcu(hd_state_t *state)
{
    uint8_t *p = g_packet;

    memset(p, 0xEE, sizeof g_packet);
    memset(p, 0, HD_BODY_MCU + 8);
    put_be16(p + 0, 0x2600);
    p[6] = 0x05;                   /* MCU */
    p[7] = 0x21; p[8] = 7;         /* firmware 2.1.7 */
    p[9] = 0x02;                   /* 28V secondary failed */
    p[10] = 0; p[11] = 1;          /* PBIT ok, CBIT failed */
    put_be16(p + 12, 412);  put_be16(p + 14, 1203); put_be16(p + 16, 502);
    put_be16(p + 18, 150);  put_be16(p + 20, 980);  put_be16(p + 22, 2310);
    put_be16(p + 24, 1870);
    put_be16(p + 26, 3310); put_be16(p + 28, 3300); put_be16(p + 30, 12010);
    put_be16(p + 32, 1800); put_be16(p + 34, 1300); put_be16(p + 36, 1000);
    put_be16(p + 38, 1001);
    put_be16(p + 40, 4215); put_be16(p + 42, 3980);
    p[48] = (uint8_t)45; p[49] = (uint8_t)(-7);

    check(hd_ingest(state, p, HD_BODY_MCU + 1), "the MCU packet decodes");
    const hd_mcu_t *m = &state->mcu;
    check(m->valid && m->device_id == 0x2600, "MCU header");
    check(m->fw_major == 2 && m->fw_minor == 1 && m->fw_patch == 7,
          "the firmware version is a packed nibble pair plus a byte");
    check((m->input_power & 0x01) == 0 && (m->input_power & 0x02) != 0,
          "28V primary passed and secondary failed");
    check(m->pbit == 0 && m->cbit == 1, "PBIT and CBIT");
    check(m->curr_12v == 412 && m->volt_12v == 12010, "the 12V rail");
    check(m->curr_1v0_mgr == 2310 && m->volt_1v0_ast == 1001, "the FPGA rails");
    check(m->board_temp == 4215 && m->fo_temp == 3980, "the hundredths temperatures");
    check(m->phy_1g_temp == 45 && m->phy_100m_temp == -7,
          "the PHY temperatures are signed whole degrees");
    printf("[ OK ] the MCU packet: rails, temperatures and the 28V input\n");
}

static void test_port_errors(void)
{
    hd_port_t clean = {0};
    char text[192];

    hd_port_errors(&clean, text, sizeof text);
    check(text[0] == '\0', "a clean port has nothing to say");

    clean.vlid_drop = 4471;
    clean.crc_err = 3;
    hd_port_errors(&clean, text, sizeof text);
    check(strcmp(text, "undef-VL 4471, CRC 3") == 0, "each non-zero counter is named");
    printf("[ OK ] port errors are named rather than counted into one number\n");
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
    check(!hd_ingest(state, g_packet, HD_BODY_MCU + 1),
          "an MCU-sized packet that does not say MCU is refused");
    check(!hd_ingest(state, g_packet, HD_BODY_WITH_HEADER - 1), "one byte short is refused");
    check(!hd_ingest(state, g_packet, HD_BODY_8_PORTS + HD_SIZE_ALLOWANCE),
          "past the allowance is refused");
    check(!hd_ingest(state, g_packet, 20), "something far too small is refused");
    fill_device(g_packet, 0x05, 1);
    check(!hd_ingest(state, g_packet, HD_BODY_WITH_HEADER + 1),
          "a header that is neither FPGA is refused");
    check(state->undecoded == before + 5, "each refusal is counted");
    printf("[ OK ] anything that is not one of the three shapes is refused\n");
}

int main(void)
{
    hd_state_t state;

    hd_init(&state);
    test_with_header(&state);
    test_conversions(&state);
    test_both_trailer_conventions(&state);
    test_mini_header(&state);
    test_mcu(&state);
    test_port_errors();
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

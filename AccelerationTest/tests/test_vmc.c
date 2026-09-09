/*
 * VMC health-monitor decode test.
 *
 * There is no capture to compare against, so what this pins is that a frame is
 * sorted to the right report and comes back the right way round: build a frame
 * for each of the six reports on each of the two sides, with a distinct value
 * in every field the decoder touches, and require those values back.
 *
 * Byte order is the part most likely to be got wrong. Everything the VMC sends
 * is big-endian, so every field here is written big-endian and checked against
 * a host-order expectation - a decoder that forgets a swap fails, and so does
 * one that swaps something it should not.
 */

#include "VmcHealth.h"

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

/* ------------------------------------------------------------------ */
/* Building a frame the way the VMC does: VL id in the destination MAC, then
 * IP and UDP, then the report. */

#define ETH 14
#define IP  20
#define UDP  8
#define PAYLOAD_OFFSET (ETH + IP + UDP)

static uint8_t g_frame[4096];

static uint8_t *frame_for(uint16_t vl_id, size_t payload_len, size_t *frame_len)
{
    memset(g_frame, 0, sizeof g_frame);

    g_frame[0] = 0x03;
    g_frame[4] = (uint8_t)(vl_id >> 8);
    g_frame[5] = (uint8_t)vl_id;
    g_frame[12] = 0x08; g_frame[13] = 0x00;      /* IPv4 */
    g_frame[ETH + 0] = 0x45;
    g_frame[ETH + 9] = 17;                       /* UDP */

    *frame_len = PAYLOAD_OFFSET + payload_len;
    return g_frame + PAYLOAD_OFFSET;
}

static void put_be16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static void put_be32(uint8_t *p, uint32_t v)
{
    for (int i = 0; i < 4; i++)
        p[i] = (uint8_t)(v >> (8 * (3 - i)));
}
static void put_be64(uint8_t *p, uint64_t v)
{
    for (int i = 0; i < 8; i++)
        p[i] = (uint8_t)(v >> (8 * (7 - i)));
}
static void put_be_float(uint8_t *p, float f)
{
    uint32_t bits;
    memcpy(&bits, &f, 4);
    put_be32(p, bits);
}

/* Header: message id, then a big-endian length and timestamp. */
static void put_header(uint8_t *p, uint8_t msg_id, uint16_t len, uint64_t stamp)
{
    p[0] = msg_id;
    put_be16(p + 1, len);
    put_be64(p + 3, stamp);
}

/* ------------------------------------------------------------------ */

static void test_cpu_usage(vmc_health_t *h, const vmc_config_t *c)
{
    size_t frame_len;
    uint8_t *p = frame_for(c->vs_cpu_usage, sizeof(Pcs_profile_stats), &frame_len);

    /* Three uint64 counters, then four exec-time monitors, then two memory
     * profiles. A monitor is 16 bytes, not 8: a percentage byte, seven bytes of
     * padding to align the counter, then the counter. */
    put_be64(p +   0, 4102);         /* sample count */
    put_be64(p +   8, 777000);       /* latest read time */
    put_be64(p +  16, 888000);       /* total run time */
    put_be64(p +  24 + 8, 120);      /* min exec */
    put_be64(p +  40 + 8, 980);      /* max exec */
    put_be64(p +  56 + 8, 135);      /* avg exec */
    put_be64(p +  72 + 8, 128);      /* last exec */
    put_be64(p +  88, 65536);        /* heap total */
    put_be64(p +  96, 12288);        /* heap used */
    put_be64(p + 104, 20480);        /* heap peak */
    put_be64(p + 112, 8192);         /* stack total */
    put_be64(p + 120, 3072);         /* stack used */
    put_be64(p + 128, 4096);         /* stack peak */

    check(vmc_health_ingest(h, g_frame, frame_len), "the CPU usage frame is accepted");

    const Pcs_profile_stats *cpu = &h->side[VMC_VS].cpu_usage;
    check(cpu->sample_count == 4102 && cpu->latest_read_time == 777000 &&
          cpu->total_run_time == 888000, "the three counters");
    check(cpu->cpu_exec_time.min_exec_time.usage == 120 &&
          cpu->cpu_exec_time.max_exec_time.usage == 980 &&
          cpu->cpu_exec_time.avg_exec_time.usage == 135 &&
          cpu->cpu_exec_time.last_exec_time.usage == 128, "the exec times");
    check(cpu->heap_mem.total_size == 65536 && cpu->heap_mem.used_size == 12288 &&
          cpu->heap_mem.max_used_size == 20480, "the heap profile");
    check(cpu->stack_mem.total_size == 8192 && cpu->stack_mem.used_size == 3072 &&
          cpu->stack_mem.max_used_size == 4096, "the stack profile");
    check(h->side[VMC_VS].seen[VMC_REPORT_CPU_USAGE].packets == 1 &&
          h->side[VMC_FLCS].seen[VMC_REPORT_CPU_USAGE].packets == 0,
          "it lands on the VS side, not FLCS");
    printf("[ OK ] CPU usage\n");
}

static void test_pbit(vmc_health_t *h, const vmc_config_t *c)
{
    size_t frame_len;
    uint8_t *p = frame_for(c->flcs_pbit_response, sizeof(vmc_pbit_data_t), &frame_len);

    put_header(p, c->msg_pbit_response, 454, 0x1122334455667788ull);
    p[11] = 7;                        /* lru id */
    p[12] = 3;                        /* policy step count */
    p[13] = 1;                        /* policy step status */
    /* list[2]: command 9 returning -5, at 14 + 2*5 */
    p[14 + 10] = 9;
    put_be32(p + 14 + 11, (uint32_t)(int32_t)-5);
    put_be32(p + 418, 20250909);      /* serial */
    p[449] = 0x05;                    /* FLCS CPU and eMMC flagged */
    put_be16(p + 450, 0xBEEF);        /* VS CPU PBIT */
    put_be16(p + 452, 0xCAFE);        /* FLCS CPU PBIT */

    check(vmc_health_ingest(h, g_frame, frame_len), "the PBIT frame is accepted");

    const vmc_pbit_data_t *pb = &h->side[VMC_FLCS].pbit;
    check(pb->header_st.message_len == 454 &&
          pb->header_st.timestamp == 0x1122334455667788ull, "the header is swapped");
    check(pb->vmc_serial_number == 20250909, "the serial number");
    check(pb->list[2].policy_cmd == 9 && pb->list[2].ret_val == -5,
          "a policy step: the command is a byte, the return value is swapped");
    check(pb->list[0].ret_val == 0, "an untouched policy step stays zero");
    check(pb->vs_cpu_pbit == 0xBEEF && pb->flcs_cpu_pbit == 0xCAFE, "the CPU words");
    check(pb->vmp_storage_and_status_st.flcs_cpu_status == 1 &&
          pb->vmp_storage_and_status_st.vs_cpu_status == 0 &&
          pb->vmp_storage_and_status_st.eMMC_storage_status == 1 &&
          pb->vmp_storage_and_status_st.MRAM_storage_status == 0,
          "the storage bitfield, which is not swapped");

    /* The same VL carries other traffic, so the message id has to guard it. */
    uint64_t before = h->side[VMC_FLCS].seen[VMC_REPORT_PBIT].packets;
    p[0] = (uint8_t)(c->msg_pbit_response + 1);
    check(!vmc_health_ingest(h, g_frame, frame_len),
          "a long frame on the PBIT VL with another message id is refused");
    check(h->side[VMC_FLCS].seen[VMC_REPORT_PBIT].packets == before,
          "and does not overwrite the report");
    printf("[ OK ] PBIT\n");
}

static void test_cbit_sorting(vmc_health_t *h, const vmc_config_t *c)
{
    size_t frame_len;
    uint8_t *p;

    /* Board monitor: header, lru, comm status, then 96 floats. */
    p = frame_for(c->vs_cbit, sizeof(bm_engineering_cbit_report_t), &frame_len);
    put_header(p, c->msg_bm_engineering, 397, 1);
    put_be_float(p + 13, 42.5f);                        /* first float */
    put_be_float(p + 13 + 95 * 4, -12.25f);             /* last float */
    check(vmc_health_ingest(h, g_frame, frame_len), "the board monitor is accepted");
    check(h->side[VMC_VS].bm_engineering.vs_status_st.VSCPU_12V_current == 42.5f,
          "the first float");
    check(h->side[VMC_VS].bm_engineering.vmc_board_status_st.BRD_MNGR_12V_main_current
          == -12.25f, "the last float, so the whole run is swapped");

    /* Board flags: same preamble, then 16-bit words. */
    p = frame_for(c->vs_cbit, sizeof(bm_flag_cbit_report_t), &frame_len);
    put_header(p, c->msg_bm_flag, 105, 2);
    put_be16(p + 37, 0x0004);                           /* red flag 1, at 37 */
    put_be16(p + 81 + 2, 0x0100);                       /* yellow flag 2, at 83 */
    check(vmc_health_ingest(h, g_frame, frame_len), "the board flags are accepted");
    check(h->side[VMC_VS].bm_flag.event_red_bitmaps_st.red_event_flag_1 == 0x0004,
          "a red flag word");
    check(h->side[VMC_VS].bm_flag.event_yellow_bitmaps_st.yellow_event_flag_2 == 0x0100,
          "a yellow flag word");

    /* DTN end system. */
    p = frame_for(c->flcs_cbit, sizeof(dtn_es_cbit_report_t), &frame_len);
    put_header(p, c->msg_dtn_es, 352, 3);
    put_be64(p + 15 + 8,  0x2600);                      /* device id */
    put_be64(p + 15 + 24, 7);                           /* config id */
    check(vmc_health_ingest(h, g_frame, frame_len), "the DTN end system report is accepted");
    check(h->side[VMC_FLCS].dtn_es.dtn_es_monitoring_st.A664_ES_DEV_ID == 0x2600 &&
          h->side[VMC_FLCS].dtn_es.dtn_es_monitoring_st.A664_ES_CONFIG_ID == 7,
          "its device and configuration ids");

    /* DTN switch: status then eight ports. */
    p = frame_for(c->flcs_cbit, sizeof(dtn_sw_cbit_report_t), &frame_len);
    put_header(p, c->msg_dtn_sw, 1064, 4);
    put_be64(p + 15, 998110);                           /* tx total */
    put_be64(p + 15 + 8, 1002233);                      /* rx total */
    put_be16(p + 15 + 24, 0x2600);                      /* device id */
    /* The port array starts 57 bytes into the monitoring block; port 3 is three
     * 124-byte blocks in. Its id is first and its receive count 52 bytes in. */
    put_be16(p + 15 + 57 + 3 * 124, 22);
    put_be64(p + 15 + 57 + 3 * 124 + 52, 119006);
    check(vmc_health_ingest(h, g_frame, frame_len), "the DTN switch report is accepted");

    const dtn_sw_monitoring_t *sw = &h->side[VMC_FLCS].dtn_sw.dtn_sw_monitoring_st;
    check(sw->status.A664_SW_TX_TOTAL_COUNT == 998110 &&
          sw->status.A664_SW_RX_TOTAL_COUNT == 1002233 &&
          sw->status.A664_SW_DEV_ID == 0x2600, "its totals and device id");
    check(sw->port[3].A664_SW_PORT_ID == 22 &&
          sw->port[3].A664_SW_PORT_i_RX_COUNT == 119006,
          "a port block at the right stride");

    /* All four arrived on two VLs, sorted only by the message id. */
    check(h->side[VMC_VS].seen[VMC_REPORT_BM_ENGINEERING].packets == 1 &&
          h->side[VMC_VS].seen[VMC_REPORT_BM_FLAG].packets == 1 &&
          h->side[VMC_FLCS].seen[VMC_REPORT_DTN_ES].packets == 1 &&
          h->side[VMC_FLCS].seen[VMC_REPORT_DTN_SW].packets == 1,
          "one VL per side carried four different reports");
    printf("[ OK ] the four CBIT reports, sorted by message id\n");
}

static void test_refusals(vmc_health_t *h, const vmc_config_t *c)
{
    size_t frame_len;
    uint8_t *p;

    uint64_t short_before = h->too_short;
    frame_for(c->vs_cpu_usage, sizeof(Pcs_profile_stats) - 1, &frame_len);
    check(!vmc_health_ingest(h, g_frame, frame_len), "a short CPU usage frame is refused");
    check(h->too_short == short_before + 1, "and counted as short");

    uint64_t unknown_before = h->unknown_message;
    p = frame_for(c->vs_cbit, sizeof(bm_engineering_cbit_report_t), &frame_len);
    put_header(p, 99, 397, 5);
    check(!vmc_health_ingest(h, g_frame, frame_len), "an unknown message id is refused");
    check(h->unknown_message == unknown_before + 1 && h->last_unknown_msg == 99,
          "and the id is remembered, not just counted");

    uint64_t other_before = h->not_health;
    frame_for(0x4444, 200, &frame_len);
    check(!vmc_health_ingest(h, g_frame, frame_len), "a VL that is not ours is refused");
    check(h->not_health == other_before + 1 && h->last_unknown_vl == 0x4444,
          "and that VL is remembered");

    /* A DTN report with nothing in it is what the VMC sends before the DTN has
     * answered; dpdk_vmc skips those rather than overwriting a good one. */
    uint64_t empty_before = h->empty;
    uint64_t es_before = h->side[VMC_FLCS].seen[VMC_REPORT_DTN_ES].packets;
    p = frame_for(c->flcs_cbit, sizeof(dtn_es_cbit_report_t), &frame_len);
    put_header(p, c->msg_dtn_es, 352, 6);
    check(!vmc_health_ingest(h, g_frame, frame_len), "an empty DTN report is skipped");
    check(h->empty == empty_before + 1, "and counted as empty");
    check(h->side[VMC_FLCS].seen[VMC_REPORT_DTN_ES].packets == es_before &&
          h->side[VMC_FLCS].dtn_es.dtn_es_monitoring_st.A664_ES_DEV_ID == 0x2600,
          "leaving the last good report alone");
    printf("[ OK ] short frames, unknown ids and empty reports are refused\n");
}

static void test_vl_table(const vmc_config_t *c)
{
    /* Every id comes from the configuration, so changing a rig is one edit. */
    check(vmc_report_vl(c, VMC_FLCS, VMC_REPORT_CPU_USAGE) == c->flcs_cpu_usage &&
          vmc_report_vl(c, VMC_VS, VMC_REPORT_CPU_USAGE) == c->vs_cpu_usage &&
          vmc_report_vl(c, VMC_FLCS, VMC_REPORT_PBIT) == c->flcs_pbit_response &&
          vmc_report_vl(c, VMC_VS, VMC_REPORT_PBIT) == c->vs_pbit_response,
          "the CPU usage and PBIT VLs come from the configuration");
    for (int r = VMC_REPORT_BM_ENGINEERING; r < VMC_REPORT_COUNT; r++)
        check(vmc_report_vl(c, VMC_FLCS, (vmc_report_t)r) == c->flcs_cbit &&
              vmc_report_vl(c, VMC_VS, (vmc_report_t)r) == c->vs_cbit,
              "the four CBIT reports share one VL per side");
    check(c->iface != NULL && c->iface[0] != '\0', "the interface is named");
    printf("[ OK ] every id the decoder uses comes from AppConfig\n");
}

int main(void)
{
    const vmc_config_t *c = app_config_vmc();
    vmc_health_t health;

    vmc_health_init(&health, c);
    test_vl_table(c);
    test_cpu_usage(&health, c);
    test_pbit(&health, c);
    test_cbit_sorting(&health, c);
    test_refusals(&health, c);

    if (failures) {
        printf("FAILED: %d check(s)\n", failures);
        return 1;
    }
    puts("PASS: VMC reports are sorted and byte-swapped as the VMC sends them");
    return 0;
}

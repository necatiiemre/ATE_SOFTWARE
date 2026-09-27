/**
 * @file test_cmc_health.c
 * @brief The DSM health monitor: the copies, and that they decode and print.
 *
 * health_monitor.c, health_monitor_cmc.c and their nine headers are copied from
 * dpdk_cmc byte for byte - none of them touches DPDK, so there was nothing to
 * rewrite. What a copy needs testing for is therefore not its arithmetic but its
 * assumptions about this build: that the structs come out the sizes the wire
 * format says, since that is what the length-based dispatch keys on, and that
 * every printer runs to the end over a report that has something in it.
 *
 * A struct that came out the wrong size here would not fail loudly. It would
 * make every packet of that type an unknown length, and the dashboard would
 * quietly show one report fewer than the unit is sending.
 */

#include "health_monitor.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures;

static void check(bool ok, const char *what)
{
    if (!ok) {
        printf("[FAIL] %s\n", what);
        failures++;
    }
}

/* The lengths the dispatch keys on: the struct, plus the one trailing sequence
 * byte every health-monitor packet carries. Written out as numbers rather than
 * as sizeof, because the point is that sizeof agrees with the wire. */
static void test_sizes(void)
{
    /* The firmware is built for a 32-bit target, so the size_t fields of its
     * memory profile are 4 bytes there. Declaring them size_t here made them 8
     * and the struct 136, so the 113-byte packet the unit actually sends matched
     * nothing and was counted as an unknown length. */
    check(sizeof(Pcs_profile_stats) == 112, "Pcs_profile_stats is 112 bytes (packet 113)");
    check(sizeof(COUNTERS_DPM_52) == 416, "COUNTERS_DPM_52 is 52x2x4 = 416 (417)");
    check(sizeof(tA664ESMonitoring) == 352, "tA664ESMonitoring is 352 (353)");
    check(sizeof(COUNTERS_DPM) == 960, "COUNTERS_DPM is 20x6x8 = 960 (961)");
    check(sizeof(COUNTERS_INTER_DPM) == 720, "COUNTERS_INTER_DPM is 15x6x8 = 720 (721)");
    check(sizeof(COUNTERS_DSM) == 384, "COUNTERS_DSM is 8x6x8 = 384 (385)");
    check(sizeof(COUNTERS_DPM_VL) == 832, "COUNTERS_DPM_VL is 832 (833)");
    check(sizeof(Cl_cmsw_status_report_msg_type) == 662, "the CL CMSW report is 662 (663)");
    check(sizeof(Smmm_monitoring_data_t) == 130, "SMMM monitoring is 130 (131)");

    /* The switch report is too big for one frame and arrives in two, 1401 and
     * 945 bytes including each part's sequence byte. */
    check(sizeof(tA664SWMonitoring) == 2344, "the switch report is 2344, so it is split");
    check(1400 + 944 == 2344, "and the two parts add up to it");
    printf("[ OK ] every report is the size the wire format says\n");
}

/* One packet of a given total length on a given VL. The contents do not matter
 * for the dispatch - the length does - but they are not left zero, because a
 * printer that divides by a field should be run with that field set. */
static void feed(uint16_t vl_id, uint16_t total_len, uint8_t fill)
{
    static uint8_t buf[4096];

    memset(buf, fill, sizeof buf);
    buf[total_len - 1] = 0x01;          /* the trailing sequence byte */
    hm_handle_packet(vl_id, buf, total_len);
}

/* Each report type, at each DSM's VL, through the dispatch and out of the
 * dashboard. What is checked is that it all runs: a wrong format string or a
 * printer walking off the end of an array is what this catches. */
static void test_every_report(void)
{
    const uint16_t dsm[] = {HM_VLID_8009, HM_VLID_8109};

    for (size_t i = 0; i < sizeof dsm / sizeof dsm[0]; i++) {
        feed(dsm[i], 113, 0x11);        /* CPU usage and memory profile */
        feed(dsm[i], 353, 0x22);        /* end-system monitoring */
        feed(dsm[i], 961, 0x33);        /* inter-LRM counters, DPM */
        feed(dsm[i], 721, 0x44);        /* inter-DPM counters */
        feed(dsm[i], 385, 0x55);        /* inter-LRM counters, DSM */
        feed(dsm[i], 833, 0x66);        /* per-VL DPM counters */

        /* The switch report, in its two parts and in order. */
        feed(dsm[i], 1401, 0x77);
        feed(dsm[i], 945, 0x88);
    }

    /* The VLs that carry the other reports. */
    feed(HM_VLID_18, 663, 0x99);        /* CL CMSW status, DSM-A */
    feed(HM_VLID_19, 663, 0xAA);        /* CL CMSW status, DSM-B */
    feed(HM_VLID_2021, 131, 0xBB);      /* SMMM monitoring */
    feed(HM_VLID_2042, 113, 0xCC);
    feed(HM_VLID_2063, 385, 0xDD);
    feed(HM_VLID_2084, 961, 0xEE);
    feed(HM_VLID_2105, 721, 0x0F);

    /* The 52-pair DPM counters, one packet from each of the five DPMs. These
     * carry per-second deltas rather than totals, so the dashboard accumulates
     * them - see test_dpm52_accumulates. */
    feed(HM_VLID_2021, 417, 0x10);
    feed(HM_VLID_2042, 417, 0x11);
    feed(HM_VLID_2063, 417, 0x12);
    feed(HM_VLID_2084, 417, 0x13);
    feed(HM_VLID_2105, 417, 0x14);

    hm_print_dashboard();
    printf("[ OK ] every report type decodes and every printer runs\n");
}

/* The IPMC log, whose type is not told by the length but by a component id
 * inside it, and whose length requirement follows from that id. */
static void test_ipmc(void)
{
    static uint8_t buf[1024];
    /* Offset 2, big-endian: 57 is a DSM, which needs 846 bytes of board data. */
    const uint32_t comp_dsm = 57;

    memset(buf, 0x21, sizeof buf);
    buf[0] = 0x00; buf[1] = 0x07;                       /* device id */
    buf[2] = (uint8_t)(comp_dsm >> 24); buf[3] = (uint8_t)(comp_dsm >> 16);
    buf[4] = (uint8_t)(comp_dsm >> 8);   buf[5] = (uint8_t)comp_dsm;
    hm_handle_packet(HM_VLID_50, buf, 847);

    /* An unknown component id is not an IPMC packet and must not be stored as
     * one - the printer would read a layout that is not there. */
    buf[5] = 99;
    hm_handle_packet(HM_VLID_50, buf, 847);

    hm_print_dashboard();
    printf("[ OK ] the IPMC log, and an unknown board type refused\n");
}

/* A length nothing expects. The reference records the pair and the first 64
 * bytes of it, which is the only way an undocumented report gets identified, so
 * the path has to survive being handed one - including a truncated one. */
static void test_unknown_lengths(void)
{
    feed(HM_VLID_8009, 500, 0xEF);
    feed(HM_VLID_8009, 500, 0xEF);      /* the same pair again: counted, not re-dumped */
    feed(HM_VLID_8109, 3, 0x01);
    hm_handle_packet(HM_VLID_8009, NULL, 137);
    hm_handle_packet(HM_VLID_8009, (const uint8_t *)"", 0);

    /* A second part with no first part. The firmware sends the second twice on
     * a redundant pair, so this is normal rather than an error, and it must not
     * assemble half a report out of whatever was in the buffer. */
    feed(HM_VLID_8109, 945, 0x5A);

    hm_print_dashboard();
    printf("[ OK ] unknown lengths, a null payload and an orphaned fragment\n");
}

/* Which VL ids the data plane pulls out of the traffic and hands over. Get this
 * wrong and a health monitor either disappears into the PRBS verifier as a bad
 * packet, or a data-plane frame is handed to a decoder expecting a report. */
static void test_vl_set(void)
{
    check(hm_is_health_monitor_vl_id(HM_VLID_8009) &&
          hm_is_health_monitor_vl_id(HM_VLID_8109),
          "the two DSMs' VLs are health monitor VLs");
    check(hm_is_health_monitor_vl_id(18) && hm_is_health_monitor_vl_id(19) &&
          hm_is_health_monitor_vl_id(50) && hm_is_health_monitor_vl_id(2021) &&
          hm_is_health_monitor_vl_id(2042) && hm_is_health_monitor_vl_id(2063) &&
          hm_is_health_monitor_vl_id(2084) && hm_is_health_monitor_vl_id(2105),
          "so are the CL CMSW, IPMC, SMMM and counter VLs");
    check(!hm_is_health_monitor_vl_id(10001) && !hm_is_health_monitor_vl_id(10521) &&
          !hm_is_health_monitor_vl_id(10624),
          "and no data-plane VL is one");
    check(hm_vl_id_accepts_sw_mon(HM_VLID_8009) &&
          hm_vl_id_accepts_sw_mon(HM_VLID_8109) &&
          !hm_vl_id_accepts_sw_mon(2021),
          "the switch report comes only from the two DSMs");
    printf("[ OK ] the health-monitor VL set\n");
}

/*
 * The 52-pair DPM counters, and the one thing about them that is not like any
 * other report: the packet carries a *second's* counts, not a total, so the
 * dashboard adds each one to a running sum per DPM. A printer that showed the
 * packet's own numbers would look right and be wrong by however long the run had
 * been going.
 *
 * The sum only exists in the printed table, so the table is captured and read
 * back. Run before test_every_report, which feeds these packets too and would
 * otherwise have already added to the same accumulator.
 */
static void test_dpm52_accumulates(void)
{
    COUNTERS_DPM_52 d;

    for (int i = 0; i < DPM_COUNTERS52_COUNT; i++) {
        d.msg[i].rx_count = (uint32_t)(10 + i);
        d.msg[i].tx_count = (uint32_t)(100 + i);
    }

    /* Two seconds of the same deltas on DPM-1. */
    dpm52_accumulate(HM_VLID_2021, &d);
    dpm52_accumulate(HM_VLID_2021, &d);

    char path[] = "/tmp/cmc_hm_dpm52_XXXXXX";
    int tmp = mkstemp(path);
    int saved = dup(STDOUT_FILENO);

    if (tmp < 0 || saved < 0) {
        printf("[FAIL] could not capture the table\n");
        failures++;
        return;
    }
    fflush(stdout);
    dup2(tmp, STDOUT_FILENO);
    print_counters_dpm_52(&d, HM_VLID_2021, 2);
    fflush(stdout);
    dup2(saved, STDOUT_FILENO);
    close(saved);
    close(tmp);

    FILE *f = fopen(path, "r");
    char line[512];
    bool row_seen = false, packets_seen = false;

    while (f && fgets(line, sizeof line, f)) {
        unsigned idx;
        unsigned long long rx, tx;

        if (strstr(line, "2 paketin kumulatif toplami"))
            packets_seen = true;
        /* The left half of the first row: index 0, then its two totals. */
        if (sscanf(line, "║  %u %llu %llu", &idx, &rx, &tx) == 3 && idx == 0) {
            row_seen = true;
            check(rx == 20 && tx == 200,
                  "the totals are the sum of both packets, not the last one");
        }
    }
    if (f)
        fclose(f);
    remove(path);

    check(packets_seen, "the table says how many packets it is a total of");
    check(row_seen, "the first counter pair's row was printed");
    printf("[ OK ] the 52-pair DPM counters accumulate per second\n");
}

int main(void)
{
    test_sizes();
    test_vl_set();
    test_dpm52_accumulates();
    test_every_report();
    test_ipmc();
    test_unknown_lengths();

    if (failures) {
        printf("FAILED: %d check(s)\n", failures);
        return 1;
    }
    puts("PASS: the CMC health monitor decodes and prints every report");
    return 0;
}

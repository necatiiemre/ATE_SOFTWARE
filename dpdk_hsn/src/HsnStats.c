/**
 * @file HsnStats.c
 * @brief HSN per-device-port statistics table.
 */
#include <stdio.h>
#include <inttypes.h>

#include "HsnStats.h"
#include "HsnWorker.h"   /* per-VL counter externs */
#include "Config.h"      /* HSN_DEV_PORT_MAP_INIT, HSN_DEVICE_PORT_COUNT */

static const struct hsn_dev_port hsn_map[HSN_DEVICE_PORT_COUNT] = HSN_DEV_PORT_MAP_INIT;

/* Previous cumulative byte totals per device port, for per-second Gbps. */
static uint64_t prev_tx_bytes[HSN_DEVICE_PORT_COUNT];
static uint64_t prev_rx_bytes[HSN_DEVICE_PORT_COUNT];

/* Device-port name: D0..D3 (100G) then E0..E25 (10G). */
static void hsn_dev_name(int d, char *out, int n)
{
    if (d < 4) snprintf(out, n, "D%d", d);
    else       snprintf(out, n, "E%d", d - 4);
}

void hsn_stats_reset(void)
{
    for (int d = 0; d < HSN_DEVICE_PORT_COUNT; d++) {
        prev_tx_bytes[d] = 0;
        prev_rx_bytes[d] = 0;
    }
    for (int v = 0; v <= HSN_VL_ID_END; v++) {
        hsn_tx_pkts[v] = hsn_tx_bytes[v] = 0;
        hsn_rx_pkts[v] = hsn_rx_bytes[v] = 0;
        hsn_good_pkts[v] = hsn_bad_pkts[v] = 0;
        hsn_lost_pkts[v] = hsn_dup_pkts[v] = 0;
    }
}

void hsn_print_stats(uint32_t elapsed_sec, bool warmup, uint32_t warmup_left)
{
    if (warmup)
        printf("\n=== HSN PORT STATS -- WARM-UP (%3u s left) ===\n", warmup_left);
    else
        printf("\n=== HSN PORT STATS -- test %5u s ===\n", elapsed_sec);

    printf("%-4s %4s %9s %9s %12s %10s %10s %8s\n",
           "port", "spd", "TX Gbps", "RX Gbps", "good", "bad", "lost", "dup");

    double tot_tx = 0.0, tot_rx = 0.0;
    uint64_t tot_good = 0, tot_bad = 0, tot_lost = 0, tot_dup = 0;

    for (int d = 0; d < HSN_DEVICE_PORT_COUNT; d++) {
        const struct hsn_dev_port *e = &hsn_map[d];
        uint64_t txb = 0, rxb = 0, good = 0, bad = 0, lost = 0, dup = 0;

        for (uint16_t v = e->vl_start; v < e->vl_start + e->vl_count; v++) {
            txb  += hsn_tx_bytes[v];
            rxb  += hsn_rx_bytes[v];
            good += hsn_good_pkts[v];
            bad  += hsn_bad_pkts[v];
            lost += hsn_lost_pkts[v];
            dup  += hsn_dup_pkts[v];
        }

        /* per-second Gbps from byte delta (called once/sec) */
        double tx_gbps = (double)(txb - prev_tx_bytes[d]) * 8.0 / 1e9;
        double rx_gbps = (double)(rxb - prev_rx_bytes[d]) * 8.0 / 1e9;
        prev_tx_bytes[d] = txb;
        prev_rx_bytes[d] = rxb;

        tot_tx += tx_gbps; tot_rx += rx_gbps;
        tot_good += good; tot_bad += bad; tot_lost += lost; tot_dup += dup;

        char nm[8];
        hsn_dev_name(d, nm, sizeof(nm));
        printf("%-4s %3uG %9.2f %9.2f %12" PRIu64 " %10" PRIu64 " %10" PRIu64 " %8" PRIu64 "\n",
               nm, e->speed_gbps, tx_gbps, rx_gbps, good, bad, lost, dup);
    }

    printf("%-4s %4s %9.2f %9.2f %12" PRIu64 " %10" PRIu64 " %10" PRIu64 " %8" PRIu64 "\n",
           "TOT", "", tot_tx, tot_rx, tot_good, tot_bad, tot_lost, tot_dup);
}

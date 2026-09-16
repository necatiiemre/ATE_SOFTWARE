#include "VlFlowReport.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

#include "Config.h"
#include "Port.h"
#include "TxRxManager.h"
#include "AteMode.h"

#if STATS_MODE_CMC

static const struct cmc_dpm_block g_dpm_blocks[CMC_DPM_BLOCK_COUNT] =
    CMC_DPM_BLOCKS_INIT;

/* One VL-ID's worth of numbers, gathered from the TX counters and the RX
 * sequence tracker so the formatting code below never touches either. */
struct vl_row {
    uint16_t tx_vl;
    uint16_t rx_vl;
    uint64_t tx_pkts;
    uint64_t rx_pkts;
    uint64_t loss_txrx;
    uint64_t loss_seq;
    uint64_t first_seq;
    uint64_t last_seq;
    uint64_t max_seq;
    uint64_t expected_seq;
    int      seen;          /* RX tracker was ever initialized for this VL */
};

struct vl_totals {
    uint64_t tx_pkts;
    uint64_t rx_pkts;
    uint64_t loss_txrx;
    uint64_t loss_seq;
    uint32_t vls_with_loss;
    uint32_t vls_never_seen;
};

/*
 * In ATE mode the CMC is not in the path — the cables loop the traffic
 * straight back, so a packet returns on the VL-ID it was sent with. In unit
 * test mode the CMC remaps it by CMC_VL_RX_OFFSET. Everything downstream just
 * asks for the offset rather than re-deriving this.
 */
static uint16_t vlflow_rx_offset(void)
{
    return ate_mode_enabled() ? 0 : (uint16_t)CMC_VL_RX_OFFSET;
}

static void vlflow_collect(uint16_t cmc_port, uint16_t tx_vl, struct vl_row *row)
{
    const struct cmc_port_map_entry *entry = &cmc_port_map[cmc_port];
    const uint16_t srv_rx_port = entry->tx_server_port;

    memset(row, 0, sizeof(*row));
    row->tx_vl = tx_vl;
    row->rx_vl = (uint16_t)(tx_vl + vlflow_rx_offset());

    if (tx_vl <= MAX_VL_ID) {
        row->tx_pkts = vl_tx_counts[cmc_port][tx_vl];
    }

    if (srv_rx_port >= MAX_PORTS || row->rx_vl > MAX_VL_ID) {
        return;
    }

    const struct vl_sequence_tracker *t =
        &port_vl_trackers[srv_rx_port][cmc_port].vl_trackers[row->rx_vl];

    if (!t->initialized) {
        /* Nothing ever arrived on this VL — every transmitted packet is lost. */
        row->loss_txrx = row->tx_pkts;
        return;
    }

    row->seen         = 1;
    row->rx_pkts      = t->pkt_count;
    row->first_seq    = t->first_seq;
    row->last_seq     = t->last_seq;
    row->max_seq      = t->max_seq;
    row->expected_seq = t->expected_seq;

    if (row->tx_pkts > row->rx_pkts) {
        row->loss_txrx = row->tx_pkts - row->rx_pkts;
    }

    /* Sequences start at 0, so max_seq + 1 packets should have been seen. */
    uint64_t expected_count = row->max_seq + 1;
    if (expected_count > row->rx_pkts) {
        row->loss_seq = expected_count - row->rx_pkts;
    }
}

static void vlflow_accumulate(struct vl_totals *t, const struct vl_row *row)
{
    t->tx_pkts   += row->tx_pkts;
    t->rx_pkts   += row->rx_pkts;
    t->loss_txrx += row->loss_txrx;
    t->loss_seq  += row->loss_seq;
    if (row->loss_txrx > 0 || row->loss_seq > 0) t->vls_with_loss++;
    if (!row->seen)                              t->vls_never_seen++;
}

static double loss_pct(uint64_t lost, uint64_t sent)
{
    return (sent > 0) ? ((double)lost * 100.0 / (double)sent) : 0.0;
}

static void vlflow_timestamp(char *buf, size_t len)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    if (tm_info == NULL || strftime(buf, len, "%Y-%m-%d %H:%M:%S", tm_info) == 0) {
        snprintf(buf, len, "unknown");
    }
}

static void vlflow_write_file_header(FILE *f, uint32_t test_seconds)
{
    char ts[64];
    vlflow_timestamp(ts, sizeof(ts));
    const uint16_t off = vlflow_rx_offset();

    fprintf(f, "================================================================================\n");
    fprintf(f, " CMC VL-to-VL FLOW REPORT\n");
    fprintf(f, "================================================================================\n");
    fprintf(f, " Generated     : %s\n", ts);
    fprintf(f, " Test duration : %u s (after warm-up)\n", test_seconds);
    fprintf(f, " Mode          : %s\n",
            ate_mode_enabled() ? "ATE loopback (pure PRBS)"
                               : "Unit test (SplitMix64 + CRC32C + XOR + PRBS)");
    fprintf(f, " Lines         : %s (VLAN %u -> %u), %s (VLAN %u -> %u)\n",
            cmc_port_labels[0], cmc_port_map[0].rx_vlan, cmc_port_map[0].tx_vlan,
            cmc_port_labels[1], cmc_port_map[1].rx_vlan, cmc_port_map[1].tx_vlan);
    fprintf(f, " VL mapping    : TX %u..%u -> RX %u..%u (offset +%u)\n",
            CMC_TX_VL_ID_BASE, CMC_TX_VL_ID_BASE + CMC_TOTAL_VL_COUNT - 1,
            CMC_TX_VL_ID_BASE + off,
            CMC_TX_VL_ID_BASE + off + CMC_TOTAL_VL_COUNT - 1, off);
    fprintf(f, " Blocks        : %u DPM blocks x %u VL-IDs, on each of %u lines\n",
            CMC_DPM_BLOCK_COUNT, CMC_VLS_PER_DPM, CMC_PORT_COUNT);
    fprintf(f, "\n");
    fprintf(f, " TX pkts   = packets this line actually put on the wire for that VL-ID\n");
    fprintf(f, " RX pkts   = packets received back on the matching return VL-ID\n");
    fprintf(f, " Loss(T-R) = TX pkts - RX pkts (end to end)\n");
    fprintf(f, " Loss(seq) = (max_seq + 1) - RX pkts (sequence holes at the receiver)\n");
    fprintf(f, " '-' in the sequence columns means no packet ever arrived on that VL-ID\n");
    fprintf(f, "\n");
}

#define VL_TABLE_RULE \
    "  +--------+--------+--------------+--------------+--------------+---------+--------------+------------+------------+------------+------------+\n"

static void vlflow_write_table_header(FILE *f)
{
    fputs(VL_TABLE_RULE, f);
    fprintf(f, "  | %-6s | %-6s | %12s | %12s | %12s | %7s | %12s | %10s | %10s | %10s | %10s |\n",
            "TX VL", "RX VL", "TX pkts", "RX pkts", "Loss(T-R)", "Loss %",
            "Loss(seq)", "first_seq", "last_seq", "max_seq", "expect_seq");
    fputs(VL_TABLE_RULE, f);
}

static void vlflow_write_row(FILE *f, const struct vl_row *r)
{
    if (r->seen) {
        fprintf(f, "  | %6u | %6u | %12" PRIu64 " | %12" PRIu64 " | %12" PRIu64
                   " | %6.2f%% | %12" PRIu64 " | %10" PRIu64 " | %10" PRIu64
                   " | %10" PRIu64 " | %10" PRIu64 " |\n",
                r->tx_vl, r->rx_vl, r->tx_pkts, r->rx_pkts, r->loss_txrx,
                loss_pct(r->loss_txrx, r->tx_pkts), r->loss_seq,
                r->first_seq, r->last_seq, r->max_seq, r->expected_seq);
    } else {
        fprintf(f, "  | %6u | %6u | %12" PRIu64 " | %12" PRIu64 " | %12" PRIu64
                   " | %6.2f%% | %12s | %10s | %10s | %10s | %10s |\n",
                r->tx_vl, r->rx_vl, r->tx_pkts, r->rx_pkts, r->loss_txrx,
                loss_pct(r->loss_txrx, r->tx_pkts), "-", "-", "-", "-", "-");
    }
}

static void vlflow_write_totals(FILE *f, const char *label,
                                const struct vl_totals *t, uint32_t vl_count)
{
    fprintf(f, "  %-22s TX=%-12" PRIu64 " RX=%-12" PRIu64
               " Loss(T-R)=%-10" PRIu64 " (%.4f%%)  Loss(seq)=%-10" PRIu64
               "  VLs with loss: %u/%u  never seen: %u\n",
            label, t->tx_pkts, t->rx_pkts, t->loss_txrx,
            loss_pct(t->loss_txrx, t->tx_pkts), t->loss_seq,
            t->vls_with_loss, vl_count, t->vls_never_seen);
}

int vlflow_write_reports(const char *log_path, const char *csv_path,
                         uint32_t test_seconds)
{
    FILE *flog = NULL;
    FILE *fcsv = NULL;
    int   ret  = 0;

    if (log_path != NULL) {
        flog = fopen(log_path, "w");
        if (flog == NULL) {
            printf("VLFLOW: cannot open '%s' for writing\n", log_path);
            ret = -1;
        }
    }
    if (csv_path != NULL) {
        fcsv = fopen(csv_path, "w");
        if (fcsv == NULL) {
            printf("VLFLOW: cannot open '%s' for writing\n", csv_path);
            ret = -1;
        }
    }
    if (flog == NULL && fcsv == NULL) {
        return ret;
    }

    if (flog) vlflow_write_file_header(flog, test_seconds);
    if (fcsv) {
        fprintf(fcsv, "line,dpm,hm_vl,tx_vl,rx_vl,tx_pkts,rx_pkts,"
                      "loss_txrx,loss_pct,loss_seq,first_seq,last_seq,"
                      "max_seq,expected_seq,seen\n");
    }

    struct vl_totals grand = {0};
    uint32_t grand_vls = 0;

    for (uint16_t cmc = 0; cmc < CMC_PORT_COUNT; cmc++) {
        const char *line = cmc_port_labels[cmc];
        struct vl_totals line_tot = {0};
        uint32_t line_vls = 0;

        if (flog) {
            fprintf(flog, "\n");
            fprintf(flog, "################################################################################\n");
            fprintf(flog, "###  %s  (VLAN %u -> %u, SRC MAC tail 0x%02X)\n",
                    line, cmc_port_map[cmc].rx_vlan, cmc_port_map[cmc].tx_vlan,
                    (cmc == 0) ? CMC_NET_A_SRC_MAC_TAIL : CMC_NET_B_SRC_MAC_TAIL);
            fprintf(flog, "################################################################################\n");
        }

        for (uint16_t b = 0; b < CMC_DPM_BLOCK_COUNT; b++) {
            const struct cmc_dpm_block *blk = &g_dpm_blocks[b];
            struct vl_totals blk_tot = {0};

            if (flog) {
                fprintf(flog, "\n  === %s / %s (HM VL %u) : TX %u..%u -> RX %u..%u ===\n",
                        line, blk->label, blk->hm_vl_id,
                        blk->tx_vl_start, blk->tx_vl_start + blk->vl_count - 1,
                        (uint16_t)(blk->tx_vl_start + vlflow_rx_offset()),
                        (uint16_t)(blk->tx_vl_start + vlflow_rx_offset() + blk->vl_count - 1));
                vlflow_write_table_header(flog);
            }

            for (uint16_t i = 0; i < blk->vl_count; i++) {
                struct vl_row row;
                vlflow_collect(cmc, (uint16_t)(blk->tx_vl_start + i), &row);

                if (flog) vlflow_write_row(flog, &row);
                if (fcsv) {
                    fprintf(fcsv,
                            "%s,%s,%u,%u,%u,%" PRIu64 ",%" PRIu64 ",%" PRIu64
                            ",%.6f,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
                            ",%" PRIu64 ",%d\n",
                            line, blk->label, blk->hm_vl_id, row.tx_vl, row.rx_vl,
                            row.tx_pkts, row.rx_pkts, row.loss_txrx,
                            loss_pct(row.loss_txrx, row.tx_pkts), row.loss_seq,
                            row.first_seq, row.last_seq, row.max_seq,
                            row.expected_seq, row.seen);
                }

                vlflow_accumulate(&blk_tot, &row);
                vlflow_accumulate(&line_tot, &row);
                vlflow_accumulate(&grand, &row);
            }

            line_vls  += blk->vl_count;
            grand_vls += blk->vl_count;

            if (flog) {
                fputs(VL_TABLE_RULE, flog);
                char label[64];
                snprintf(label, sizeof(label), "%s TOTAL", blk->label);
                vlflow_write_totals(flog, label, &blk_tot, blk->vl_count);
            }
        }

        if (flog) {
            fprintf(flog, "\n");
            char label[64];
            snprintf(label, sizeof(label), "%s LINE TOTAL", line);
            vlflow_write_totals(flog, label, &line_tot, line_vls);
        }
    }

    if (flog) {
        fprintf(flog, "\n");
        fprintf(flog, "================================================================================\n");
        vlflow_write_totals(flog, "GRAND TOTAL", &grand, grand_vls);
        fprintf(flog, "================================================================================\n");
        fclose(flog);
        printf("VLFLOW: VL-to-VL table written to %s\n", log_path);
    }
    if (fcsv) {
        fclose(fcsv);
        printf("VLFLOW: VL-to-VL CSV written to %s\n", csv_path);
    }

    return ret;
}

void vlflow_print_summary(uint32_t test_seconds)
{
    printf("\n");
    printf("================================================================================\n");
    printf("  VL-to-VL SUMMARY  (per DPM block; full per-VL table is in %s)\n",
           CMC_VL_TABLE_LOG_PATH);
    printf("  Test duration: %u s after warm-up\n", test_seconds);
    printf("================================================================================\n");
    printf("  %-8s %-7s %-7s %14s %14s %12s %9s %12s\n",
           "Line", "Block", "HM VL", "TX pkts", "RX pkts", "Loss(T-R)", "Loss %",
           "Loss(seq)");
    printf("  ------------------------------------------------------------------------------------------------\n");

    struct vl_totals grand = {0};

    for (uint16_t cmc = 0; cmc < CMC_PORT_COUNT; cmc++) {
        struct vl_totals line_tot = {0};

        for (uint16_t b = 0; b < CMC_DPM_BLOCK_COUNT; b++) {
            const struct cmc_dpm_block *blk = &g_dpm_blocks[b];
            struct vl_totals blk_tot = {0};

            for (uint16_t i = 0; i < blk->vl_count; i++) {
                struct vl_row row;
                vlflow_collect(cmc, (uint16_t)(blk->tx_vl_start + i), &row);
                vlflow_accumulate(&blk_tot, &row);
                vlflow_accumulate(&line_tot, &row);
                vlflow_accumulate(&grand, &row);
            }

            printf("  %-8s %-7s %-7u %14" PRIu64 " %14" PRIu64 " %12" PRIu64
                   " %8.4f%% %12" PRIu64 "\n",
                   cmc_port_labels[cmc], blk->label, blk->hm_vl_id,
                   blk_tot.tx_pkts, blk_tot.rx_pkts, blk_tot.loss_txrx,
                   loss_pct(blk_tot.loss_txrx, blk_tot.tx_pkts), blk_tot.loss_seq);
        }

        printf("  %-8s %-7s %-7s %14" PRIu64 " %14" PRIu64 " %12" PRIu64
               " %8.4f%% %12" PRIu64 "\n",
               cmc_port_labels[cmc], "TOTAL", "-",
               line_tot.tx_pkts, line_tot.rx_pkts, line_tot.loss_txrx,
               loss_pct(line_tot.loss_txrx, line_tot.tx_pkts), line_tot.loss_seq);
        printf("  ------------------------------------------------------------------------------------------------\n");
    }

    printf("  %-8s %-7s %-7s %14" PRIu64 " %14" PRIu64 " %12" PRIu64
           " %8.4f%% %12" PRIu64 "\n",
           "ALL", "TOTAL", "-",
           grand.tx_pkts, grand.rx_pkts, grand.loss_txrx,
           loss_pct(grand.loss_txrx, grand.tx_pkts), grand.loss_seq);
    printf("  VL-IDs with loss: %u   VL-IDs that never received a packet: %u\n",
           grand.vls_with_loss, grand.vls_never_seen);
    printf("================================================================================\n");
}

#else /* !STATS_MODE_CMC */

int vlflow_write_reports(const char *log_path, const char *csv_path,
                         uint32_t test_seconds)
{
    (void)log_path; (void)csv_path; (void)test_seconds;
    printf("VLFLOW: VL-to-VL report needs STATS_MODE_CMC=1, skipping\n");
    return 0;
}

void vlflow_print_summary(uint32_t test_seconds)
{
    (void)test_seconds;
}

#endif /* STATS_MODE_CMC */

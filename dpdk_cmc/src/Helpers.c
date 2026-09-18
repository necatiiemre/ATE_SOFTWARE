#include "Helpers.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <rte_ethdev.h>
#include <rte_atomic.h>

#include "Config.h"
#include "Packet.h"       // for PACKET_SIZE (byte approximation for per-CMC stats)
#include "TxRxManager.h"  // for rx_stats_per_port
#include "AteMode.h"

// Daemon mode flag - when true, ANSI escape codes are disabled
bool g_daemon_mode = false;

void helper_set_daemon_mode(bool enabled) {
    g_daemon_mode = enabled;
}

// Helper functions
static inline double to_gbps(uint64_t bytes) {
    return (bytes * 8.0) / 1e9;
}

#if STATS_MODE_CMC
static void reset_cmc_prev_bytes(void);
#endif

void helper_reset_stats(const struct ports_config *ports_config,
                        uint64_t prev_tx_bytes[], uint64_t prev_rx_bytes[])
{
    // Reset HW statistics and zero out prev_* counters
    for (uint16_t i = 0; i < ports_config->nb_ports; i++) {
        uint16_t port_id = ports_config->ports[i].port_id;
        rte_eth_stats_reset(port_id);
        prev_tx_bytes[port_id] = 0;
        prev_rx_bytes[port_id] = 0;
    }

    // Reset RX validation statistics (PRBS) + per-VL sequence trackers
    init_rx_stats();

    // Reset TX VL-ID commit counters so shared-queue CMC RX
    // (Σ tx_vl_sequence) does not carry warm-up residue into the test window.
    reset_tx_vl_sequences();

#if STATS_MODE_CMC
    init_cmc_stats();
    reset_cmc_prev_bytes();
#endif
}

#if STATS_MODE_CMC
// ==========================================
// ==========================================
// CMC STATISTICS DISPLAY — ONE TABLE PER DSM LINE, ONE ROW PER DPM BLOCK
// ==========================================
// Each DSM line gets its own table; inside it, one row per DPM block
// (DPM-1..DPM-5) plus a line total.
//
// The figures come from the software counters, not the hardware per-queue
// ones. A queue carries a whole DSM line, so its counter cannot be broken
// down per DPM block, and it also folds in health-monitor traffic that has
// nothing to do with the PRBS stream. The software counters know what each
// packet was and which block its VL-ID fell in, which is exactly what this
// table needs. Non-PRBS traffic is reported on its own line underneath so
// nothing is hidden.
//
// Column naming follows the CMC's point of view, as before:
//   CMC TX (CMC→Server)  what came back to us  -> our RX counters
//   CMC RX (Server→CMC)  what we sent out      -> our TX counters

// Per-(line, block) byte baselines for the per-second rate columns.
static uint64_t cmc_prev_tx_bytes[CMC_PORT_COUNT][CMC_DPM_BLOCK_COUNT];
static uint64_t cmc_prev_rx_bytes[CMC_PORT_COUNT][CMC_DPM_BLOCK_COUNT];

static const struct cmc_dpm_block g_blocks[CMC_DPM_BLOCK_COUNT] = CMC_DPM_BLOCKS_INIT;

// Zero the baselines. Invoked from helper_reset_stats at the warm-up → test
// transition so the first test-window Gbps delta is computed against zero,
// not the warm-up byte total.
static void reset_cmc_prev_bytes(void)
{
    memset(cmc_prev_tx_bytes, 0, sizeof(cmc_prev_tx_bytes));
    memset(cmc_prev_rx_bytes, 0, sizeof(cmc_prev_rx_bytes));
}

// Packets this line put on the wire for one DPM block: the sum of the per-VL
// TX counters over the block's range. Deriving it from the same counters that
// back the VL-to-VL report keeps the two from drifting apart.
static uint64_t block_tx_pkts(uint16_t line, uint16_t block)
{
    const struct cmc_port_map_entry *e = &cmc_port_map[line];
    uint16_t base = (uint16_t)(e->tx_vl_id_start + block * CMC_VLS_PER_DPM);
    uint64_t sum = 0;

    for (uint16_t i = 0; i < CMC_VLS_PER_DPM; i++) {
        uint16_t vl = (uint16_t)(base + i);
        if (vl <= MAX_VL_ID) {
            sum += vl_tx_counts[line][vl];
        }
    }
    return sum;
}

#define CMC_ROW_FMT \
    "  │ %-7s │ %14lu │ %16lu │ %10.4f │ %14lu │ %16lu │ %10.4f │ %14lu │ %12lu │ %9lu │ %9lu │ %9lu │ %12lu │ %14lu │ %10.2e │\n"

#define CMC_RULE \
    "  ├─────────┼────────────────┼──────────────────┼────────────┼────────────────┼──────────────────┼────────────┼────────────────┼──────────────┼───────────┼───────────┼───────────┼──────────────┼────────────────┼────────────┤\n"
#define CMC_RULE_TOP \
    "  ┌─────────┬────────────────┬──────────────────┬────────────┬────────────────┬──────────────────┬────────────┬────────────────┬──────────────┬───────────┬───────────┬───────────┬──────────────┬────────────────┬────────────┐\n"
#define CMC_RULE_BOT \
    "  └─────────┴────────────────┴──────────────────┴────────────┴────────────────┴──────────────────┴────────────┴────────────────┴──────────────┴───────────┴───────────┴───────────┴──────────────┴────────────────┴────────────┘\n"

// One block's numbers, gathered so the row printer has nothing to work out.
struct cmc_row {
    uint64_t cmc_tx_pkts,  cmc_tx_bytes;   // came back to us
    uint64_t cmc_rx_pkts,  cmc_rx_bytes;   // we sent out
    double   cmc_tx_gbps,  cmc_rx_gbps;
    uint64_t good, bad, sm_fail, crc_fail, xor_fail, lost, bit_errors;
    double   ber;
};

static void cmc_row_gather(uint16_t line, uint16_t block, bool update_prev,
                           struct cmc_row *r)
{
    struct cmc_port_stats *st = &cmc_stats[line][block];

    memset(r, 0, sizeof(*r));
    r->cmc_tx_pkts  = (uint64_t)rte_atomic64_read(&st->total_rx_pkts);
    r->cmc_tx_bytes = (uint64_t)rte_atomic64_read(&st->rx_bytes);
    r->cmc_rx_pkts  = block_tx_pkts(line, block);
    r->cmc_rx_bytes = cmc_tx_bytes[line][block];

    r->good     = (uint64_t)rte_atomic64_read(&st->good_pkts);
    r->bad      = (uint64_t)rte_atomic64_read(&st->bad_pkts);
    r->sm_fail  = (uint64_t)rte_atomic64_read(&st->splitmix_fail);
    r->crc_fail = (uint64_t)rte_atomic64_read(&st->crc32_fail);
    r->xor_fail = (uint64_t)rte_atomic64_read(&st->xor_fail);
    r->lost     = (uint64_t)rte_atomic64_read(&st->lost_pkts);

    uint64_t bit_errors_raw = (uint64_t)rte_atomic64_read(&st->bit_errors);
#if IMIX_ENABLED
    uint64_t lost_bits = r->lost * (uint64_t)IMIX_AVG_PACKET_SIZE * 8;
#else
    uint64_t lost_bits = r->lost * (uint64_t)PACKET_SIZE * 8;
#endif
    r->bit_errors = bit_errors_raw + lost_bits;

    uint64_t total_bits = r->cmc_tx_bytes * 8 + lost_bits;
    r->ber = (total_bits > 0) ? ((double)r->bit_errors / (double)total_bits) : 0.0;

    // Rate columns are a delta against the previous print. Only the live table
    // advances the baseline; a snapshot reading the same counters must not
    // consume the delta the next live print is owed.
    if (update_prev) {
        r->cmc_tx_gbps = to_gbps(r->cmc_tx_bytes - cmc_prev_tx_bytes[line][block]);
        r->cmc_rx_gbps = to_gbps(r->cmc_rx_bytes - cmc_prev_rx_bytes[line][block]);
        cmc_prev_tx_bytes[line][block] = r->cmc_tx_bytes;
        cmc_prev_rx_bytes[line][block] = r->cmc_rx_bytes;
    }
}

static void cmc_row_add(struct cmc_row *acc, const struct cmc_row *r)
{
    acc->cmc_tx_pkts  += r->cmc_tx_pkts;
    acc->cmc_tx_bytes += r->cmc_tx_bytes;
    acc->cmc_rx_pkts  += r->cmc_rx_pkts;
    acc->cmc_rx_bytes += r->cmc_rx_bytes;
    acc->cmc_tx_gbps  += r->cmc_tx_gbps;
    acc->cmc_rx_gbps  += r->cmc_rx_gbps;
    acc->good         += r->good;
    acc->bad          += r->bad;
    acc->sm_fail      += r->sm_fail;
    acc->crc_fail     += r->crc_fail;
    acc->xor_fail     += r->xor_fail;
    acc->lost         += r->lost;
    acc->bit_errors   += r->bit_errors;
}

// Centre a label inside a column-group span. ASCII only on purpose: the width
// is a character count, and a multi-byte arrow would make strlen overstate the
// text and shift the group header off its separators.
static void print_centered(const char *text, int width)
{
    int len = (int)strlen(text);
    if (len >= width) {
        printf("%.*s", width, text);
        return;
    }
    int left = (width - len) / 2;
    printf("%*s%s%*s", left, "", text, width - len - left, "");
}

// Group-span widths: each is the sum of its columns' printed widths plus the
// separators between them, so the group header lines up with the rules.
//   TX / RX : Packets(16) + Bytes(18) + Gbps(12) + 2 separators      = 48
//   Verify  : Good(16)+Bad(14)+SM(11)+CRC(11)+XOR(11)+Loss(14)
//             +BitErr(16)+BER(12) + 7 separators                     = 112
#define CMC_SPAN_DIR    48
#define CMC_SPAN_VERIFY 112

static void cmc_row_print(const char *label, const struct cmc_row *r)
{
    printf(CMC_ROW_FMT, label,
           r->cmc_tx_pkts, r->cmc_tx_bytes, r->cmc_tx_gbps,
           r->cmc_rx_pkts, r->cmc_rx_bytes, r->cmc_rx_gbps,
           r->good, r->bad, r->sm_fail, r->crc_fail, r->xor_fail,
           r->lost, r->bit_errors, r->ber);
}

// Print one DSM line's table: five DPM rows plus the line total.
static void print_cmc_line_table(uint16_t line, bool update_prev)
{
    printf("\n");
    printf("  ═══ %s ═══ (VLAN %u → %u, SRC MAC tail 0x%02X | TX VL %u..%u → RX VL %u..%u)\n",
           cmc_port_labels[line],
           cmc_port_map[line].rx_vlan, cmc_port_map[line].tx_vlan,
           (line == 0) ? CMC_NET_A_SRC_MAC_TAIL : CMC_NET_B_SRC_MAC_TAIL,
           cmc_port_map[line].tx_vl_id_start,
           cmc_port_map[line].tx_vl_id_start + CMC_TOTAL_VL_COUNT - 1,
           cmc_port_map[line].vl_id_start,
           cmc_port_map[line].vl_id_start + CMC_TOTAL_VL_COUNT - 1);

    fputs(CMC_RULE_TOP, stdout);
    printf("  │ %-7s │", "");
    print_centered("CMC TX (CMC -> Server)", CMC_SPAN_DIR);
    printf("│");
    print_centered("CMC RX (Server -> CMC)", CMC_SPAN_DIR);
    printf("│");
    print_centered("Payload Verification", CMC_SPAN_VERIFY);
    printf("│\n");
    fputs(CMC_RULE, stdout);
    printf("  │ %-7s │ %14s │ %16s │ %10s │ %14s │ %16s │ %10s │ %14s │ %12s │ %9s │ %9s │ %9s │ %12s │ %14s │ %10s │\n",
           "Block", "Packets", "Bytes", "Gbps", "Packets", "Bytes", "Gbps",
           "Good", "Bad", "SM Fail", "CRC Fail", "XOR Fail", "Loss",
           "Bit Error", "BER");
    fputs(CMC_RULE, stdout);

    struct cmc_row total;
    memset(&total, 0, sizeof(total));

    for (uint16_t b = 0; b < CMC_DPM_BLOCK_COUNT; b++) {
        struct cmc_row row;
        cmc_row_gather(line, b, update_prev, &row);
        cmc_row_print(g_blocks[b].label, &row);
        cmc_row_add(&total, &row);
    }

    fputs(CMC_RULE, stdout);
    {
#if IMIX_ENABLED
        uint64_t lost_bits = total.lost * (uint64_t)IMIX_AVG_PACKET_SIZE * 8;
#else
        uint64_t lost_bits = total.lost * (uint64_t)PACKET_SIZE * 8;
#endif
        uint64_t total_bits = total.cmc_tx_bytes * 8 + lost_bits;
        total.ber = (total_bits > 0)
                        ? ((double)total.bit_errors / (double)total_bits) : 0.0;
        cmc_row_print("TOTAL", &total);
    }
    fputs(CMC_RULE_BOT, stdout);

    // Traffic that has no DPM block, kept out of the table above so the PRBS
    // figures stay clean but still visible.
    printf("  Non-PRBS on this line: health monitor %lu pkts / %lu bytes, "
           "other (undersized) %lu pkts / %lu bytes\n",
           (uint64_t)rte_atomic64_read(&cmc_line_stats[line].hm_rx_pkts),
           (uint64_t)rte_atomic64_read(&cmc_line_stats[line].hm_rx_bytes),
           (uint64_t)rte_atomic64_read(&cmc_line_stats[line].other_rx_pkts),
           (uint64_t)rte_atomic64_read(&cmc_line_stats[line].other_rx_bytes));
}
// ==========================================
// FINAL SNAPSHOT (end of run)
// ==========================================
// Same per-DPM tables as the live view -- deliberately the same printer, so
// the numbers an operator watched all run are the numbers the report ends on
// -- followed by the traffic that is not PRBS and a reconciliation against the
// hardware counters.
//
// The only difference from the live call is that the rate baselines are left
// alone: this reads the counters, it does not consume the delta.

void helper_print_prbs_summary(const struct ports_config *ports_config,
                               uint32_t test_seconds)
{
    // Zeroed up front: the loop only fills entries for configured ports, and
    // the accounting table below indexes by the port recorded in the CMC map.
    struct rte_eth_stats hw[MAX_PORTS];
    memset(hw, 0, sizeof(hw));

    for (uint16_t i = 0; i < ports_config->nb_ports; i++) {
        uint16_t port_id = ports_config->ports[i].port_id;
        if (rte_eth_stats_get(port_id, &hw[port_id]) != 0) {
            memset(&hw[port_id], 0, sizeof(struct rte_eth_stats));
        }
    }

    printf("\n");
    printf("════════════════════════════════════════════════════════════════════════════════\n");
    printf("  PRBS TRAFFIC — FINAL TOTALS PER DPM BLOCK (health-monitor traffic excluded)\n");
    printf("  Test duration: %u s after warm-up\n", test_seconds);
    printf("════════════════════════════════════════════════════════════════════════════════\n");

    uint64_t all_tx_pkts = 0, all_rx_pkts = 0;

    for (uint16_t line = 0; line < CMC_PORT_COUNT; line++) {
        print_cmc_line_table(line, /*update_prev=*/false);

        for (uint16_t b = 0; b < CMC_DPM_BLOCK_COUNT; b++) {
            all_tx_pkts += block_tx_pkts(line, b);
            all_rx_pkts += (uint64_t)rte_atomic64_read(&cmc_stats[line][b].total_rx_pkts);
        }
    }

    printf("\n  PRBS delivery, both lines: %lu of %lu packets returned", all_rx_pkts, all_tx_pkts);
    if (all_tx_pkts > 0) {
        uint64_t missing = (all_tx_pkts > all_rx_pkts) ? (all_tx_pkts - all_rx_pkts) : 0;
        printf("  (%lu missing, %.6f%%)",
               missing, (double)missing * 100.0 / (double)all_tx_pkts);
    }
    printf("\n");

    // ---- Health monitor, counted separately ----
    printf("\n");
    printf("  ═══ HEALTH MONITOR TRAFFIC ═══ (not included in the PRBS totals above)\n");
    printf("  ┌─────────┬──────────────┬────────────────┐\n");
    printf("  │ %-7s │ %12s │ %14s │\n", "Line", "HM RX pkts", "HM RX bytes");
    printf("  ├─────────┼──────────────┼────────────────┤\n");

    uint64_t t_hm_p = 0, t_hm_b = 0;
    for (uint16_t line = 0; line < CMC_PORT_COUNT; line++) {
        uint64_t hp = (uint64_t)rte_atomic64_read(&cmc_line_stats[line].hm_rx_pkts);
        uint64_t hb = (uint64_t)rte_atomic64_read(&cmc_line_stats[line].hm_rx_bytes);
        printf("  │ %-7s │ %12lu │ %14lu │\n", cmc_port_labels[line], hp, hb);
        t_hm_p += hp; t_hm_b += hb;
    }
    printf("  ├─────────┼──────────────┼────────────────┤\n");
    printf("  │ %-7s │ %12lu │ %14lu │\n", "TOTAL", t_hm_p, t_hm_b);
    printf("  └─────────┴──────────────┴────────────────┘\n");

    // ---- Reconciliation against the hardware counter ----
    // If this does not add up, one of the software counters is missing a path;
    // showing the difference is better than quietly presenting totals that do
    // not match the NIC.
    printf("\n");
    printf("  ═══ RX ACCOUNTING ═══ (hardware queue counter vs. classified traffic)\n");
    printf("  ┌─────────┬──────────────┬──────────────┬──────────────┬──────────────┬──────────────┐\n");
    printf("  │ %-7s │ %12s │ %12s │ %12s │ %12s │ %12s │\n",
           "Line", "HW RX pkts", "PRBS", "Health mon", "Other/short", "Unaccounted");
    printf("  ├─────────┼──────────────┼──────────────┼──────────────┼──────────────┼──────────────┤\n");

    for (uint16_t line = 0; line < CMC_PORT_COUNT; line++) {
        const struct cmc_port_map_entry *e = &cmc_port_map[line];
        uint64_t hw_p = 0;
        if (e->tx_server_port < MAX_PORTS &&
            e->tx_server_queue < RTE_ETHDEV_QUEUE_STAT_CNTRS) {
            hw_p = hw[e->tx_server_port].q_ipackets[e->tx_server_queue];
        }

        uint64_t prbs_p = 0;
        for (uint16_t b = 0; b < CMC_DPM_BLOCK_COUNT; b++) {
            prbs_p += (uint64_t)rte_atomic64_read(&cmc_stats[line][b].total_rx_pkts);
        }
        uint64_t hm_p    = (uint64_t)rte_atomic64_read(&cmc_line_stats[line].hm_rx_pkts);
        uint64_t other_p = (uint64_t)rte_atomic64_read(&cmc_line_stats[line].other_rx_pkts);
        uint64_t seen    = prbs_p + hm_p + other_p;

        char unacc[24];
        if (hw_p >= seen) {
            snprintf(unacc, sizeof(unacc), "%lu", hw_p - seen);
        } else {
            snprintf(unacc, sizeof(unacc), "-%lu", seen - hw_p);
        }

        printf("  │ %-7s │ %12lu │ %12lu │ %12lu │ %12lu │ %12s │\n",
               cmc_port_labels[line], hw_p, prbs_p, hm_p, other_p, unacc);
    }
    printf("  └─────────┴──────────────┴──────────────┴──────────────┴──────────────┴──────────────┘\n");
    printf("  Unaccounted counts packets the NIC delivered that no classifier claimed.\n");
    printf("  A small positive value right after shutdown is normal (a burst can land\n");
    printf("  between the last software flush and this snapshot).\n");
}

// ==========================================
// LIVE VIEW — one table per DSM line, printed every second
// ==========================================
static void helper_print_cmc_stats(const struct ports_config *ports_config,
                                   bool warmup_complete, unsigned loop_count,
                                   unsigned test_time)
{
    (void)ports_config;

    // Clear the screen
    if (!g_daemon_mode) {
        printf("\033[2J\033[H");
    } else {
        printf("\n========== [%s %u sec] ==========\n",
               warmup_complete ? "TEST" : "WARM-UP",
               warmup_complete ? test_time : loop_count);
    }

    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    if (!warmup_complete) {
        printf("║   CMC STATS — WARM-UP (%3u/120 sec)                                          ║\n",
               loop_count);
    } else {
        printf("║   CMC STATS — TEST Duration: %5u sec                                        ║\n",
               test_time);
    }
    printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");

    for (uint16_t line = 0; line < CMC_PORT_COUNT; line++) {
        print_cmc_line_table(line, /*update_prev=*/true);
    }

    // Warnings, per (line, block) so the operator is told which DPM to look at.
    bool has_warning = false;
    for (uint16_t line = 0; line < CMC_PORT_COUNT; line++) {
        for (uint16_t b = 0; b < CMC_DPM_BLOCK_COUNT; b++) {
            struct cmc_port_stats *st = &cmc_stats[line][b];
            uint64_t bad     = (uint64_t)rte_atomic64_read(&st->bad_pkts);
            uint64_t sm_fail = (uint64_t)rte_atomic64_read(&st->splitmix_fail);
            uint64_t crc_f   = (uint64_t)rte_atomic64_read(&st->crc32_fail);
            uint64_t xor_f   = (uint64_t)rte_atomic64_read(&st->xor_fail);
            uint64_t bit_err = (uint64_t)rte_atomic64_read(&st->bit_errors);
            uint64_t lost    = (uint64_t)rte_atomic64_read(&st->lost_pkts);

            if (bad == 0 && bit_err == 0 && lost == 0) {
                continue;
            }
            if (!has_warning) {
                printf("\n  WARNINGS:\n");
                has_warning = true;
            }
            if (bad > 0)
                printf("      %s %s: %lu bad packets! (SM:%lu CRC:%lu XOR:%lu)\n",
                       cmc_port_labels[line], g_blocks[b].label, bad, sm_fail, crc_f, xor_f);
            if (bit_err > 0)
                printf("      %s %s: %lu bit errors!\n",
                       cmc_port_labels[line], g_blocks[b].label, bit_err);
            if (lost > 0)
                printf("      %s %s: %lu lost packets!\n",
                       cmc_port_labels[line], g_blocks[b].label, lost);
        }
    }

    printf("\n  Press Ctrl+C to stop\n");
}

#endif /* STATS_MODE_CMC */

// ==========================================
// SERVER PORT-BASED STATISTICS TABLE (Legacy table)
// ==========================================
static void helper_print_server_stats(const struct ports_config *ports_config,
                                      const uint64_t prev_tx_bytes[],
                                      const uint64_t prev_rx_bytes[],
                                      bool warmup_complete, unsigned loop_count,
                                      unsigned test_time)
{
    // Clear screen (only in interactive mode, disabled in daemon mode for log files)
    if (!g_daemon_mode) {
        printf("\033[2J\033[H");
    } else {
        // Daemon mode: separator line between tables
        printf("\n========== [%s %u sec] ==========\n",
               warmup_complete ? "TEST" : "WARM-UP",
               warmup_complete ? test_time : loop_count);
    }

    // Header (240 characters wide)
    printf("╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    if (!warmup_complete) {
        printf("║                                                                    WARM-UP PHASE (%3u/120 sec) - Statistics will be reset at 120 seconds                                                                                        ║\n", loop_count);
    } else {
        printf("║                                                                    TEST IN PROGRESS - Test Duration: %5u sec                                                                                                                   ║\n", test_time);
    }
    printf("╚══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝\n\n");

    // Main statistics table (240 characters)
    printf("┌──────┬─────────────────────────────────────────────────────────────────────┬─────────────────────────────────────────────────────────────────────┬───────────────────────────────────────────────────────────────────────────────────────────────────┐\n");
    printf("│ Port │                            TX (Transmitted)                          │                            RX (Received)                            │                                      PRBS Verification                                              │\n");
    printf("│      ├─────────────────────┬─────────────────────┬─────────────────────────┼─────────────────────┬─────────────────────┬─────────────────────────┼─────────────────────┬─────────────────────┬─────────────────────┬─────────────────────┬─────────────┤\n");
    printf("│      │       Packets       │        Bytes        │          Gbps           │       Packets       │        Bytes        │          Gbps           │        Good         │         Bad         │        Lost         │      Bit Error      │     BER     │\n");
    printf("├──────┼─────────────────────┼─────────────────────┼─────────────────────────┼─────────────────────┼─────────────────────┼─────────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────┤\n");

    struct rte_eth_stats st;

    for (uint16_t i = 0; i < ports_config->nb_ports; i++) {
        uint16_t port_id = ports_config->ports[i].port_id;

        if (rte_eth_stats_get(port_id, &st) != 0) {
            printf("│  %2u  │         N/A         │         N/A         │           N/A           │         N/A         │         N/A         │           N/A           │         N/A         │         N/A         │         N/A         │         N/A         │     N/A     │\n", port_id);
            continue;
        }

        // HW statistics
        uint64_t tx_pkts = st.opackets;
        uint64_t tx_bytes = st.obytes;
        uint64_t rx_pkts = st.ipackets;
        uint64_t rx_bytes = st.ibytes;

        // Per-second rate calculation
        uint64_t tx_bytes_delta = tx_bytes - prev_tx_bytes[port_id];
        uint64_t rx_bytes_delta = rx_bytes - prev_rx_bytes[port_id];
        double tx_gbps = to_gbps(tx_bytes_delta);
        double rx_gbps = to_gbps(rx_bytes_delta);

        // PRBS verification statistics
        uint64_t good = rte_atomic64_read(&rx_stats_per_port[port_id].good_pkts);
        uint64_t bad = rte_atomic64_read(&rx_stats_per_port[port_id].bad_pkts);
        uint64_t lost = rte_atomic64_read(&rx_stats_per_port[port_id].lost_pkts);
        uint64_t bit_errors_raw = rte_atomic64_read(&rx_stats_per_port[port_id].bit_errors);

        // Include lost packets in bit_errors (each lost packet = all bits erroneous)
#if IMIX_ENABLED
        uint64_t lost_bits = lost * (uint64_t)IMIX_AVG_PACKET_SIZE * 8;
#else
        uint64_t lost_bits = lost * (uint64_t)PACKET_SIZE * 8;
#endif
        uint64_t bit_errors = bit_errors_raw + lost_bits;

        // Bit Error Rate (BER) calculation (lost packet bits added to total)
        double ber = 0.0;
        uint64_t total_bits = rx_bytes * 8 + lost_bits;
        if (total_bits > 0) {
            ber = (double)bit_errors / (double)total_bits;
        }

        // Print table row
        printf("│  %2u  │ %19lu │ %19lu │ %23.4f │ %19lu │ %19lu │ %23.4f │ %19lu │ %19lu │ %19lu │ %19lu │ %11.2e │\n",
               port_id,
               tx_pkts, tx_bytes, tx_gbps,
               rx_pkts, rx_bytes, rx_gbps,
               good, bad, lost, bit_errors, ber);
    }

    printf("└──────┴─────────────────────┴─────────────────────┴─────────────────────────┴─────────────────────┴─────────────────────┴─────────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────┘\n");

    // Warnings
    bool has_warning = false;
    for (uint16_t i = 0; i < ports_config->nb_ports; i++) {
        uint16_t port_id = ports_config->ports[i].port_id;

        uint64_t bad_pkts = rte_atomic64_read(&rx_stats_per_port[port_id].bad_pkts);
        uint64_t bit_errors = rte_atomic64_read(&rx_stats_per_port[port_id].bit_errors);
        uint64_t lost_pkts = rte_atomic64_read(&rx_stats_per_port[port_id].lost_pkts);

        if (bad_pkts > 0 || bit_errors > 0 || lost_pkts > 0) {
            if (!has_warning) {
                printf("\n  WARNINGS:\n");
                has_warning = true;
            }
            if (bad_pkts > 0) {
                printf("      Port %u: %lu bad packets detected!\n", port_id, bad_pkts);
            }
            if (bit_errors > 0) {
                printf("      Port %u: %lu bit errors detected!\n", port_id, bit_errors);
            }
            if (lost_pkts > 0) {
                printf("      Port %u: %lu lost packets detected!\n", port_id, lost_pkts);
            }
        }

        // HW missed packets check
        struct rte_eth_stats st2;
        if (rte_eth_stats_get(port_id, &st2) == 0 && st2.imissed > 0) {
            if (!has_warning) {
                printf("\n  WARNINGS:\n");
                has_warning = true;
            }
            printf("      Port %u: %lu packets missed by hardware (imissed)!\n", port_id, st2.imissed);
        }
    }

    printf("\n  Press Ctrl+C to stop\n");
}

// ==========================================
// PUBLIC API: helper_print_stats
// ==========================================
// CMC mode (STATS_MODE_CMC=1): per-network 2-table CMC display (Net A / Net B).
// Both ATE mode and unit-test mode use the same layout — the only difference
// is verification semantics, which are captured in the same per-CMC counters
// (Good/Bad/Lost/Bit Error). The SplitMix64 / CRC32 / XOR Fail columns stay
// at zero in ATE mode by design (the pure-PRBS path doesn't touch them).
// Legacy (STATS_MODE_CMC=0): single port-aggregate table.

void helper_print_stats(const struct ports_config *ports_config,
                        const uint64_t prev_tx_bytes[], const uint64_t prev_rx_bytes[],
                        bool warmup_complete, unsigned loop_count, unsigned test_time)
{
#if STATS_MODE_CMC
    helper_print_cmc_stats(ports_config, warmup_complete, loop_count, test_time);
    (void)prev_tx_bytes;
    (void)prev_rx_bytes;
#else
    helper_print_server_stats(ports_config, prev_tx_bytes, prev_rx_bytes,
                              warmup_complete, loop_count, test_time);
#endif
}
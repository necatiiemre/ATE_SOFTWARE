/**
 * @file VlFlowReport.h
 * @brief Per-VL-ID TX/RX flow report, written once at the end of a run.
 *
 * The live stats table aggregates everything into one row per DSM line. This
 * report is the opposite view: one row per VL-ID, so "which VL lost packets"
 * is answerable without re-running the test.
 *
 * Layout mirrors the physical topology — two DSM lines, each carrying the same
 * five DPM blocks of 104 VL-IDs:
 *
 *   DSM-A / DPM-1 .. DPM-5      DSM-B / DPM-1 .. DPM-5
 *
 * Block boundaries come from CMC_DPM_BLOCKS_INIT (Config.h), which matches
 * dpm_vl_bases() in the health monitor, so a row here can be compared
 * directly against the DPM's own COUNTERS_DPM_VL entry for the same flow.
 *
 * Two loss figures are reported per VL because they answer different
 * questions and disagreeing is informative:
 *   loss_txrx = tx_pkts - rx_pkts   — end to end: what we put on the wire
 *                                     versus what came back.
 *   loss_seq  = (max_seq + 1) - rx_pkts — sequence holes seen by the receiver.
 * A gap in the first with none in the second means packets never entered the
 * return path at all; the reverse means the tail is still in flight or
 * reordered.
 *
 * Call only after TX has stopped and the RX drain window has elapsed —
 * the counters are read without locking and are only stable once nothing
 * is writing to them.
 */
#ifndef VL_FLOW_REPORT_H
#define VL_FLOW_REPORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Write the VL-to-VL report to disk in both formats.
 *
 * @param log_path      aligned human-readable table (NULL skips it)
 * @param csv_path      one row per VL-ID with a header line (NULL skips it)
 * @param test_seconds  test duration after warm-up, for the file header
 * @return 0 if every requested file was written, -1 otherwise.
 */
int vlflow_write_reports(const char *log_path, const char *csv_path,
                         uint32_t test_seconds);

/**
 * Print the per-line / per-DPM roll-up (no per-VL rows) to stdout. Used by the
 * final-result snapshot, which captures stdout to its own file.
 */
void vlflow_print_summary(uint32_t test_seconds);

#ifdef __cplusplus
}
#endif

#endif /* VL_FLOW_REPORT_H */

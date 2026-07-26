/**
 * @file HsnStats.h
 * @brief HSN per-device-port statistics table (30 rows).
 *
 * Aggregates the per-VL software counters (from HsnWorker) into a
 * per-device-port view: TX/RX throughput plus PRBS and sequence results.
 * Pure C (no DPDK) so it can be unit-tested standalone.
 */
#ifndef HSN_STATS_H
#define HSN_STATS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Zero the cumulative per-VL counters and the per-port deltas.
 * Call at the warm-up boundary so the measured test starts from zero.
 * (Does NOT touch the RX sequence watermark, so loss keeps tracking.)
 */
void hsn_stats_reset(void);

/**
 * @brief Print the 30-row HSN device-port table.
 * @param elapsed_sec   seconds since test start (post warm-up)
 * @param warmup        true while still in warm-up
 * @param warmup_left   seconds of warm-up remaining (when warmup==true)
 *
 * Throughput is computed from the byte delta since the previous call, so
 * this must be invoked once per second (matching the main loop).
 */
void hsn_print_stats(uint32_t elapsed_sec, bool warmup, uint32_t warmup_left);

#endif /* HSN_STATS_H */

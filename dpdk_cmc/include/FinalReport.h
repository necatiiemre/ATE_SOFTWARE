/**
 * @file FinalReport.h
 * @brief End-of-run snapshot: the last stats table, the VL-to-VL roll-up and
 *        the health monitor's final state, captured into one file.
 *
 * Written after TX has stopped and the RX drain window has elapsed, so the
 * numbers in it are settled rather than a moving target.
 *
 * The stats and health-monitor printers are plain printf() code spread over
 * well past a thousand lines. Rather than thread a FILE* through all of it,
 * this module redirects fd 1 to the report file for the duration of the
 * snapshot and puts it back afterwards — the printers keep writing to stdout
 * and neither knows nor cares. It also forces daemon mode on while capturing
 * so the stats table does not emit screen-clearing ANSI escapes into a file.
 */
#ifndef FINAL_REPORT_H
#define FINAL_REPORT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ports_config;

/**
 * Capture the final snapshot to @p path.
 *
 * @param path           output file, truncated if it exists
 * @param ports_config   ports to read hardware counters from
 * @param prev_tx_bytes  main loop's per-port byte baselines (rate columns)
 * @param prev_rx_bytes  same, for RX
 * @param test_seconds   test duration after warm-up
 * @param warmup_complete false if the run never got past warm-up
 * @return 0 on success, -1 if the file could not be opened.
 */
int final_report_write(const char *path,
                       const struct ports_config *ports_config,
                       const uint64_t prev_tx_bytes[],
                       const uint64_t prev_rx_bytes[],
                       uint32_t test_seconds,
                       bool warmup_complete);

#ifdef __cplusplus
}
#endif

#endif /* FINAL_REPORT_H */

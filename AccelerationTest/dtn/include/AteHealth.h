/**
 * @file AteHealth.h
 * @brief The DTN health monitor, as the ATE software does it.
 *
 * The main ATE software polls the DTN once a second with a `0x52` read and prints
 * what comes back: both FPGAs' device status, all 35 ports with their counters,
 * and the MCU's rails and temperatures. That is the most informative thing the
 * device says about itself, and it is what the operator is used to reading.
 *
 * So it is not reimplemented here. dpdk/src/HealthMonitor/HealthMonitor.c is
 * copied verbatim into dtn/ate/ and its parsing and rendering are driven from
 * this test's own receive loop - see dtn/src/AteHealth.c for why it is included
 * rather than linked, and what the two stand-in headers beside it replace.
 *
 * The cycle is the ATE software's one second. What is shown is always the last
 * *complete* cycle, so a table is never half a second's worth of answers.
 *
 * The query is a different matter from the cycle, and off by default. The ATE
 * software polls every second, but its `0x52` query is addressed to VL 0 - that is
 * how a management frame is addressed - and VL 0 is a VL the VMC uses. One of those
 * a second for the length of a vibration run is a frame turning up at a unit that
 * means something else by it, so nothing is sent unless the rig is one where that
 * is safe; see app_config_dtn_health_poll. The DTN streams its health monitor
 * unprompted, so listening costs nothing.
 *
 * This is the DTN's own health monitor, on VL 38 from the internal management
 * port. The fibre-side unit's health monitor on VL 100 and 101 is a different
 * thing and is not decoded yet.
 */

#ifndef ATE_HEALTH_H
#define ATE_HEALTH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * How often the query goes out. The ATE software's interval, so the cycle this
 * shows covers the same second theirs does. The constant is repeated here rather
 * than taken from the copy's header so a caller does not have to include the ATE
 * software's header to drive it; AteHealth.c checks the two agree.
 */
#define ATE_HEALTH_QUERY_INTERVAL_MS 1000

/** The longest query ate_health_build_query can produce. */
#define ATE_HEALTH_QUERY_MAX 64

/** Forget everything, including the sequence counter. Call before a run. */
void ate_health_reset(void);

/**
 * @brief Build the `0x52` query the ATE software sends, and advance the sequence.
 *
 * Byte for byte the ATE software's template with its sequence rule (start 0x2F,
 * then 1..255, never 0 again). 64 bytes: unlike the configuration frames, this one
 * carries its sequence as the last byte *inside* the UDP length.
 *
 * @return frame length, or -1 if @p cap is too small
 */
int ate_health_build_query(uint8_t *out, size_t cap);

/**
 * @brief Close the cycle that was filling and start a new one.
 *
 * Called just before each query goes out, which is where the ATE software clears
 * its cycle. What was collected becomes the block that ate_health_render prints.
 */
void ate_health_cycle(void);

/**
 * @brief Feed one health response.
 *
 * Takes the whole frame from the wire, length included, because that is what the
 * ATE software's parser is given and it tells the six packet types apart by
 * length. The caller does the filtering the ATE software does with its VL check:
 * only frames on the DTN's health-monitor VL belong here.
 */
void ate_health_ingest(const uint8_t *frame, size_t len);

/**
 * @brief Print the last complete cycle, exactly as the ATE software prints it.
 * @return false when no cycle has completed yet, and nothing was printed
 */
bool ate_health_render(void);

/** Packets a full cycle carries: two for one FPGA, three for the other, one MCU. */
#define ATE_HEALTH_EXPECTED_RESPONSES 6

/** Packets in the last complete cycle, out of the six expected. */
unsigned ate_health_responses(void);

/** Queries built so far, and cycles that came back short. */
void ate_health_counts(uint64_t *queries, uint64_t *short_cycles);

#endif /* ATE_HEALTH_H */

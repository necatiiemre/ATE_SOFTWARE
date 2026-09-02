/**
 * @file Report.h
 * @brief What was injected and what came back.
 *
 * One row per VL. A VL that was sent and never returned is the finding: either
 * the DTN was not configured to forward it, or the path is broken. Taps are
 * counted as sent only - they leave through copper, where the acceleration test
 * on the workstation is the one that sees them.
 */

#ifndef REPORT_H
#define REPORT_H

#include "Scenario.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint64_t sent;
    uint64_t received;
    uint64_t wrong_vlan;    /**< came back, but not on the VLAN we expected */
    uint64_t last_ms;       /**< monotonic time of the last return */
} flow_result_t;

typedef struct {
    const scenario_t      *scenario;
    const scenario_flow_t *flows;
    size_t                 flow_count;
    flow_result_t          results[SCENARIO_MAX_FLOWS];
    uint64_t               unrecognised;  /**< frames that were not ours */
} report_t;

void report_init(report_t *report, const scenario_t *scenario,
                 const scenario_flow_t *flows, size_t count);

void report_sent(report_t *report, size_t flow_index);

/**
 * @brief Account for a frame that came back.
 * @return true if it matched a flow we are waiting for
 */
bool report_received(report_t *report, uint16_t vl_id, uint16_t vlan);

/** Milliseconds on a monotonic clock. */
uint64_t report_now_ms(void);

/** Redraw the per-link table in place while a run is going. */
void report_render_live(const report_t *report, uint64_t elapsed_s, uint64_t cycles);

/** Per-link rollup, printed at the end of a run. */
void report_render(const report_t *report);

/** True when every VL expecting a return got at least one. */
bool report_all_links_up(const report_t *report);

#endif /* REPORT_H */

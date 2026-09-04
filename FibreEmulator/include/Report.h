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

/**
 * @brief What the receive path saw, before any of it was matched to a flow.
 *
 * Without this a run cannot tell "the DTN forwards nothing" from "it forwards
 * and we fail to recognise it", which look identical in the loss column.
 */
typedef struct {
    uint64_t frames[8];     /**< every frame off the wire, per server port */
    uint64_t ours;          /**< carried our magic word */
    uint64_t foreign;       /**< something else on the link */
    uint64_t unmatched;     /**< ours, but no flow is waiting for that VL */
    uint64_t wrong_vlan;    /**< ours, matched, but not the VLAN we expected */
} rx_stats_t;

typedef struct {
    const scenario_t      *scenario;
    const scenario_flow_t *flows;
    size_t                 flow_count;
    flow_result_t          results[SCENARIO_MAX_FLOWS];
    rx_stats_t             rx;
} report_t;

void report_init(report_t *report, const scenario_t *scenario,
                 const scenario_flow_t *flows, size_t count);

void report_sent(report_t *report, size_t flow_index);

/** Note a frame off the wire, whatever it turns out to be. */
void report_frame_seen(report_t *report, uint8_t server_port);

/** Note a frame that did not carry our magic word. */
void report_foreign(report_t *report);

/**
 * @brief Account for one of our probes coming back.
 *
 * A probe is identified by its VL id alone. The VLAN it arrives on is checked
 * and reported, but a surprise there is not the same as a lost packet - NICs
 * strip tags, and a probe that came back on an unexpected VLAN still proves the
 * DTN forwarded it.
 *
 * @return true if a flow was waiting for that VL
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

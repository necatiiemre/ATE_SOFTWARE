/**
 * @file Scenario.h
 * @brief The three rounds, seen from the fibre side.
 *
 * The unit that will eventually sit on the DTN's fibre ports is not built yet,
 * so this application stands in for it: it injects a handful of packets on
 * every VL the DTN has been configured to carry and checks what comes back.
 * Nothing here cares about packet contents - only whether a VL arrives where
 * the configuration says it should.
 *
 * These rounds mirror the ones the acceleration test writes into the DTN. The
 * two applications are independent and must be kept in step by hand, which is
 * exactly what a run is meant to expose: a link the DTN never forwards shows up
 * as a VL that was sent and never came back.
 */

#ifndef SCENARIO_H
#define SCENARIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SCENARIO_MAX_LINKS  16
#define SCENARIO_MAX_TAPS    4
#define SCENARIO_MAX_FLOWS 512

/** One directed fibre link the DTN forwards: in at @p src, out at @p dst. */
typedef struct {
    uint8_t  src;
    uint8_t  dst;
    uint16_t vl_first;
    uint16_t vl_count;
} scenario_link_t;

/**
 * @brief A VL the DTN routes from a fibre port out to copper.
 *
 * We can inject it, but only the acceleration test on the workstation sees it
 * arrive - so a run reports it as sent and leaves the confirmation to the other
 * side.
 */
typedef struct {
    uint8_t  src;
    uint8_t  copper_port;
    uint16_t vl_id;
} scenario_tap_t;

typedef struct {
    const char     *name;
    const char     *description;
    uint8_t         link_count;
    scenario_link_t links[SCENARIO_MAX_LINKS];
    uint8_t         tap_count;
    scenario_tap_t  taps[SCENARIO_MAX_TAPS];
} scenario_t;

/** One VL to inject and, for a link, to wait for. */
typedef struct {
    uint16_t vl_id;
    uint8_t  src_dtn_port;
    uint8_t  dst_dtn_port;   /**< a copper port for a tap */
    uint8_t  tx_server_port;
    uint8_t  rx_server_port; /**< meaningless when !expect_return */
    uint16_t tx_vlan;
    uint16_t rx_vlan;
    bool     expect_return;  /**< false for taps, which land on the copper side */
} scenario_flow_t;

const scenario_t *scenario_all(size_t *count);

/**
 * @brief Expand a scenario into the individual VLs to inject.
 * @return flow count, or -1 if the scenario does not fit or maps to no port
 */
int scenario_expand(const scenario_t *scenario, scenario_flow_t *out, size_t cap);

/** Server ports a scenario transmits on and receives on, as bitmasks. */
void scenario_port_masks(const scenario_flow_t *flows, size_t count,
                         uint16_t *tx_mask, uint16_t *rx_mask);

#endif /* SCENARIO_H */

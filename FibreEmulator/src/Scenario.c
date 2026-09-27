#include "Scenario.h"

#include "FibreMap.h"

#include <string.h>

/* Ten VLs per direction, matching the acceleration test's rounds: the forward
 * direction numbers from 1024 and the reverse from 2024.
 *
 * Each round is written out on its own rather than generated from a shared
 * macro, because they no longer have a shape in common - and because the round
 * on this side has to be edited in step with the one on the other, so it is
 * worth being able to read one without decoding a macro.
 */

/* config2 and config3 still have the six-pair shape, so a macro still earns its
 * place for them. */
#define SIX_PAIRS(a, b)                                                          \
    {                                                                            \
        {(a) + 0, (b) + 0, 1024, 10}, {(a) + 1, (b) + 1, 1034, 10},              \
        {(a) + 2, (b) + 2, 1044, 10}, {(a) + 3, (b) + 3, 1054, 10},              \
        {(a) + 4, (b) + 4, 1064, 10}, {(a) + 5, (b) + 5, 1074, 10},              \
        {(b) + 0, (a) + 0, 2024, 10}, {(b) + 1, (a) + 1, 2034, 10},              \
        {(b) + 2, (a) + 2, 2044, 10}, {(b) + 3, (a) + 3, 2054, 10},              \
        {(b) + 4, (a) + 4, 2064, 10}, {(b) + 5, (a) + 5, 2074, 10},              \
    }

static const scenario_t g_scenarios[] = {
    /* ----------------------------------------------------------------
     * config1: four pairs of adjacent fibre ports.
     *
     * No taps. DTN ports 8 and 9 are cabled to the VMC in this round, not to
     * this emulator: the taps carry the VMC's health monitor and the copper legs
     * carry the workstation's traffic, and both of those happen on wires this
     * program is not on. Injecting them here would report traffic sent that
     * nothing could have carried.
     * ---------------------------------------------------------------- */
    {
        .name = "config1",
        .description = "fibre 0-3 <-> 4-7 (ports 8/9 belong to the VMC)",
        .link_count = 8,
        .links = {
            {0, 4, 1024, 10}, {1, 5, 1034, 10}, {2, 6, 1044, 10}, {3, 7, 1054, 10},
            {4, 0, 2024, 10}, {5, 1, 2034, 10}, {6, 2, 2044, 10}, {7, 3, 2054, 10},
        },
        .tap_count = 0,
    },

    /* config2 and config3 - unchanged. */
    {
        .name = "config2",
        .description = "fibre ports 6-11 <-> 22-27,  taps from ports 15 and 31",
        .link_count = 12,
        .links = SIX_PAIRS(6, 22),
        .tap_count = 2,
        .taps = {{15, 33, 100}, {31, 33, 101}},
    },
    /* Round 3 moves the taps: ports 15 and 31 carry fibre traffic here. */
    {
        .name = "config3",
        .description = "fibre ports 10-15 <-> 26-31, taps from ports 0 and 16",
        .link_count = 12,
        .links = SIX_PAIRS(10, 26),
        .tap_count = 2,
        .taps = {{0, 33, 100}, {16, 33, 101}},
    },
};

const scenario_t *scenario_all(size_t *count)
{
    if (count)
        *count = sizeof g_scenarios / sizeof g_scenarios[0];
    return g_scenarios;
}

int scenario_expand(const scenario_t *scenario, scenario_flow_t *out, size_t cap)
{
    size_t n = 0;

    for (uint8_t l = 0; l < scenario->link_count; l++) {
        const scenario_link_t *link = &scenario->links[l];
        int tx_port = fibre_server_port(link->src);
        int rx_port = fibre_rx_server_port(link->dst);   /* not the same map */

        if (tx_port < 0 || rx_port < 0)
            return -1;

        for (uint16_t k = 0; k < link->vl_count; k++) {
            if (n == cap)
                return -1;
            out[n++] = (scenario_flow_t){
                .vl_id          = (uint16_t)(link->vl_first + k),
                .src_dtn_port   = link->src,
                .dst_dtn_port   = link->dst,
                .tx_server_port = (uint8_t)tx_port,
                .rx_server_port = (uint8_t)rx_port,
                .tx_vlan        = fibre_tx_vlan(link->src),
                .rx_vlan        = fibre_rx_vlan(link->dst),
                .expect_return  = true,
            };
        }
    }

    for (uint8_t t = 0; t < scenario->tap_count; t++) {
        const scenario_tap_t *tap = &scenario->taps[t];
        int tx_port = fibre_server_port(tap->src);

        if (tx_port < 0)
            return -1;
        if (n == cap)
            return -1;
        out[n++] = (scenario_flow_t){
            .vl_id          = tap->vl_id,
            .src_dtn_port   = tap->src,
            .dst_dtn_port   = tap->copper_port,
            .tx_server_port = (uint8_t)tx_port,
            .tx_vlan        = fibre_tx_vlan(tap->src),
            .expect_return  = false,
        };
    }
    return (int)n;
}

void scenario_port_masks(const scenario_flow_t *flows, size_t count,
                         uint16_t *tx_mask, uint16_t *rx_mask)
{
    uint16_t tx = 0, rx = 0;

    for (size_t i = 0; i < count; i++) {
        tx |= (uint16_t)(1u << flows[i].tx_server_port);
        if (flows[i].expect_return)
            rx |= (uint16_t)(1u << flows[i].rx_server_port);
    }
    if (tx_mask)
        *tx_mask = tx;
    if (rx_mask)
        *rx_mask = rx;
}

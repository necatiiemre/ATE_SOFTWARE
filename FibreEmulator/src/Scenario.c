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

    /* ----------------------------------------------------------------
     * config2: config1 ten ports along. Ports 18 and 19 are the VMC's in this
     * round, so no taps here either.
     * ---------------------------------------------------------------- */
    {
        .name = "config2",
        .description = "fibre 10-13 <-> 14-17 (ports 18/19 belong to the VMC)",
        .link_count = 8,
        .links = {
            {10, 14, 1024, 10}, {11, 15, 1034, 10},
            {12, 16, 1044, 10}, {13, 17, 1054, 10},
            {14, 10, 2024, 10}, {15, 11, 2034, 10},
            {16, 12, 2044, 10}, {17, 13, 2054, 10},
        },
        .tap_count = 0,
    },
    /* ----------------------------------------------------------------
     * config3: the last twelve ports, so five pairs rather than four. Ports 30
     * and 31 are the VMC's, so no taps here either.
     * ---------------------------------------------------------------- */
    {
        .name = "config3",
        .description = "fibre 20-24 <-> 25-29 (ports 30/31 belong to the VMC)",
        .link_count = 10,
        .links = {
            {20, 25, 1024, 10}, {21, 26, 1034, 10}, {22, 27, 1044, 10},
            {23, 28, 1054, 10}, {24, 29, 1064, 10},
            {25, 20, 2024, 10}, {26, 21, 2034, 10}, {27, 22, 2044, 10},
            {28, 23, 2054, 10}, {29, 24, 2064, 10},
        },
        .tap_count = 0,
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

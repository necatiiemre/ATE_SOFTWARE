#include "Scenario.h"

#include "FibreMap.h"

#include <string.h>

/* Ten VLs per direction, matching the acceleration test's rounds: the forward
 * direction numbers from 1024 and the reverse from 2024. */
#define ROUND(nm, desc, a, b, tap0, tap1)                                        \
    {                                                                            \
        .name = nm, .description = desc,                                         \
        .link_count = 12,                                                        \
        .links = {                                                               \
            {a + 0, b + 0, 1024, 10}, {a + 1, b + 1, 1034, 10},                  \
            {a + 2, b + 2, 1044, 10}, {a + 3, b + 3, 1054, 10},                  \
            {a + 4, b + 4, 1064, 10}, {a + 5, b + 5, 1074, 10},                  \
            {b + 0, a + 0, 2024, 10}, {b + 1, a + 1, 2034, 10},                  \
            {b + 2, a + 2, 2044, 10}, {b + 3, a + 3, 2054, 10},                  \
            {b + 4, a + 4, 2064, 10}, {b + 5, a + 5, 2074, 10},                  \
        },                                                                       \
        .tap_count = 2,                                                          \
        .taps = {{tap0, 33, 100}, {tap1, 33, 101}},                              \
    }

static const scenario_t g_scenarios[] = {
    ROUND("config1", "fibre ports 0-5 <-> 16-21,   taps from ports 15 and 31",
          0, 16, 15, 31),
    ROUND("config2", "fibre ports 6-11 <-> 22-27,  taps from ports 15 and 31",
          6, 22, 15, 31),
    /* Round 3 moves the taps: ports 15 and 31 carry fibre traffic here. */
    ROUND("config3", "fibre ports 10-15 <-> 26-31, taps from ports 0 and 16",
          10, 26, 0, 16),
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
        int rx_port = fibre_server_port(link->dst);

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

#include "FibreMap.h"

#include <stddef.h>

/* Fixed by the cabling: each server port feeds four DTN ports through one
 * breakout on the Mellanox switch. Taken from the traffic generator's own
 * VLAN table, where server port N's transmit VLANs identify its DTN ports. */
static const uint8_t g_server_port_of_dtn[FIBRE_DTN_PORT_COUNT] = {
    2, 2, 2, 2,   /* DTN  0-3  */
    3, 3, 3, 3,   /* DTN  4-7  */
    0, 0, 0, 0,   /* DTN  8-11 */
    1, 1, 1, 1,   /* DTN 12-15 */
    4, 4, 4, 4,   /* DTN 16-19 */
    5, 5, 5, 5,   /* DTN 20-23 */
    6, 6, 6, 6,   /* DTN 24-27 */
    7, 7, 7, 7,   /* DTN 28-31 */
};

int fibre_server_port(uint8_t dtn_port)
{
    if (dtn_port >= FIBRE_DTN_PORT_COUNT)
        return -1;
    return g_server_port_of_dtn[dtn_port];
}

int fibre_dtn_port_from_rx_vlan(uint16_t vlan)
{
    if (vlan < 225 || vlan > 225 + FIBRE_DTN_PORT_COUNT - 1)
        return -1;
    return vlan - 225;
}

uint16_t fibre_server_port_mask(const uint8_t *dtn_ports, size_t count)
{
    uint16_t mask = 0;

    for (size_t i = 0; i < count; i++) {
        int server = fibre_server_port(dtn_ports[i]);
        if (server >= 0)
            mask |= (uint16_t)(1u << server);
    }
    return mask;
}

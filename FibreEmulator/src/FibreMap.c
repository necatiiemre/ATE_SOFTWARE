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

/* Where traffic from a DTN port comes back. The switch tags it with the port's
 * PVID (225 + port) and sends it out whichever trunk carries that VLAN, and
 * those trunks are not the transmit ones:
 *
 *     swp13  transmits to DTN  0-3   receives from DTN 20-23
 *     swp14  transmits to DTN  4-7   receives from DTN 16-19
 *     swp15  transmits to DTN  8-11  receives from DTN 28-31
 *     swp16  transmits to DTN 12-15  receives from DTN 24-27
 *     swp17  transmits to DTN 16-19  receives from DTN  4-7
 *     swp18  transmits to DTN 20-23  receives from DTN  0-3
 *     swp19  transmits to DTN 24-27  receives from DTN 12-15
 *     swp20  transmits to DTN 28-31  receives from DTN  8-11
 *
 * Read through g_server_port_of_dtn, that gives the table below. */
static const uint8_t g_rx_server_port_of_dtn[FIBRE_DTN_PORT_COUNT] = {
    5, 5, 5, 5,   /* DTN  0-3  come back on the port that sends to DTN 20-23 */
    4, 4, 4, 4,   /* DTN  4-7  ... to DTN 16-19 */
    7, 7, 7, 7,   /* DTN  8-11 ... to DTN 28-31 */
    6, 6, 6, 6,   /* DTN 12-15 ... to DTN 24-27 */
    3, 3, 3, 3,   /* DTN 16-19 ... to DTN  4-7  */
    2, 2, 2, 2,   /* DTN 20-23 ... to DTN  0-3  */
    1, 1, 1, 1,   /* DTN 24-27 ... to DTN 12-15 */
    0, 0, 0, 0,   /* DTN 28-31 ... to DTN  8-11 */
};

/* Fibre NICs, in the order the rig's traffic generator numbers them. */
static const char *const g_server_pci[FIBRE_SERVER_PORT_COUNT] = {
    "0000:21:00.0",  /* server port 0 */
    "0000:21:00.1",
    "0000:41:00.0",
    "0000:41:00.1",
    "0000:64:00.0",
    "0000:64:00.1",
    "0000:81:00.0",
    "0000:81:00.1",  /* server port 7 */
};

const char *fibre_server_pci(uint8_t server_port)
{
    if (server_port >= FIBRE_SERVER_PORT_COUNT)
        return NULL;
    return g_server_pci[server_port];
}

int fibre_server_port(uint8_t dtn_port)
{
    if (dtn_port >= FIBRE_DTN_PORT_COUNT)
        return -1;
    return g_server_port_of_dtn[dtn_port];
}

int fibre_rx_server_port(uint8_t dtn_port)
{
    if (dtn_port >= FIBRE_DTN_PORT_COUNT)
        return -1;
    return g_rx_server_port_of_dtn[dtn_port];
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

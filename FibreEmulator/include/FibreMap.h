/**
 * @file FibreMap.h
 * @brief How a DTN fibre port is reached from the server.
 *
 * The server's fibre ports do not touch the DTN directly; a Mellanox switch
 * sits between them and fans eight server ports out to the DTN's 32 fibre
 * ports. Which DTN port a frame ends up on is decided entirely by its VLAN tag:
 *
 *     server -> DTN   VLAN = 97 + dtn_port      (the switch strips the tag)
 *     DTN -> server   VLAN = 225 + dtn_port     (the switch adds it)
 *
 * Both rules hold for all 32 breakout ports in the switch's own configuration.
 * Which server port carries a given DTN port is fixed by the cabling.
 */

#ifndef FIBRE_MAP_H
#define FIBRE_MAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FIBRE_DTN_PORT_COUNT     32
#define FIBRE_SERVER_PORT_COUNT   8
#define FIBRE_PORTS_PER_SERVER    4

/** VLAN the server tags a frame with to have it leave the DTN's @p dtn_port. */
static inline uint16_t fibre_tx_vlan(uint8_t dtn_port) { return (uint16_t)(97 + dtn_port); }

/** VLAN a frame carries when it reaches the server from the DTN's @p dtn_port. */
static inline uint16_t fibre_rx_vlan(uint8_t dtn_port) { return (uint16_t)(225 + dtn_port); }

/** Server DPDK port carrying a DTN fibre port, or -1 if out of range. */
int fibre_server_port(uint8_t dtn_port);

/** DTN port a received VLAN belongs to, or -1 if it is not an RX VLAN. */
int fibre_dtn_port_from_rx_vlan(uint16_t vlan);

/** Which server ports a scenario needs, as a bitmask. */
uint16_t fibre_server_port_mask(const uint8_t *dtn_ports, size_t count);

#endif /* FIBRE_MAP_H */

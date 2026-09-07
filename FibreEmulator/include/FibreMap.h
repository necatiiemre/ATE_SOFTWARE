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
 *
 * Which server port a frame leaves and which one it comes back on are two
 * different questions, and the switch answers them differently. Each DTN-facing
 * breakout carries its transmit VLAN tagged and its receive VLAN as the PVID:
 *
 *     iface swp30s2          (DTN port 22)
 *         bridge-vids 119        97 + 22, tagged, from the server
 *         bridge-pvid 247       225 + 22, put on anything the DTN sends
 *
 * The receive VLAN then leaves through whichever server-facing trunk carries
 * it, and the switch's configuration does not pair those with the transmit
 * trunks. VLAN 247 is on swp13, which transmits to DTN 0-3 - so traffic from
 * DTN port 22 arrives on the server port that sends to DTN port 0, not on the
 * one that sends to DTN 22.
 *
 * Assuming the two were the same cost a round: config2 polled four server ports
 * while its returns were landing on four others, and reported every link as
 * losing everything. cumulus/interfaces is the authority for both maps and
 * tests/test_logic.c re-derives them from it.
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

/** Server DPDK port that transmits to a DTN fibre port, or -1 if out of range. */
int fibre_server_port(uint8_t dtn_port);

/**
 * @brief Server DPDK port that traffic from a DTN fibre port arrives on.
 *
 * Not the same as fibre_server_port - see the note above. Returns -1 if the
 * DTN port is out of range.
 */
int fibre_rx_server_port(uint8_t dtn_port);

/**
 * @brief PCI address of a server fibre port, or NULL if out of range.
 *
 * DPDK numbers ports in probe order, so an allowlist that names only some of
 * them renumbers the rest - and sending on the wrong port looks exactly like a
 * link that does not forward. Ports are therefore resolved by PCI address, and
 * the numbering DPDK happens to choose does not matter.
 *
 * The copper ports live on a different bus (01:00.x) and must stay bound to the
 * kernel: the acceleration test reaches them through AF_PACKET.
 */
const char *fibre_server_pci(uint8_t server_port);

/** DTN port a received VLAN belongs to, or -1 if it is not an RX VLAN. */
int fibre_dtn_port_from_rx_vlan(uint16_t vlan);

/** Which server ports transmit to these DTN ports, as a bitmask. */
uint16_t fibre_server_port_mask(const uint8_t *dtn_ports, size_t count);

#endif /* FIBRE_MAP_H */

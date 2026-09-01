/**
 * @file PortRunner.h
 * @brief The only part of this application that touches DPDK.
 *
 * A run injects a few packets on each of about 120 VLs and waits for them to
 * come back, so there is no throughput to chase: one RX and one TX queue per
 * port, driven from the main lcore, no workers. Keeping the DPDK surface this
 * small is deliberate - everything above it builds and tests on any machine.
 */

#ifndef PORT_RUNNER_H
#define PORT_RUNNER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Initialise EAL.
 * @param consumed set to the number of arguments EAL took
 * @return false if EAL refused to start
 */
bool port_runner_init(int argc, char **argv, int *consumed);

/**
 * @brief Configure and start the server ports named by @p server_port_mask.
 *
 * Ports are looked up by PCI address rather than by DPDK's own numbering, which
 * shifts with the EAL allowlist. Everything above this file talks in server
 * port numbers and never sees a DPDK port id.
 *
 * Ports come up promiscuous: the probes come back addressed to the multicast
 * MAC the DTN uses, not to the server's own.
 */
bool port_runner_open(uint16_t server_port_mask);

/** Put one frame on the wire. Retries briefly if the ring is full. */
bool port_runner_send(uint8_t server_port, const uint8_t *frame, size_t len);

/**
 * @brief Take the next received frame from a server port.
 * @return length, or 0 when nothing is waiting
 */
int port_runner_receive(uint8_t server_port, uint8_t *buf, size_t cap);

void port_runner_close(void);

#endif /* PORT_RUNNER_H */

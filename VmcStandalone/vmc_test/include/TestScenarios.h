#ifndef VMC_TEST_TEST_SCENARIOS_H_
#define VMC_TEST_TEST_SCENARIOS_H_

#include <stdbool.h>
#include <stdint.h>

#include "Packet.h"
#include "RawSocket.h"

/* Print every sent frame as hex (--hexdump). */
void scenarioSetHexDump(bool enabled);

/* Set from the SIGINT handler: running scenarios stop after the current packet. */
extern volatile int g_stopRequested;

/*
 * Send `count` packets on one network / VLAN / VL-IDX.
 *
 *   net        : NET_A or NET_B (SRC MAC tail 0x20 / 0x40)
 *   vlanId     : 802.1Q VLAN ID (0..4094)
 *   vlIdx      : VL-IDX (DST MAC / DST IP last 2 bytes)
 *   payloadLen : data bytes N (1..1466); byte N+1 is the sequence number
 *   count      : number of packets
 *   delayMs    : wait between two packets in milliseconds (0 = none)
 *
 * The sequence number comes from the network's own counter (Net A and Net B
 * are independent): 0, 1, ..., 255, 1, ...
 *
 * Returns the number of packets actually sent (== count on success),
 * or -1 for invalid arguments.
 */
int tryPacketSend(RawSocket *sock, Network net, uint16_t vlanId, uint16_t vlIdx,
                  uint16_t payloadLen, int count, unsigned delayMs);

#endif /* VMC_TEST_TEST_SCENARIOS_H_ */

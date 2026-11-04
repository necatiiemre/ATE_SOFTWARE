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

/*
 * End-system scenario:
 *   Phase 1: countA    x Net A (vlanIdA / vlIdxA)
 *   Phase 2: countB    x Net B (vlanIdB / vlIdxB)
 *   Phase 3: pairCount x (Net A, Net B)  ->  A B A B ...
 *
 * Same payload length for every packet; delayMs between every two
 * consecutive packets (also across phases and between A and B of a pair).
 * Net A and Net B sequence counters continue across the phases.
 *
 * Returns packets sent (== countA + countB + 2*pairCount on success),
 * or -1 for invalid arguments.
 */
int senaryoEndSystem(RawSocket *sock,
                     uint16_t vlanIdA, uint16_t vlIdxA,
                     uint16_t vlanIdB, uint16_t vlIdxB,
                     uint16_t payloadLen,
                     int countA, int countB, int pairCount,
                     unsigned delayMs);

#endif /* VMC_TEST_TEST_SCENARIOS_H_ */

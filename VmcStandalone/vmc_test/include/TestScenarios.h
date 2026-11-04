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
 *   ifname     : server interface; the raw socket is opened on it for this
 *                call and closed again at the end
 *   net        : NET_A or NET_B (SRC MAC tail 0x20 / 0x40)
 *   vlanId     : 802.1Q VLAN ID (0..4094)
 *   vlIdx      : VL-IDX (DST MAC / DST IP last 2 bytes)
 *   payloadLen : data bytes N (1..1466); byte N+1 is the sequence number
 *   count      : number of packets
 *   delayMs    : wait between two packets in milliseconds (0 = none)
 *   resetSeq   : true  -> this network's sequence restarts at 0
 *                false -> it continues from where the last packet left it
 *
 * The sequence number comes from the network's own counter (Net A and Net B
 * are independent): 0, 1, ..., 255, 1, ...
 *
 * Returns the number of packets actually sent (== count on success),
 * or -1 for invalid arguments / socket error.
 */
int tryPacketSend(const char *ifname, Network net, uint16_t vlanId, uint16_t vlIdx,
                  uint16_t payloadLen, int count, unsigned delayMs, bool resetSeq);

/*
 * End-system scenarios (FLCS and VS use the same flow):
 *   Phase 1: countA    x Net A (vlanIdA / vlIdxA)
 *   Phase 2: countB    x Net B (vlanIdB / vlIdxB)
 *   Phase 3: pairCount x (Net A, Net B)  ->  A B A B ...
 *            A and B of a pair carry the SAME sequence number. If the two
 *            counters differ when phase 3 starts, the one behind jumps to
 *            the one ahead (logged as a WARNING).
 *
 * ifname: server interface - the scenario opens its own raw socket on it
 * and closes it at the end.
 * Same payload length for every packet.
 *   delayMs   : wait between two consecutive packets (phases 1-2, across
 *               phases, and from the B of one pair to the A of the next)
 *   abDelayUs : wait between the A and the B of the SAME pair (phase 3),
 *               in MICROSECONDS; 0 = B right after A
 * Net A and Net B sequence counters continue across the phases.
 * resetSeq: true -> both A and B restart at 0 when the scenario starts;
 *           false -> both continue from the previous scenario.
 *
 * Returns packets sent (== countA + countB + 2*pairCount on success),
 * or -1 for invalid arguments / socket error.
 */
int senaryoFlcsEndSystem(const char *ifname,
                         uint16_t vlanIdA, uint16_t vlIdxA,
                         uint16_t vlanIdB, uint16_t vlIdxB,
                         uint16_t payloadLen,
                         int countA, int countB, int pairCount,
                         unsigned delayMs, unsigned abDelayUs, bool resetSeq);

int senaryoVsEndSystem(const char *ifname,
                       uint16_t vlanIdA, uint16_t vlIdxA,
                       uint16_t vlanIdB, uint16_t vlIdxB,
                       uint16_t payloadLen,
                       int countA, int countB, int pairCount,
                       unsigned delayMs, unsigned abDelayUs, bool resetSeq);

/*
 * Switch scenarios (FLCS and VS use the same flow): countPerVl packets on
 * ONE network and VLAN for each VL-IDX in vlIdxList, in list order.
 *
 *   ifname      : server interface (own raw socket, closed at the end)
 *   net         : NET_A or NET_B
 *   vlanId      : 802.1Q VLAN ID for every packet
 *   vlIdxList   : VL-IDX values, sent in this order
 *   vlIdxCount  : number of entries in vlIdxList
 *   payloadLen  : data bytes N (1..1466); byte N+1 is the sequence number
 *   countPerVl  : packets per VL-IDX
 *   delayMs     : wait between two consecutive packets (also across VL-IDX)
 *   resetSeq    : true -> the network's sequence restarts at 0 at the start;
 *                 false -> it continues. The counter is per network, so it
 *                 keeps counting across the VL-IDX blocks.
 *
 * Returns packets sent (== vlIdxCount * countPerVl on success), or -1.
 */
int senaryoFlcsSwitch(const char *ifname, Network net, uint16_t vlanId,
                      const uint16_t *vlIdxList, int vlIdxCount,
                      uint16_t payloadLen, int countPerVl,
                      unsigned delayMs, bool resetSeq);

int senaryoVsSwitch(const char *ifname, Network net, uint16_t vlanId,
                    const uint16_t *vlIdxList, int vlIdxCount,
                    uint16_t payloadLen, int countPerVl,
                    unsigned delayMs, bool resetSeq);

#endif /* VMC_TEST_TEST_SCENARIOS_H_ */

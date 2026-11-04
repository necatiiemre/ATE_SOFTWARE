#ifndef VMC_TEST_PACKET_H_
#define VMC_TEST_PACKET_H_

#include <stddef.h>
#include <stdint.h>

typedef enum {
    NET_A = 0,
    NET_B = 1,
    NET_COUNT
} Network;

const char *networkName(Network net);

/*
 * Build one complete Ethernet frame (see PacketDefs.h for the layout).
 *   dataLen : number of data bytes (N); the sequence byte is byte N+1
 *   seq     : written to every data byte and to the trailing sequence byte
 * Returns the frame length, or 0 if the arguments are invalid / buf too small.
 */
size_t buildPacket(uint8_t *buf, size_t bufSize, Network net, uint16_t vlanId,
                   uint16_t vlIdx, uint16_t dataLen, uint8_t seq);

/*
 * Next sequence value for a network, then advance its counter.
 * Separate counter per network: 0, 1, ..., 255, 1, 2, ... (0 only once).
 */
uint8_t nextSequence(Network net);

/* Sequence value the next packet of this network will carry (no advance). */
uint8_t peekSequence(Network net);

/* Restart a network's counter: its next packet carries sequence 0 again. */
void resetSequence(Network net);

/* Print a frame as hex (16 bytes per line). */
void hexDump(const uint8_t *buf, size_t len);

#endif /* VMC_TEST_PACKET_H_ */

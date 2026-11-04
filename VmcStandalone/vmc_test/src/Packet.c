#include "Packet.h"
#include "PacketDefs.h"

#include <stdio.h>
#include <string.h>

static uint8_t  g_nextSeq[NET_COUNT];          /* starts at 0 */

const char *networkName(Network net)
{
    switch (net) {
    case NET_A: return "NetA";
    case NET_B: return "NetB";
    default:    return "Net?";
    }
}

uint8_t nextSequence(Network net)
{
    if (net < 0 || net >= NET_COUNT) return 0;
    uint8_t seq = g_nextSeq[net];
    g_nextSeq[net] = (seq == 255) ? 1 : (uint8_t)(seq + 1);   /* 255 -> 1, never back to 0 */
    return seq;
}

static void put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static void put32(uint8_t *p, uint32_t v) { put16(p, (uint16_t)(v >> 16)); put16(p + 2, (uint16_t)v); }

static uint16_t ipv4Checksum(const uint8_t *hdr)
{
    uint32_t sum = 0;
    for (int i = 0; i < IPV4_HDR_LEN; i += 2)
        sum += (uint32_t)((hdr[i] << 8) | hdr[i + 1]);
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

size_t buildPacket(uint8_t *buf, size_t bufSize, Network net, uint16_t vlanId,
                   uint16_t vlIdx, uint16_t dataLen, uint8_t seq)
{
    if (net < 0 || net >= NET_COUNT) return 0;
    if (vlanId > MAX_VLAN_ID) return 0;
    if (dataLen < MIN_DATA_LEN || dataLen > MAX_DATA_LEN) return 0;

    const size_t udpPayloadLen = (size_t)dataLen + SEQ_LEN;
    const size_t frameLen = HEADERS_LEN + udpPayloadLen;
    if (!buf || bufSize < frameLen) return 0;

    memset(buf, 0, frameLen);
    uint8_t *p = buf;

    /* ---- Ethernet ---- */
    static const uint8_t dstPrefix[4] = DST_MAC_PREFIX;
    static const uint8_t srcPrefix[5] = SRC_MAC_PREFIX;
    memcpy(p, dstPrefix, 4);
    put16(p + 4, vlIdx);                                      /* DST MAC = VL-IDX */
    memcpy(p + 6, srcPrefix, 5);
    p[11] = (net == NET_A) ? NET_A_SRC_MAC_TAIL : NET_B_SRC_MAC_TAIL;
    put16(p + 12, ETHERTYPE_VLAN);
    p += ETH_HDR_LEN;

    /* ---- 802.1Q ---- */
    put16(p, (uint16_t)(vlanId & 0x0FFF));                    /* priority 0, CFI 0 */
    put16(p + 2, ETHERTYPE_IPV4);
    p += VLAN_HDR_LEN;

    /* ---- IPv4 ---- */
    uint8_t *ip = p;
    ip[0] = 0x45;                                             /* v4, IHL 5 */
    ip[1] = PKT_TOS;
    put16(ip + 2, (uint16_t)(IPV4_HDR_LEN + UDP_HDR_LEN + udpPayloadLen));
    put16(ip + 4, 0);                                         /* ID */
    put16(ip + 6, 0);                                         /* flags/fragment */
    ip[8] = PKT_TTL;
    ip[9] = 17;                                               /* UDP */
    put32(ip + 12, SRC_IP);
    put32(ip + 16, DST_IP_PREFIX | vlIdx);                    /* 224.224.VV.VV */
    put16(ip + 10, ipv4Checksum(ip));
    p += IPV4_HDR_LEN;

    /* ---- UDP ---- */
    put16(p, UDP_SRC_PORT);
    put16(p + 2, UDP_DST_PORT);
    put16(p + 4, (uint16_t)(UDP_HDR_LEN + udpPayloadLen));
    put16(p + 6, 0);                                          /* checksum disabled */
    p += UDP_HDR_LEN;

    /* ---- Payload: N data bytes = seq, then the seq byte ---- */
    memset(p, seq, dataLen);
    p[dataLen] = seq;

    return frameLen;
}

void hexDump(const uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i += 16) {
        printf("    %04zx ", i);
        for (size_t j = i; j < i + 16 && j < len; ++j) printf(" %02x", buf[j]);
        printf("\n");
    }
}

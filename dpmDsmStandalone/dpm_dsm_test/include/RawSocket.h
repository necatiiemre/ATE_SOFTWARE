#ifndef VMC_TEST_RAW_SOCKET_H_
#define VMC_TEST_RAW_SOCKET_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* AF_PACKET raw socket bound to one interface; sends complete Ethernet
 * frames (we build every header ourselves, VLAN tag included). Needs root
 * or CAP_NET_RAW. */
typedef struct {
    int  fd;
    int  ifindex;
    char ifname[32];
} RawSocket;

/* 0 on success, -1 on error (message printed). */
int  rawSocketOpen(RawSocket *s, const char *ifname);

/* 0 on success, -1 on error (message printed). */
int  rawSocketSend(RawSocket *s, const unsigned char *frame, size_t len);

void rawSocketClose(RawSocket *s);

/* ---------------- Receive side (used by the PTP scenario) ----------------
 * A separate AF_PACKET socket that receives every frame arriving on the
 * interface. Frames we send ourselves are skipped. The kernel strips the
 * 802.1Q tag on receive; the VLAN is reported back through PACKET_AUXDATA
 * and the frame is handed out WITHOUT the tag (if a NIC leaves the tag in
 * the data instead, it is removed and reported the same way). */

typedef struct {
    uint16_t etherType;     /* EtherType after the (removed) VLAN tag */
    bool     hasVlan;
    uint16_t vlanId;
    uint64_t rxTimeNs;      /* kernel RX timestamp, CLOCK_REALTIME ns */
    size_t   len;           /* frame length WITHOUT the VLAN tag */
} RawRxInfo;

/* 0 on success, -1 on error (message printed). */
int  rawSocketOpenRx(RawSocket *s, const char *ifname);

/* Wait up to timeoutMs for one incoming frame (0 = don't wait).
 * Returns 1 if a frame was stored in buf (VLAN tag removed, see info),
 *         0 on timeout, -1 on error. */
int  rawSocketRecv(RawSocket *s, uint8_t *buf, size_t bufSize, RawRxInfo *info, int timeoutMs);

#endif /* VMC_TEST_RAW_SOCKET_H_ */

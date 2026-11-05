#ifndef VMC_TEST_RAW_SOCKET_H_
#define VMC_TEST_RAW_SOCKET_H_

#include <stddef.h>

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

#endif /* VMC_TEST_RAW_SOCKET_H_ */

#include "RawSocket.h"

#include <errno.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

int rawSocketOpen(RawSocket *s, const char *ifname)
{
    memset(s, 0, sizeof(*s));
    s->fd = -1;

    if (!ifname || strlen(ifname) >= sizeof(s->ifname)) {
        fprintf(stderr, "[RawSocket] invalid interface name\n");
        return -1;
    }
    snprintf(s->ifname, sizeof(s->ifname), "%s", ifname);

    s->ifindex = (int)if_nametoindex(ifname);
    if (s->ifindex == 0) {
        fprintf(stderr, "[RawSocket] interface '%s' not found: %s\n", ifname, strerror(errno));
        return -1;
    }

    /* protocol 0: TX only, the socket receives nothing. */
    s->fd = socket(AF_PACKET, SOCK_RAW, 0);
    if (s->fd < 0) {
        fprintf(stderr, "[RawSocket] socket(AF_PACKET) failed: %s%s\n", strerror(errno),
                errno == EPERM ? " (run with sudo)" : "");
        return -1;
    }

    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family  = AF_PACKET;
    sll.sll_ifindex = s->ifindex;
    if (bind(s->fd, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
        fprintf(stderr, "[RawSocket] bind to '%s' failed: %s\n", ifname, strerror(errno));
        close(s->fd);
        s->fd = -1;
        return -1;
    }

    /* Warn early if the link is administratively down. */
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
    if (ioctl(s->fd, SIOCGIFFLAGS, &ifr) == 0 && !(ifr.ifr_flags & IFF_UP)) {
        fprintf(stderr, "[RawSocket] WARNING: interface '%s' is DOWN\n", ifname);
    }

    printf("[RawSocket] opened on %s (ifindex %d)\n", s->ifname, s->ifindex);
    return 0;
}

int rawSocketSend(RawSocket *s, const unsigned char *frame, size_t len)
{
    if (!s || s->fd < 0) return -1;

    struct sockaddr_ll dst;
    memset(&dst, 0, sizeof(dst));
    dst.sll_family  = AF_PACKET;
    dst.sll_ifindex = s->ifindex;
    dst.sll_halen   = 6;
    memcpy(dst.sll_addr, frame, 6);   /* DST MAC from the frame itself */

    ssize_t n = sendto(s->fd, frame, len, 0, (struct sockaddr *)&dst, sizeof(dst));
    if (n < 0) {
        fprintf(stderr, "[RawSocket] sendto failed on %s: %s\n", s->ifname, strerror(errno));
        return -1;
    }
    if ((size_t)n != len) {
        fprintf(stderr, "[RawSocket] short send on %s: %zd/%zu bytes\n", s->ifname, n, len);
        return -1;
    }
    return 0;
}

void rawSocketClose(RawSocket *s)
{
    if (s && s->fd >= 0) {
        close(s->fd);
        s->fd = -1;
    }
}

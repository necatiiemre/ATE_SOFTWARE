#include "RawSocket.h"

#include <arpa/inet.h>
#include <errno.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <poll.h>
#include <time.h>
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

/* ==================== Receive side ==================== */

int rawSocketOpenRx(RawSocket *s, const char *ifname)
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

    s->fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (s->fd < 0) {
        fprintf(stderr, "[RawSocket] RX socket(AF_PACKET) failed: %s%s\n", strerror(errno),
                errno == EPERM ? " (run with sudo)" : "");
        return -1;
    }

    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family   = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex  = s->ifindex;
    if (bind(s->fd, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
        fprintf(stderr, "[RawSocket] RX bind to '%s' failed: %s\n", ifname, strerror(errno));
        close(s->fd);
        s->fd = -1;
        return -1;
    }

    int one = 1;
    if (setsockopt(s->fd, SOL_PACKET, PACKET_AUXDATA, &one, sizeof(one)) < 0) {
        fprintf(stderr, "[RawSocket] WARNING: PACKET_AUXDATA not available (%s) - "
                        "VLAN of received frames may be unknown\n", strerror(errno));
    }
    if (setsockopt(s->fd, SOL_SOCKET, SO_TIMESTAMPNS, &one, sizeof(one)) < 0) {
        fprintf(stderr, "[RawSocket] WARNING: SO_TIMESTAMPNS not available (%s) - "
                        "using user-space RX time\n", strerror(errno));
    }

    printf("[RawSocket] RX opened on %s (ifindex %d)\n", s->ifname, s->ifindex);
    return 0;
}

int rawSocketRecv(RawSocket *s, uint8_t *buf, size_t bufSize, RawRxInfo *info, int timeoutMs)
{
    if (!s || s->fd < 0 || !buf || !info) return -1;

    for (;;) {
        struct pollfd pfd = { s->fd, POLLIN, 0 };
        int pr = poll(&pfd, 1, timeoutMs);
        if (pr < 0) {
            if (errno == EINTR) return 0;          /* let the caller check Ctrl+C */
            fprintf(stderr, "[RawSocket] poll failed on %s: %s\n", s->ifname, strerror(errno));
            return -1;
        }
        if (pr == 0) return 0;

        struct sockaddr_ll from;
        union {
            struct cmsghdr align;
            char buf[CMSG_SPACE(sizeof(struct tpacket_auxdata)) +
                     CMSG_SPACE(sizeof(struct timespec))];
        } ctrl;
        struct iovec iov = { buf, bufSize };
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_name       = &from;
        msg.msg_namelen    = sizeof(from);
        msg.msg_iov        = &iov;
        msg.msg_iovlen     = 1;
        msg.msg_control    = ctrl.buf;
        msg.msg_controllen = sizeof(ctrl.buf);

        ssize_t n = recvmsg(s->fd, &msg, MSG_DONTWAIT);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                if (timeoutMs == 0) return 0;
                continue;
            }
            fprintf(stderr, "[RawSocket] recvmsg failed on %s: %s\n", s->ifname, strerror(errno));
            return -1;
        }

        /* Our own transmissions are looped back to packet sockets: skip. */
        if (from.sll_pkttype == PACKET_OUTGOING) continue;
        if (n < 14) continue;

        memset(info, 0, sizeof(*info));
        for (struct cmsghdr *c = CMSG_FIRSTHDR(&msg); c; c = CMSG_NXTHDR(&msg, c)) {
            if (c->cmsg_level == SOL_PACKET && c->cmsg_type == PACKET_AUXDATA) {
                struct tpacket_auxdata aux;
                memcpy(&aux, CMSG_DATA(c), sizeof(aux));
                if (aux.tp_status & TP_STATUS_VLAN_VALID) {
                    info->hasVlan = true;
                    info->vlanId  = aux.tp_vlan_tci & 0x0FFF;
                }
            } else if (c->cmsg_level == SOL_SOCKET && c->cmsg_type == SO_TIMESTAMPNS) {
                struct timespec ts;
                memcpy(&ts, CMSG_DATA(c), sizeof(ts));
                info->rxTimeNs = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
            }
        }
        if (info->rxTimeNs == 0) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            info->rxTimeNs = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
        }

        size_t len = (size_t)n;
        uint16_t et = (uint16_t)((buf[12] << 8) | buf[13]);

        /* Tag still in the data (no kernel/NIC untagging): remove it. */
        if (et == 0x8100 && len >= 18) {
            info->hasVlan = true;
            info->vlanId  = (uint16_t)(((buf[14] << 8) | buf[15]) & 0x0FFF);
            et = (uint16_t)((buf[16] << 8) | buf[17]);
            memmove(buf + 12, buf + 16, len - 16);
            len -= 4;
        }

        info->etherType = et;
        info->len       = len;
        return 1;
    }
}

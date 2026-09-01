#include "RawSocket.h"

#include <errno.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

bool raw_socket_link_up(const char *iface, bool *carrier)
{
    struct ifreq ifr;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0)
        return false;

    memset(&ifr, 0, sizeof ifr);
    snprintf(ifr.ifr_name, IFNAMSIZ, "%s", iface);
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        close(fd);
        return false;
    }
    close(fd);

    if (carrier)
        *carrier = (ifr.ifr_flags & IFF_RUNNING) != 0;
    return (ifr.ifr_flags & IFF_UP) != 0;
}

bool raw_socket_open(raw_socket_t *sock, const char *iface, bool promiscuous)
{
    struct ifreq ifr;
    struct sockaddr_ll sll;

    sock->fd = -1;
    snprintf(sock->name, sizeof sock->name, "%s", iface);

    sock->fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock->fd < 0) {
        fprintf(stderr, "[net] cannot open a raw socket: %s%s\n", strerror(errno),
                errno == EPERM ? " (needs root or CAP_NET_RAW)" : "");
        return false;
    }

    memset(&ifr, 0, sizeof ifr);
    snprintf(ifr.ifr_name, IFNAMSIZ, "%s", iface);
    if (ioctl(sock->fd, SIOCGIFINDEX, &ifr) < 0) {
        fprintf(stderr, "[net] no such interface: %s\n", iface);
        raw_socket_close(sock);
        return false;
    }
    sock->ifindex = ifr.ifr_ifindex;

    memset(&sll, 0, sizeof sll);
    sll.sll_family   = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex  = sock->ifindex;
    if (bind(sock->fd, (struct sockaddr *)&sll, sizeof sll) < 0) {
        fprintf(stderr, "[net] cannot bind to %s: %s\n", iface, strerror(errno));
        raw_socket_close(sock);
        return false;
    }

    if (promiscuous) {
        struct packet_mreq mreq;
        memset(&mreq, 0, sizeof mreq);
        mreq.mr_ifindex = sock->ifindex;
        mreq.mr_type    = PACKET_MR_PROMISC;
        if (setsockopt(sock->fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
                       &mreq, sizeof mreq) < 0)
            fprintf(stderr, "[net] %s: promiscuous mode refused: %s\n",
                    iface, strerror(errno));
    }
    return true;
}

void raw_socket_close(raw_socket_t *sock)
{
    if (sock->fd >= 0)
        close(sock->fd);
    sock->fd = -1;
}

bool raw_socket_send(raw_socket_t *sock, const uint8_t *frame, size_t len)
{
    struct sockaddr_ll dest;

    memset(&dest, 0, sizeof dest);
    dest.sll_family  = AF_PACKET;
    dest.sll_ifindex = sock->ifindex;
    dest.sll_halen   = ETH_ALEN;
    memcpy(dest.sll_addr, frame, ETH_ALEN);   /* destination MAC */

    ssize_t sent = sendto(sock->fd, frame, len, 0,
                          (struct sockaddr *)&dest, sizeof dest);
    if (sent != (ssize_t)len) {
        fprintf(stderr, "[net] %s: send failed: %s\n", sock->name, strerror(errno));
        return false;
    }
    return true;
}

int raw_socket_recv(raw_socket_t *sock, uint8_t *buf, size_t cap, unsigned timeout_ms)
{
    struct pollfd pfd = {.fd = sock->fd, .events = POLLIN};

    int ready = poll(&pfd, 1, (int)timeout_ms);
    if (ready == 0)
        return 0;
    if (ready < 0)
        return -1;   /* EINTR included: the caller checks for a stop request */

    ssize_t n = recv(sock->fd, buf, cap, 0);
    return n < 0 ? -1 : (int)n;
}

int raw_socket_recv_any(raw_socket_t *socks, size_t count, uint8_t *buf, size_t cap,
                        unsigned timeout_ms, size_t *which)
{
    struct pollfd pfd[8];

    if (count == 0 || count > sizeof pfd / sizeof pfd[0])
        return -1;
    for (size_t i = 0; i < count; i++) {
        pfd[i].fd      = socks[i].fd;
        pfd[i].events  = POLLIN;
        pfd[i].revents = 0;
    }

    int ready = poll(pfd, (nfds_t)count, (int)timeout_ms);
    if (ready == 0)
        return 0;
    if (ready < 0)
        return -1;

    for (size_t i = 0; i < count; i++)
        if (pfd[i].revents & POLLIN) {
            ssize_t n = recv(socks[i].fd, buf, cap, 0);
            if (n < 0)
                return -1;
            if (which)
                *which = i;
            return (int)n;
        }
    return 0;
}

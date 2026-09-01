/**
 * @file RawSocket.h
 * @brief AF_PACKET access to a copper link.
 *
 * Configuration frames carry their own Ethernet header and a sequence byte
 * outside the IP length, so they go out verbatim - no kernel stack in the way.
 * The interface needs no IP address, only to be up.
 */

#ifndef RAW_SOCKET_H
#define RAW_SOCKET_H

#include <net/if.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    int  fd;
    int  ifindex;
    char name[IFNAMSIZ];
} raw_socket_t;

/** True if the interface exists and is administratively up with a carrier. */
bool raw_socket_link_up(const char *iface, bool *carrier);

/**
 * @brief Bind a raw socket to one interface.
 * @param promiscuous also accept frames not addressed to us - the health
 *        monitor is multicast to 03:00:00:00:xx:xx, so this is needed to see it
 */
bool raw_socket_open(raw_socket_t *sock, const char *iface, bool promiscuous);

void raw_socket_close(raw_socket_t *sock);

/** Put one complete Ethernet frame on the wire. */
bool raw_socket_send(raw_socket_t *sock, const uint8_t *frame, size_t len);

/**
 * @brief Wait for one frame.
 * @return bytes received, 0 on timeout, -1 on error (including an interrupt)
 */
int raw_socket_recv(raw_socket_t *sock, uint8_t *buf, size_t cap, unsigned timeout_ms);

/**
 * @brief Wait for a frame on any of several links.
 * @param which set to the index of the link the frame came from
 * @return bytes received, 0 on timeout, -1 on error (including an interrupt)
 */
int raw_socket_recv_any(raw_socket_t *socks, size_t count, uint8_t *buf, size_t cap,
                        unsigned timeout_ms, size_t *which);

#endif /* RAW_SOCKET_H */

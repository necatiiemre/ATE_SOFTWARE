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
    int     fd;
    int     ifindex;
    char    name[IFNAMSIZ];
    uint8_t mac[6];      /**< the interface's own address, for frames we source */
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

/**
 * @brief Grow the kernel's socket buffers.
 *
 * The configuration and health-monitor paths do not need this - they handle a
 * frame every few milliseconds. The CMC data plane sends and receives tens of
 * thousands of frames a second, and the default buffers are a few hundred
 * kilobytes, so a scheduling hiccup on the reader becomes a drop that looks
 * exactly like a lost packet on the unit's side. Best effort: a kernel that
 * refuses the size is not an error, and the actual sizes come back.
 */
void raw_socket_set_buffers(raw_socket_t *sock, int rcv_bytes, int snd_bytes,
                            int *rcv_got, int *snd_got);

/**
 * @brief Send without going through the interface's queueing discipline.
 *
 * One less place for a frame to be reordered or delayed on the way out, which
 * matters when the test measures the unit by what comes back in what order.
 * Best effort; false if the kernel does not support it.
 */
bool raw_socket_bypass_qdisc(raw_socket_t *sock);

/** Put one complete Ethernet frame on the wire. */
bool raw_socket_send(raw_socket_t *sock, const uint8_t *frame, size_t len);

/**
 * @brief Wait for one frame.
 * @return bytes received, 0 on timeout, -1 on error (including an interrupt)
 */
int raw_socket_recv(raw_socket_t *sock, uint8_t *buf, size_t cap, unsigned timeout_ms);

/**
 * @brief Receive from whichever of @p socks has something, in turn.
 *
 * Each call resumes the scan one past the socket it served last, so two links
 * carrying traffic at the same rate are read evenly rather than the first one
 * starving the rest.
 *
 * @param which set to the index the frame came from
 * @return bytes received, 0 on timeout, -1 on error
 */
int raw_socket_recv_any(raw_socket_t *socks, size_t count, uint8_t *buf, size_t cap,
                        unsigned timeout_ms, size_t *which);

#endif /* RAW_SOCKET_H */

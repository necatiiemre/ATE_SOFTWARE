/**
 * @file VmcPbitRequest.h
 * @brief Asking the VMC for its PBIT result.
 *
 * PBIT is a power-on test: the VMC runs it once at startup and then holds the
 * result until someone asks. Nothing else here sends anything - every other
 * report the VMC pushes on its own - so this is the one place the test talks
 * back, and without it the two PBIT slots stay empty for the whole run.
 *
 * The request is Test_Starters/vmc/src/main.c's send_pbit_request: an 11-byte
 * cmsw header with message identifier 50, a length of 11, a zero timestamp and
 * the sequence number in the last byte, wrapped in Ethernet, an optional 802.1Q
 * tag, IPv4 and UDP, padded to the 64-byte Ethernet minimum. It goes out on the
 * PBIT request VL for that side and the answer comes back on that side's PBIT
 * response VL, where VmcHealth picks it up like any other report.
 *
 * Both the request VLs and the VLANs are in AppConfig.h, because that starter
 * runs through the Mellanox switch and tags its requests while this test is
 * cabled straight to the VMC and may not need to.
 */

#ifndef VMC_PBIT_REQUEST_H
#define VMC_PBIT_REQUEST_H

#include "AppConfig.h"
#include "RawSocket.h"

#include <stdbool.h>
#include <stdint.h>

/* 11-byte payload, and the frame padded up to the Ethernet minimum. */
#define VMC_PBIT_REQ_PAYLOAD_LEN 11
#define VMC_PBIT_REQ_FRAME_LEN   64

/**
 * @brief Build one PBIT result request.
 *
 * @param out   at least VMC_PBIT_REQ_FRAME_LEN bytes
 * @param vlan  802.1Q id, or -1 to leave the frame untagged
 * @param seq   request counter; the byte on the wire is 0 once, then 1..255
 * @return frame length
 */
size_t vmc_pbit_request_build(uint8_t *out, const uint8_t src_mac[6], int vlan,
                              uint16_t vl_id, uint8_t msg_id, uint64_t seq);

/** The sequence byte for a given request count: 0, then 1..255 cycling. */
uint8_t vmc_pbit_request_seq(uint64_t seq);

/**
 * @brief Send a PBIT result request for one side and log what went out.
 * @return true when the frame reached the wire
 */
bool vmc_pbit_request_send(raw_socket_t *link, const vmc_config_t *config,
                           uint8_t side, uint64_t seq);

#endif /* VMC_PBIT_REQUEST_H */

#ifndef VMC_TEST_PACKET_DEFS_H_
#define VMC_TEST_PACKET_DEFS_H_

/*
 * Packet format - identical headers to dpdk_vmc / dpdk_cmc:
 *
 *   ETH 14 | 802.1Q 4 | IPv4 20 | UDP 8 | DATA N | SEQ 1
 *
 *   DST MAC : 03:00:00:00:VV:VV   (last 2 bytes = VL-IDX)
 *   SRC MAC : 02:00:00:00:00:TT   (TT = network: 0x20 Net A, 0x40 Net B)
 *   VLAN    : TPID 0x8100, priority 0, VID = VLAN ID
 *   SRC IP  : 10.0.0.0
 *   DST IP  : 224.224.VV.VV       (last 2 bytes = VL-IDX)
 *   IPv4    : TTL 1, TOS 0, ID 0, no fragmentation, header checksum
 *   UDP     : src/dst port 100, checksum 0 (disabled, same as DPDK)
 *
 * UDP payload = N data bytes + 1 sequence byte (the (N+1)-th byte), the same
 * "payload + trailing seq" layout dpdk_cmc uses for MMMS / DTN_SEQ.
 * Every data byte carries the packet's sequence value (no PRBS).
 *
 * Sequence (per network): 0, 1, 2, ... 255, 1, 2, ... (0 only once, first).
 */

#define ETH_HDR_LEN       14
#define VLAN_HDR_LEN      4
#define IPV4_HDR_LEN      20
#define UDP_HDR_LEN       8
#define SEQ_LEN           1
#define HEADERS_LEN       (ETH_HDR_LEN + VLAN_HDR_LEN + IPV4_HDR_LEN + UDP_HDR_LEN)  /* 46 */

#define ETHERTYPE_VLAN    0x8100
#define ETHERTYPE_IPV4    0x0800

#define SRC_MAC_PREFIX    {0x02, 0x00, 0x00, 0x00, 0x00}   /* + network tail */
#define DST_MAC_PREFIX    {0x03, 0x00, 0x00, 0x00}         /* + VL-IDX (2 B) */
#define NET_A_SRC_MAC_TAIL 0x20
#define NET_B_SRC_MAC_TAIL 0x40

#define SRC_IP            0x0A000000u   /* 10.0.0.0 */
#define DST_IP_PREFIX     0xE0E00000u   /* 224.224.0.0 | VL-IDX */
#define PKT_TTL           1
#define PKT_TOS           0
#define UDP_SRC_PORT      100
#define UDP_DST_PORT      100

/* Data length limits. 1466 = largest DPDK payload (1466 data + 1 seq = 1467),
 * fits a 1500-byte MTU. */
#define MIN_DATA_LEN      1
#define MAX_DATA_LEN      1466
#define MAX_FRAME_LEN     (HEADERS_LEN + MAX_DATA_LEN + SEQ_LEN)              /* 1513 */

#define MAX_VLAN_ID       4094

#endif /* VMC_TEST_PACKET_DEFS_H_ */

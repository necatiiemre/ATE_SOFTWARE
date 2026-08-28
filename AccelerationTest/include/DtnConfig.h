/**
 * @file DtnConfig.h
 * @brief Wire format of the DTN (LRU 0x2600) configuration protocol.
 *
 * Every management frame the DTN accepts looks like this:
 *
 *   Ethernet  dst 03:00:00:00:<VL hi>:<VL lo>   src 02:00:00:00:00:<net>
 *             ethertype 0x0800, or 0x8100 + TCI + 0x0800 when tagged
 *   IPv4      src 10.1.33.1  dst 224.224.<VL hi>.<VL lo>  TTL 1  proto UDP
 *   UDP       100 -> 100, checksum disabled
 *   Payload   LRU_ID(2) OpType(1) | Addr(1) Len(2) Data(Len) | M | Addr Len Data | ...
 *   Trailer   one AFDX sequence byte, OUTSIDE the IP total_length
 *
 * OpType is DTN_OP_WRITE to push configuration and DTN_OP_READ to poll device
 * and port status. `M` is a one-byte end-of-block marker, see dtn_block_marker.
 *
 * The format is not documented anywhere; it was recovered from the reference
 * blobs in RemoteConfigSender/main.cpp, the status query in
 * dpdk/src/HealthMonitor/HealthMonitor.c and the framing in
 * FirmwareUpdater/fpga_firmware_loader.py. tests/ pins it down by rebuilding
 * all 47 reference frames byte for byte.
 */

#ifndef DTN_CONFIG_H
#define DTN_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define DTN_LRU_ID              0x2600  /* device id of the DTN */
#define DTN_OP_WRITE            0x57    /* push configuration */
#define DTN_OP_READ             0x52    /* poll device / port status */

#define DTN_NET_A               0x20    /* low byte of the source MAC */
#define DTN_NET_B               0x40

/* Config-space start addresses (the "CfgType" byte). */
#define DTN_ADDR_ES_GLOBAL      0x10    /* end-system globals: VL count, Lmax */
#define DTN_ADDR_ES_PARAMS      0x17    /* end-system parameters */
#define DTN_ADDR_PTP            0x46    /* PTP session table */
#define DTN_ADDR_SW_BEGIN       0x70    /* switch table: begin */
#define DTN_ADDR_SW_END         0x71    /* switch table: end */
#define DTN_ADDR_VL_TABLE       0x72    /* switch table: VL records */
#define DTN_ADDR_PORT_TABLE     0x73    /* per-port table */
#define DTN_ADDR_SW_MISC        0x74    /* switch misc */

#define DTN_PORT_COUNT          35      /* ports 0..34; 34 is the CPU port */
#define DTN_VL_RECORD_LEN       14
#define DTN_MAX_FRAME           1518
#define DTN_MAX_BLOCKS          8

/* Largest VL record count that keeps an untagged frame within DTN_MAX_FRAME:
 * 14 eth + 20 ip + 8 udp + 3 header + 3 block + 14*N + 1 seq <= 1518. */
#define DTN_MAX_RECORDS_PER_BLOCK 104

/* Flag nibble of the LMAX field. Every reference record carries
 * DTN_VL_FLAG_ENABLE | DTN_VL_FLAG_RESERVED; the health-monitor response VL
 * additionally carries DTN_VL_FLAG_PRIORITY. */
#define DTN_VL_FLAG_RESERVED    0x1
#define DTN_VL_FLAG_PRIORITY    0x4     /* PRIORITY="HIGH" (inferred) */
#define DTN_VL_FLAG_ENABLE      0x8

/* BAG word. Only BAG="1MS" appears in the reference configuration, so no other
 * value has a known encoding. */
#define DTN_BAG_WORD_1MS        0x0602

/**
 * @brief One row of the switch VL table - the binary form of an XML <VL .../>.
 *
 * Field layout, matched one-to-one against the vendor XML attributes:
 *   [0]  2  ID          [8]   1  DESTPORT bits 34..32
 *   [2]  2  BAG word    [9]   1  SRCPORT
 *   [4]  1  JITTER (ms) [10]  4  DESTPORT bits 31..0
 *   [5]  1  LMIN
 *   [6]  2  flags(4) | LMAX(12)
 */
typedef struct {
    uint16_t vl_id;
    uint8_t  src_port;
    uint64_t dest_mask;   /**< bit N set = egress on DTN port N (N < 35) */
    uint16_t lmax;        /**< 12 bits */
    uint8_t  lmin;
    uint8_t  jitter_ms;
    uint16_t bag_word;
    uint8_t  flags;       /**< 4 bits */
} dtn_vl_t;

/** One Addr/Len/Data block inside a configuration datagram. */
typedef struct {
    uint8_t         addr;
    const uint8_t  *data;
    uint16_t        len;
} dtn_block_t;

/** A frame ready to hand to sendto() on a raw socket. */
typedef struct {
    uint8_t     seq;
    uint16_t    len;
    const char *label;
    uint8_t     data[DTN_MAX_FRAME];
} dtn_frame_t;

/** Fill in a VL record with the defaults every reference record uses. */
void dtn_vl_init(dtn_vl_t *vl, uint16_t vl_id, uint8_t src_port, uint64_t dest_mask);

/** Encode one VL record. Returns 0, or -1 if a field is out of range. */
int dtn_vl_encode(const dtn_vl_t *vl, uint8_t out[DTN_VL_RECORD_LEN]);

/** Decode one VL record. */
void dtn_vl_decode(const uint8_t in[DTN_VL_RECORD_LEN], dtn_vl_t *vl);

/**
 * @brief End-of-block marker that follows a block's data.
 *
 * Derived empirically; reproduces every marker in the reference blobs:
 *   0x70..0x7F -> (addr & 0x0F) | 0x80    (0x70->0x80, 0x72->0x82, ...)
 *   otherwise  -> addr + 2                (0x10->0x12, 0x17->0x19, 0x46->0x48)
 */
uint8_t dtn_block_marker(uint8_t addr);

/**
 * @brief Serialise LRU/OpType and a chain of blocks into a UDP payload.
 * @param terminate append the last block's marker (last datagram of a group)
 * @return payload length, or -1 if it does not fit in @p cap
 */
int dtn_build_payload(const dtn_block_t *blocks, size_t count, uint8_t op,
                      bool terminate, uint8_t *out, size_t cap);

/**
 * @brief Wrap a payload in Ethernet/IPv4/UDP and append the sequence byte.
 * @param vlan 802.1Q tag, or -1 for untagged (the copper path)
 * @return frame length, or -1 if it would exceed DTN_MAX_FRAME
 */
int dtn_build_frame(const uint8_t *payload, size_t len, uint8_t seq,
                    uint16_t vl_id, int vlan, uint8_t net, uint8_t *out, size_t cap);

/** The device's 1-byte AFDX counter: 0 is only the first value, then 1..255. */
uint8_t dtn_next_seq(uint8_t seq);

/** Per-port table payload (address 0x73). Writes 4 * @p port_count bytes. */
size_t dtn_encode_port_table(uint8_t *out, size_t cap, uint16_t value, uint8_t port_count);

#endif /* DTN_CONFIG_H */

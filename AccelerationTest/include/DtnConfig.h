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
#define DTN_ADDR_ES_GLOBAL      0x10    /* end-system globals: HM VL, Lmax */
#define DTN_ADDR_ES_PARAMS      0x17    /* end-system parameters */
#define DTN_ADDR_PTP            0x46    /* PTP session table */
#define DTN_ADDR_SW_BEGIN       0x70    /* switch table: begin */
#define DTN_ADDR_SW_END         0x71    /* switch table: end */
#define DTN_ADDR_VL_TABLE       0x72    /* switch table: VL records */
#define DTN_ADDR_PORT_TABLE     0x73    /* per-port table */
#define DTN_ADDR_SW_MISC        0x74    /* switch misc */

#define DTN_PORT_COUNT          35      /* ports 0..34 */
#define DTN_PORT_MANAGEMENT     34      /* internal, not a physical port */

/**
 * The VL the DTN puts its own health monitor and its 0x52 replies on. It is
 * bytes 2-3 of the end-system block, so the block and the switch record that
 * carries the VL out to copper are both built from this one constant - they
 * cannot drift apart.
 *
 * RemoteConfigSender writes 0x1188 here, and the main ATE software has that
 * number compiled in (HEALTH_MONITOR_RESPONSE_VL_IDX, and the receive filter at
 * dpdk/src/HealthMonitor/HealthMonitor.c:660 drops everything else). The
 * end-system block we were given writes 0x0026, so the device answers on VL 38
 * instead - which is why a DTN configured from here goes quiet as far as the
 * main software is concerned. Our receiver does not filter by VL, so it sees
 * either.
 */
#define DTN_HEALTH_MONITOR_VL   0x0026
#define DTN_HEALTH_MONITOR_PORT 33      /* out of the 100M copper end-system port */
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

/**
 * @brief A record for a VL the configuration does not use.
 *
 * The vendor XML has an ENABLE attribute for exactly this. The device does not
 * need the holes spelled out - the captured configuration writes 122 records
 * whose ids are neither contiguous nor sorted, which only works if the device
 * reads the id out of each record - but a disabled record is still the way to
 * retire a VL a previous configuration left behind.
 */
void dtn_vl_init_disabled(dtn_vl_t *vl, uint16_t vl_id);

/** True when the record's ENABLE flag is set. */
static inline bool dtn_vl_enabled(const dtn_vl_t *vl)
{
    return (vl->flags & DTN_VL_FLAG_ENABLE) != 0;
}

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

/** Upper bound on the frames dtn_build_config_frames can produce. */
#define DTN_MAX_CONFIG_FRAMES 64

/**
 * @brief Which optional pieces a configuration carries.
 *
 * The switch table itself (0x70, 0x72, 0x74, 0x71) is always written; these are
 * the parts around it. The captured configuration numbers its first switch
 * datagram seq 2, so two datagrams precede it: the end-system blocks and the
 * 0x46 block, exactly as RemoteConfigSender sends them.
 */
typedef struct {
    bool end_system;      /**< blocks 0x10 and 0x17, as their own datagram */
    bool protocol_block;  /**< block 0x46, as its own datagram */
    bool port_table;      /**< block 0x73, inside the closing datagram */
    bool status_query;    /**< the trailing 0x52 read */
} dtn_config_opts_t;

/**
 * @brief The three-datagram configuration: end system, then the switch table.
 *
 * seq 0 is the end-system blocks, seq 1 and seq 2 the switch table, and a 0x52
 * status query closes it off. No 0x46 - its body enumerates VL 4420-4487, which
 * this configuration's table does not contain - and no port table, because the
 * capture's closing datagram goes straight from 0x72 to 0x74 to 0x71.
 */
extern const dtn_config_opts_t DTN_CONFIG_DEFAULT;

/**
 * @brief Turn a VL table into the frames that configure the DTN.
 *
 * Emits the optional end-system and 0x46 datagrams, then the VL table split
 * across as many datagrams as it needs, closing the last one with the switch
 * end markers, and optionally finishes with a 0x52 status query so the caller
 * can read back what the device made of it.
 *
 * @param protocol_block payload for address 0x46; ignored unless opts asks for it
 * @param vlan 802.1Q tag, or -1 for untagged (the copper path)
 * @param opts which optional pieces to include; NULL means DTN_CONFIG_DEFAULT
 * @return frame count, or -1 if the VL table does not fit
 */
int dtn_build_config_frames(const dtn_vl_t *vls, size_t count,
                            const uint8_t *protocol_block, size_t protocol_len,
                            int vlan, const dtn_config_opts_t *opts,
                            dtn_frame_t *frames, size_t max_frames);

#endif /* DTN_CONFIG_H */

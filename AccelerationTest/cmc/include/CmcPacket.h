/**
 * @file CmcPacket.h
 * @brief The CMC data-plane frame, and the PRBS-31 stream that fills it.
 *
 * One frame shape, the same one dpdk_cmc puts on the fibre:
 *
 *   Ethernet 14 B  [ 802.1Q 4 B ]  IPv4 20 B  UDP 8 B  payload 1467 B
 *
 * The payload is an 8-byte sequence number followed by 1459 bytes read out of
 * a PRBS-31 stream at an offset the sequence decides, so the receiver can
 * regenerate what it should have got from the sequence alone and needs to
 * remember nothing. The last payload byte is overwritten with a cyclic counter
 * (DTN_SEQ), so it deliberately differs from the PRBS byte at that offset.
 *
 * The frame is 1509 bytes on a direct cable and 1513 with a tag. The payload
 * is 1467 bytes either way - the reference builds it that length because its
 * frames are tagged, and the switch strips the tag before the CMC sees them,
 * so 1509 untagged is exactly the frame the unit handles today. Keeping the
 * payload length fixed is what makes the direct-cable frame byte-for-byte the
 * one the unit already answers.
 *
 * The identity of a flow is the VL id, carried twice: in the low two bytes of
 * the destination MAC (03:00:00:00:VV:VV) and of the destination IP
 * (224.224.VV.VV). Source MAC's last byte says which network it is - 0x20 for
 * A, 0x40 for B - and is the only field that differs between the twin frames
 * the two networks carry.
 */

#ifndef CMC_PACKET_H
#define CMC_PACKET_H

#include "Prbs31.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Sizes                                                              */
/* ------------------------------------------------------------------ */

#define CMC_ETH_HDR_LEN   14
#define CMC_VLAN_HDR_LEN   4
#define CMC_IP_HDR_LEN    20
#define CMC_UDP_HDR_LEN    8

#define CMC_SEQ_BYTES      8
#define CMC_PAYLOAD_SIZE  1467
#define CMC_NUM_PRBS_BYTES (CMC_PAYLOAD_SIZE - CMC_SEQ_BYTES)   /* 1459 */

#define CMC_L2_LEN(tagged)     ((tagged) ? (CMC_ETH_HDR_LEN + CMC_VLAN_HDR_LEN) \
                                         : CMC_ETH_HDR_LEN)
#define CMC_PAYLOAD_OFF(tagged) (CMC_L2_LEN(tagged) + CMC_IP_HDR_LEN + CMC_UDP_HDR_LEN)
#define CMC_FRAME_LEN(tagged)   (CMC_PAYLOAD_OFF(tagged) + CMC_PAYLOAD_SIZE)

#define CMC_FRAME_LEN_UNTAGGED CMC_FRAME_LEN(false)   /* 1509 */
#define CMC_FRAME_LEN_TAGGED   CMC_FRAME_LEN(true)    /* 1513 */
#define CMC_FRAME_LEN_MAX      CMC_FRAME_LEN_TAGGED

#define CMC_ETHER_TYPE_IPV4 0x0800
#define CMC_ETHER_TYPE_VLAN 0x8100

/* ------------------------------------------------------------------ */
/* PRBS-31                                                            */
/* ------------------------------------------------------------------ */

/* The stream itself is in common/Prbs31.h - it is the same stream the DTN's
 * copper legs use, so it lives with neither unit. What is CMC-specific is only
 * how much of it a packet takes: the stride. */
#define CMC_PRBS_INITIAL_STATE PRBS31_INITIAL_STATE
#define CMC_PRBS_CACHE_SIZE    PRBS31_CACHE_SIZE
#define CMC_PRBS_STRIDE        CMC_NUM_PRBS_BYTES

/** The cache, with the CMC's stride. */
bool cmc_prbs_cache_init(prbs31_cache_t *cache, uint32_t initial_state,
                         void (*progress)(size_t done, size_t total));

/* ------------------------------------------------------------------ */
/* The frame                                                          */
/* ------------------------------------------------------------------ */

typedef struct {
    uint8_t  src_mac[6];
    uint16_t vl_id;
    bool     vlan_tagged;
    uint16_t vlan_id;
    uint8_t  vlan_priority;
    uint32_t src_ip;        /**< host order; 10.0.0.0 in the reference */
    uint8_t  ttl;
    uint8_t  tos;
    uint16_t src_port;
    uint16_t dst_port;
} cmc_packet_config_t;

/** The reference's fixed fields: src MAC 02:00:00:00:00:20, 10.0.0.0, TTL 1, UDP 100->100. */
void cmc_packet_config_init(cmc_packet_config_t *cfg);

/**
 * @brief Write the headers and zero the payload.
 * @return the frame length
 *
 * Destination MAC and IP come from cfg->vl_id. The IP checksum is computed;
 * the UDP checksum is left zero, as the reference leaves it.
 */
size_t cmc_packet_build(uint8_t *frame, const cmc_packet_config_t *cfg);

/**
 * @brief Write the sequence number and its PRBS bytes into a built frame.
 *
 * The sequence goes in as a raw host-order 64-bit word, which is what the
 * reference writes and what its receiver reads back.
 */
void cmc_packet_fill_payload(uint8_t *frame, bool vlan_tagged,
                             const prbs31_cache_t *cache, uint64_t seq);

/**
 * @brief The trailing DTN_SEQ byte for a sequence.
 *
 * 0 for sequence 0, then 1..255 cycling - the value never returns to 0, so a
 * zero in that byte after the first packet means the first packet.
 */
uint8_t cmc_dtn_seq(uint64_t seq);

/** Stamp DTN_SEQ over the last payload byte. */
void cmc_packet_stamp_dtn_seq(uint8_t *frame, size_t frame_len, uint64_t seq);

/** The VL id out of a frame's destination MAC. */
uint16_t cmc_vl_id_of(const uint8_t *frame);

/** The one's-complement sum an IPv4 header needs, over 20 bytes. */
uint16_t cmc_ip_checksum(const uint8_t *ip_hdr);

#endif /* CMC_PACKET_H */

#include "CmcPacket.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* PRBS-31                                                            */
/* ------------------------------------------------------------------ */

/* The reference's generator, unchanged. It taps bits 0 and 3 of a 31-bit
 * state, shifts right and feeds the new bit in at bit 30. Written the obvious
 * way rather than the fast way: it runs once, for a quarter of a gigabyte, and
 * being recognisably the same loop as dpdk_cmc's matters more than the minute
 * it takes. */
static inline uint32_t prbs31_next(uint32_t *state)
{
    uint32_t output = *state & 0x01;
    uint32_t new_bit = ((*state & 0x01) ^ ((*state >> 3) & 0x01)) & 0x01;

    *state = (new_bit << 30 | (*state >> 1)) & 0x7FFFFFFF;
    return output;
}

uint32_t cmc_prbs_fill(uint8_t *out, size_t len, uint32_t state)
{
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = 0;

        for (int bit = 0; bit < 8; bit++)
            byte = (uint8_t)((byte << 1) | prbs31_next(&state));
        out[i] = byte;
    }
    return state;
}

bool cmc_prbs_cache_init(cmc_prbs_cache_t *cache, uint32_t initial_state,
                         void (*progress)(size_t done, size_t total))
{
    if (!cache)
        return false;

    memset(cache, 0, sizeof *cache);

    cache->bytes = malloc(CMC_PRBS_BUFFER_SIZE);
    if (!cache->bytes)
        return false;

    cache->initial_state = initial_state;

    /* Generated in 10 MB chunks so the progress callback has something to say;
     * the stream is the same as one call over the whole period, because the
     * state is carried across. */
    const size_t chunk = 10u * 1024u * 1024u;
    uint32_t state = initial_state;

    for (size_t done = 0; done < CMC_PRBS_CACHE_SIZE; done += chunk) {
        const size_t n = (CMC_PRBS_CACHE_SIZE - done < chunk)
                             ? CMC_PRBS_CACHE_SIZE - done : chunk;

        state = cmc_prbs_fill(cache->bytes + done, n, state);
        if (progress)
            progress(done + n, CMC_PRBS_CACHE_SIZE);
    }
    memcpy(cache->bytes + CMC_PRBS_CACHE_SIZE, cache->bytes, CMC_NUM_PRBS_BYTES);

    cache->ready = true;
    return true;
}

void cmc_prbs_cache_free(cmc_prbs_cache_t *cache)
{
    if (!cache)
        return;
    free(cache->bytes);
    cache->bytes = NULL;
    cache->ready = false;
}

const uint8_t *cmc_prbs_at(const cmc_prbs_cache_t *cache, uint64_t seq)
{
    if (!cache || !cache->ready)
        return NULL;

    const uint64_t off = (seq * (uint64_t)CMC_NUM_PRBS_BYTES) % (uint64_t)CMC_PRBS_CACHE_SIZE;

    return cache->bytes + off;
}

/* ------------------------------------------------------------------ */
/* The frame                                                          */
/* ------------------------------------------------------------------ */

static void put_be16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static void put_be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

uint16_t cmc_ip_checksum(const uint8_t *ip_hdr)
{
    uint32_t sum = 0;

    for (int i = 0; i < CMC_IP_HDR_LEN; i += 2)
        sum += (uint32_t)((ip_hdr[i] << 8) | ip_hdr[i + 1]);
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

void cmc_packet_config_init(cmc_packet_config_t *cfg)
{
    if (!cfg)
        return;

    memset(cfg, 0, sizeof *cfg);

    /* 02:00:00:00:00:20 - the tail is the network byte and the caller
     * overwrites it per network. */
    cfg->src_mac[0] = 0x02;
    cfg->src_mac[5] = 0x20;

    cfg->src_ip   = (10u << 24);          /* 10.0.0.0 */
    cfg->ttl      = 0x01;
    cfg->tos      = 0x00;
    cfg->src_port = 100;
    cfg->dst_port = 100;
}

size_t cmc_packet_build(uint8_t *frame, const cmc_packet_config_t *cfg)
{
    if (!frame || !cfg)
        return 0;

    const size_t len = CMC_FRAME_LEN(cfg->vlan_tagged);
    const size_t l2  = CMC_L2_LEN(cfg->vlan_tagged);

    memset(frame, 0, len);

    /* Ethernet. The VL id lives in the low two bytes of the destination MAC,
     * the same place the rest of this rig's AFDX traffic carries it. */
    frame[0] = 0x03;
    frame[4] = (uint8_t)(cfg->vl_id >> 8);
    frame[5] = (uint8_t)cfg->vl_id;
    memcpy(frame + 6, cfg->src_mac, 6);

    if (cfg->vlan_tagged) {
        put_be16(frame + 12, CMC_ETHER_TYPE_VLAN);
        put_be16(frame + 14, (uint16_t)(((cfg->vlan_priority & 0x07) << 13) |
                                        (cfg->vlan_id & 0x0FFF)));
        put_be16(frame + 16, CMC_ETHER_TYPE_IPV4);
    } else {
        put_be16(frame + 12, CMC_ETHER_TYPE_IPV4);
    }

    /* IPv4. The total length counts the payload the reference builds, which is
     * the tagged-mode length whether or not the tag is there - see the header. */
    uint8_t *ip = frame + l2;

    ip[0] = 0x45;                         /* version 4, 20-byte header */
    ip[1] = cfg->tos;
    put_be16(ip + 2, (uint16_t)(CMC_IP_HDR_LEN + CMC_UDP_HDR_LEN + CMC_PAYLOAD_SIZE));
    ip[8] = cfg->ttl;
    ip[9] = 17;                           /* UDP */
    put_be32(ip + 12, cfg->src_ip);
    put_be32(ip + 16, (uint32_t)((224u << 24) | (224u << 16) |
                                 ((uint32_t)((cfg->vl_id >> 8) & 0xFF) << 8) |
                                 (uint32_t)(cfg->vl_id & 0xFF)));
    put_be16(ip + 10, cmc_ip_checksum(ip));

    /* UDP. The checksum is left zero, as the reference leaves it. */
    uint8_t *udp = ip + CMC_IP_HDR_LEN;

    put_be16(udp + 0, cfg->src_port);
    put_be16(udp + 2, cfg->dst_port);
    put_be16(udp + 4, (uint16_t)(CMC_UDP_HDR_LEN + CMC_PAYLOAD_SIZE));

    return len;
}

void cmc_packet_fill_payload(uint8_t *frame, bool vlan_tagged,
                             const cmc_prbs_cache_t *cache, uint64_t seq)
{
    if (!frame || !cache || !cache->ready)
        return;

    uint8_t *payload = frame + CMC_PAYLOAD_OFF(vlan_tagged);

    /* A raw host-order 64-bit word, which is what the reference writes and
     * what its receiver reads straight back out. */
    memcpy(payload, &seq, sizeof seq);
    memcpy(payload + CMC_SEQ_BYTES, cmc_prbs_at(cache, seq), CMC_NUM_PRBS_BYTES);
}

uint8_t cmc_dtn_seq(uint64_t seq)
{
    if (seq == 0)
        return 0;
    return (uint8_t)(((seq - 1u) % 255u) + 1u);
}

void cmc_packet_stamp_dtn_seq(uint8_t *frame, size_t frame_len, uint64_t seq)
{
    if (!frame || frame_len == 0)
        return;
    frame[frame_len - 1] = cmc_dtn_seq(seq);
}

uint16_t cmc_vl_id_of(const uint8_t *frame)
{
    return (uint16_t)((frame[4] << 8) | frame[5]);
}

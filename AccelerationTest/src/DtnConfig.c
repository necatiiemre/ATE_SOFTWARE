#include "DtnConfig.h"

#include <string.h>

#define ETH_HDR_LEN   14
#define VLAN_TAG_LEN  4
#define IP_HDR_LEN    20
#define UDP_HDR_LEN   8
#define DTN_UDP_PORT  100

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

static uint16_t get_be16(const uint8_t *p)
{
    return (uint16_t)((p[0] << 8) | p[1]);
}

static uint32_t get_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static uint16_t ip_checksum(const uint8_t *header, size_t len)
{
    uint32_t sum = 0;
    for (size_t i = 0; i + 1 < len; i += 2)
        sum += (uint32_t)((header[i] << 8) | header[i + 1]);
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

void dtn_vl_init(dtn_vl_t *vl, uint16_t vl_id, uint8_t src_port, uint64_t dest_mask)
{
    vl->vl_id     = vl_id;
    vl->src_port  = src_port;
    vl->dest_mask = dest_mask;
    vl->lmax      = 1518;
    vl->lmin      = 64;
    vl->jitter_ms = 0;
    vl->bag_word  = DTN_BAG_WORD_1MS;
    vl->flags     = DTN_VL_FLAG_ENABLE | DTN_VL_FLAG_RESERVED;
}

void dtn_vl_init_disabled(dtn_vl_t *vl, uint16_t vl_id)
{
    dtn_vl_init(vl, vl_id, 0, 0);
    vl->flags &= (uint8_t)~DTN_VL_FLAG_ENABLE;
}

int dtn_vl_encode(const dtn_vl_t *vl, uint8_t out[DTN_VL_RECORD_LEN])
{
    if (vl->src_port >= DTN_PORT_COUNT)
        return -1;
    if (vl->lmax == 0 || vl->lmax > 0x0FFF)
        return -1;
    if (vl->dest_mask >> DTN_PORT_COUNT)
        return -1;

    put_be16(out + 0, vl->vl_id);
    put_be16(out + 2, vl->bag_word);
    out[4] = vl->jitter_ms;
    out[5] = vl->lmin;
    put_be16(out + 6, (uint16_t)(((vl->flags & 0x0F) << 12) | (vl->lmax & 0x0FFF)));
    out[8] = (uint8_t)((vl->dest_mask >> 32) & 0xFF);
    out[9] = vl->src_port;
    put_be32(out + 10, (uint32_t)(vl->dest_mask & 0xFFFFFFFFu));
    return 0;
}

void dtn_vl_decode(const uint8_t in[DTN_VL_RECORD_LEN], dtn_vl_t *vl)
{
    uint16_t flags_lmax = get_be16(in + 6);

    vl->vl_id     = get_be16(in + 0);
    vl->bag_word  = get_be16(in + 2);
    vl->jitter_ms = in[4];
    vl->lmin      = in[5];
    vl->flags     = (uint8_t)(flags_lmax >> 12);
    vl->lmax      = flags_lmax & 0x0FFF;
    vl->src_port  = in[9];
    vl->dest_mask = ((uint64_t)in[8] << 32) | get_be32(in + 10);
}

uint8_t dtn_block_marker(uint8_t addr)
{
    if (addr >= 0x70 && addr <= 0x7F)
        return (uint8_t)((addr & 0x0F) | 0x80);
    return (uint8_t)(addr + 2);
}

int dtn_build_payload(const dtn_block_t *blocks, size_t count, uint8_t op,
                      bool terminate, uint8_t *out, size_t cap)
{
    size_t n = 0;

    if (count == 0)
        return -1;

    /* LRU_ID(2) + OpType(1) */
    if (cap < 3)
        return -1;
    put_be16(out, DTN_LRU_ID);
    out[2] = op;
    n = 3;

    for (size_t i = 0; i < count; i++) {
        size_t need = (i ? 1u : 0u) + 3u + blocks[i].len;
        if (n + need > cap)
            return -1;
        if (i)
            out[n++] = dtn_block_marker(blocks[i - 1].addr);
        out[n++] = blocks[i].addr;
        put_be16(out + n, blocks[i].len);
        n += 2;
        memcpy(out + n, blocks[i].data, blocks[i].len);
        n += blocks[i].len;
    }

    if (terminate) {
        if (n + 1 > cap)
            return -1;
        out[n++] = dtn_block_marker(blocks[count - 1].addr);
    }
    return (int)n;
}

int dtn_build_frame(const uint8_t *payload, size_t len, uint8_t seq,
                    uint16_t vl_id, int vlan, uint8_t net, uint8_t *out, size_t cap)
{
    size_t tagged = (vlan >= 0) ? VLAN_TAG_LEN : 0;
    size_t total  = ETH_HDR_LEN + tagged + IP_HDR_LEN + UDP_HDR_LEN + len + 1;
    size_t n = 0;
    uint8_t *ip;

    if (total > DTN_MAX_FRAME || total > cap)
        return -1;

    /* Ethernet: the low 16 bits of the destination MAC carry the VL id. */
    out[n++] = 0x03; out[n++] = 0x00; out[n++] = 0x00; out[n++] = 0x00;
    put_be16(out + n, vl_id); n += 2;
    out[n++] = 0x02; out[n++] = 0x00; out[n++] = 0x00; out[n++] = 0x00;
    out[n++] = 0x00; out[n++] = net;
    if (vlan >= 0) {
        out[n++] = 0x81; out[n++] = 0x00;
        put_be16(out + n, (uint16_t)(vlan & 0x0FFF)); n += 2;
    }
    out[n++] = 0x08; out[n++] = 0x00;

    /* IPv4. The identification field is fixed at 0xd43b in every reference
     * frame; keep it so generated frames stay comparable with them. */
    ip = out + n;
    ip[0] = 0x45; ip[1] = 0x00;
    put_be16(ip + 2, (uint16_t)(IP_HDR_LEN + UDP_HDR_LEN + len));
    ip[4] = 0xd4; ip[5] = 0x3b;
    ip[6] = 0x00; ip[7] = 0x00;
    ip[8] = 0x01;               /* TTL */
    ip[9] = 0x11;               /* UDP */
    ip[10] = 0x00; ip[11] = 0x00;
    ip[12] = 10; ip[13] = 1; ip[14] = 33; ip[15] = 1;
    ip[16] = 0xe0; ip[17] = 0xe0;
    put_be16(ip + 18, vl_id);
    put_be16(ip + 10, ip_checksum(ip, IP_HDR_LEN));
    n += IP_HDR_LEN;

    /* UDP, checksum disabled. */
    put_be16(out + n, DTN_UDP_PORT); n += 2;
    put_be16(out + n, DTN_UDP_PORT); n += 2;
    put_be16(out + n, (uint16_t)(UDP_HDR_LEN + len)); n += 2;
    put_be16(out + n, 0); n += 2;

    memcpy(out + n, payload, len);
    n += len;

    /* AFDX sequence byte, deliberately outside the IP total_length. */
    out[n++] = seq;
    return (int)n;
}

uint8_t dtn_next_seq(uint8_t seq)
{
    return (uint8_t)(seq >= 255 ? 1 : seq + 1);
}

size_t dtn_encode_port_table(uint8_t *out, size_t cap, uint16_t value, uint8_t port_count)
{
    size_t need = (size_t)port_count * 4u;

    if (need > cap)
        return 0;
    for (uint8_t p = 0; p < port_count; p++) {
        put_be16(out + p * 4, p);
        put_be16(out + p * 4 + 2, value);
    }
    return need;
}

/* ------------------------------------------------------------------------- */
/* Fixed blocks, copied verbatim from the reference configuration that the
 * hardware is known to accept. Only the VL count inside ES_GLOBAL varies. */

static const uint8_t REF_ES_GLOBAL[] = {
    0x00, 0x01, 0x11, 0x88, 0x05, 0xee, 0x04, 0x09, 0xc7, 0x00};
static const uint8_t REF_ES_PARAMS[] = {
    0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t REF_SW_BEGIN[] = {0x00, 0x00, 0x00};
static const uint8_t REF_SW_END[]   = {0x00, 0x00, 0x00};
static const uint8_t REF_SW_MISC[]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x30};

/* The 0x52 status query, byte-identical to the one the main software polls
 * with every second (HealthMonitor.c). */
static const uint8_t STATUS_QUERY[] = {
    0x26, 0x00, 0x52, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/* Scratch for the largest VL table block. */
static uint8_t g_record_buf[DTN_MAX_RECORDS_PER_BLOCK * DTN_VL_RECORD_LEN];
static uint8_t g_port_table[DTN_PORT_COUNT * 4];

int dtn_build_config_frames(const dtn_vl_t *vls, size_t count, int vlan,
                            dtn_frame_t *frames, size_t max_frames)
{
    uint8_t payload[DTN_MAX_FRAME];
    uint8_t es_global[sizeof REF_ES_GLOBAL];
    dtn_block_t blocks[DTN_MAX_BLOCKS];
    size_t emitted = 0;
    uint8_t seq = 0;

    if (count == 0 || count > 0xFFFF)
        return -1;

    /* Emit one frame; the sequence byte advances with each. */
    #define EMIT(nblocks, terminate, text)                                          \
        do {                                                                        \
            if (emitted == max_frames)                                              \
                return -1;                                                          \
            int _p = dtn_build_payload(blocks, (nblocks), DTN_OP_WRITE, (terminate), \
                                       payload, sizeof payload);                    \
            if (_p < 0)                                                             \
                return -1;                                                          \
            int _f = dtn_build_frame(payload, (size_t)_p, seq, 0, vlan, DTN_NET_A,  \
                                     frames[emitted].data, DTN_MAX_FRAME);          \
            if (_f < 0)                                                             \
                return -1;                                                          \
            frames[emitted].seq = seq;                                              \
            frames[emitted].len = (uint16_t)_f;                                     \
            frames[emitted].label = (text);                                         \
            emitted++;                                                              \
            seq = emitted == 1 ? 1 : dtn_next_seq(seq);                             \
        } while (0)

    memcpy(es_global, REF_ES_GLOBAL, sizeof REF_ES_GLOBAL);
    es_global[2] = (uint8_t)(count >> 8);
    es_global[3] = (uint8_t)count;

    blocks[0] = (dtn_block_t){DTN_ADDR_ES_GLOBAL, es_global, sizeof es_global};
    blocks[1] = (dtn_block_t){DTN_ADDR_ES_PARAMS, REF_ES_PARAMS, sizeof REF_ES_PARAMS};
    EMIT(2, true, "end system");

    size_t chunks = (count + DTN_MAX_RECORDS_PER_BLOCK - 1) / DTN_MAX_RECORDS_PER_BLOCK;
    for (size_t c = 0; c < chunks; c++) {
        size_t first = c * DTN_MAX_RECORDS_PER_BLOCK;
        size_t take  = count - first;
        size_t nb    = 0;
        bool   last  = (c == chunks - 1);

        if (take > DTN_MAX_RECORDS_PER_BLOCK)
            take = DTN_MAX_RECORDS_PER_BLOCK;
        for (size_t i = 0; i < take; i++)
            if (dtn_vl_encode(&vls[first + i], g_record_buf + i * DTN_VL_RECORD_LEN) < 0)
                return -1;

        if (c == 0)
            blocks[nb++] = (dtn_block_t){DTN_ADDR_SW_BEGIN, REF_SW_BEGIN, sizeof REF_SW_BEGIN};
        blocks[nb++] = (dtn_block_t){DTN_ADDR_VL_TABLE, g_record_buf,
                                     (uint16_t)(take * DTN_VL_RECORD_LEN)};
        if (last) {
            size_t pt = dtn_encode_port_table(g_port_table, sizeof g_port_table,
                                              0x0112, DTN_PORT_COUNT - 1);
            if (pt == 0)
                return -1;
            blocks[nb++] = (dtn_block_t){DTN_ADDR_PORT_TABLE, g_port_table, (uint16_t)pt};
            blocks[nb++] = (dtn_block_t){DTN_ADDR_SW_MISC, REF_SW_MISC, sizeof REF_SW_MISC};
            blocks[nb++] = (dtn_block_t){DTN_ADDR_SW_END, REF_SW_END, sizeof REF_SW_END};
        }
        EMIT(nb, last, "vl table");
    }
    #undef EMIT

    if (emitted == max_frames)
        return -1;
    int flen = dtn_build_frame(STATUS_QUERY, sizeof STATUS_QUERY, seq, 0, -1,
                               DTN_NET_A, frames[emitted].data, DTN_MAX_FRAME);
    if (flen < 0)
        return -1;
    frames[emitted].seq = seq;
    frames[emitted].len = (uint16_t)flen;
    frames[emitted].label = "status query";
    emitted++;

    return (int)emitted;
}

#include "VlFrame.h"

#include <string.h>

#define ETH_LEN   14
#define VLAN_LEN   4
#define IP_LEN    20
#define UDP_LEN    8
#define HEADERS   (ETH_LEN + VLAN_LEN + IP_LEN + UDP_LEN)
#define UDP_PORT 100

static void put_be16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static void put_be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);  p[3] = (uint8_t)v;
}
static uint16_t get_be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }
static uint32_t get_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static uint16_t ip_checksum(const uint8_t *header)
{
    uint32_t sum = 0;

    for (int i = 0; i < IP_LEN; i += 2)
        sum += (uint32_t)((header[i] << 8) | header[i + 1]);
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

int vl_frame_build(uint8_t *out, size_t cap, uint16_t vl_id, uint16_t vlan,
                   uint8_t src_dtn_port, uint32_t sequence)
{
    if (cap < VL_FRAME_SIZE || VL_FRAME_SIZE < HEADERS + 12)
        return -1;

    memset(out, 0, VL_FRAME_SIZE);

    /* Ethernet: the VL id occupies the low two bytes of the destination MAC. */
    out[0] = 0x03; out[1] = 0x00; out[2] = 0x00; out[3] = 0x00;
    put_be16(out + 4, vl_id);
    out[6] = 0x02; out[7] = 0x00; out[8] = 0x00; out[9] = 0x00;
    out[10] = 0x00; out[11] = 0x20;               /* network A */
    put_be16(out + 12, 0x8100);
    put_be16(out + 14, (uint16_t)(vlan & 0x0FFF));
    put_be16(out + 16, 0x0800);

    uint8_t *ip = out + ETH_LEN + VLAN_LEN;
    ip[0] = 0x45;
    put_be16(ip + 2, (uint16_t)(VL_FRAME_SIZE - ETH_LEN - VLAN_LEN));
    ip[8] = 1;      /* TTL */
    ip[9] = 17;     /* UDP */
    ip[12] = 10;    /* 10.0.0.0 */
    ip[16] = 224; ip[17] = 224;
    put_be16(ip + 18, vl_id);
    put_be16(ip + 10, ip_checksum(ip));

    uint8_t *udp = ip + IP_LEN;
    put_be16(udp + 0, UDP_PORT);
    put_be16(udp + 2, UDP_PORT);
    put_be16(udp + 4, (uint16_t)(VL_FRAME_SIZE - ETH_LEN - VLAN_LEN - IP_LEN));

    uint8_t *payload = udp + UDP_LEN;
    put_be32(payload + 0, VL_FRAME_MAGIC);
    put_be16(payload + 4, vl_id);
    payload[6] = src_dtn_port;
    put_be32(payload + 8, sequence);

    return VL_FRAME_SIZE;
}

bool vl_frame_parse(const uint8_t *frame, size_t len, vl_probe_t *probe)
{
    size_t offset = ETH_LEN;
    uint16_t vlan = 0;

    memset(probe, 0, sizeof *probe);
    if (len < HEADERS + 12)
        return false;

    if (get_be16(frame + 12) == 0x8100) {
        vlan   = get_be16(frame + 14) & 0x0FFF;
        offset = ETH_LEN + VLAN_LEN;
    } else if (get_be16(frame + 12) != 0x0800) {
        return false;
    }

    const uint8_t *ip = frame + offset;
    if ((ip[0] >> 4) != 4 || ip[9] != 17)
        return false;

    const uint8_t *udp = ip + IP_LEN;
    if (get_be16(udp) != UDP_PORT || get_be16(udp + 2) != UDP_PORT)
        return false;

    const uint8_t *payload = udp + UDP_LEN;
    if (get_be32(payload) != VL_FRAME_MAGIC)
        return false;

    probe->vl_id        = get_be16(payload + 4);
    probe->src_dtn_port = payload[6];
    probe->sequence     = get_be32(payload + 8);
    probe->vlan         = vlan;
    return true;
}

#include "VmcPbitRequest.h"

#include "Log.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#define VMC_UDP_PORT 100

/* Reference DTN sequence pattern: 0, then 1..255 cycling. */
uint8_t vmc_pbit_request_seq(uint64_t seq)
{
    if (seq == 0)
        return 0;
    return (uint8_t)(((seq - 1) % 255) + 1);
}

size_t vmc_pbit_request_build(uint8_t *out, const uint8_t src_mac[6],
                              uint16_t vl_id, uint8_t msg_id, uint64_t seq)
{
    const uint16_t udp_len = 8 + VMC_PBIT_REQ_PAYLOAD_LEN;
    const uint16_t ip_len  = 20 + udp_len;
    size_t n = 0;

    memset(out, 0, VMC_PBIT_REQ_FRAME_LEN);

    /* Ethernet: the VL id in the low two bytes of the destination MAC. */
    out[n++] = 0x03; out[n++] = 0x00; out[n++] = 0x00; out[n++] = 0x00;
    out[n++] = (uint8_t)(vl_id >> 8);
    out[n++] = (uint8_t)(vl_id & 0xFF);
    memcpy(out + n, src_mac, 6);
    n += 6;
    out[n++] = 0x08; out[n++] = 0x00;

    uint8_t *ip = out + n;
    ip[0] = 0x45;                       /* version 4, IHL 5 */
    ip[1] = 0x00;                       /* TOS */
    ip[2] = (uint8_t)(ip_len >> 8);
    ip[3] = (uint8_t)(ip_len & 0xFF);
    ip[4] = 0; ip[5] = 0;               /* id */
    ip[6] = 0; ip[7] = 0;               /* flags/frag */
    ip[8] = 1;                          /* TTL */
    ip[9] = 17;                         /* UDP */
    ip[10] = 0; ip[11] = 0;             /* checksum, filled below */
    ip[12] = 10; ip[13] = 0; ip[14] = 0; ip[15] = 0;     /* src 10.0.0.0 */
    ip[16] = 224; ip[17] = 224;                          /* dst 224.224.x.y */
    ip[18] = (uint8_t)(vl_id >> 8);
    ip[19] = (uint8_t)(vl_id & 0xFF);
    {
        uint32_t sum = 0;
        for (int i = 0; i < 20; i += 2)
            sum += ((uint32_t)ip[i] << 8) | ip[i + 1];
        while (sum >> 16)
            sum = (sum & 0xFFFF) + (sum >> 16);
        uint16_t cksum = (uint16_t)~sum;
        ip[10] = (uint8_t)(cksum >> 8);
        ip[11] = (uint8_t)(cksum & 0xFF);
    }
    n += 20;

    uint8_t *udp = out + n;
    udp[0] = 0; udp[1] = VMC_UDP_PORT;
    udp[2] = 0; udp[3] = VMC_UDP_PORT;
    udp[4] = (uint8_t)(udp_len >> 8);
    udp[5] = (uint8_t)(udp_len & 0xFF);
    udp[6] = 0; udp[7] = 0;             /* checksum, optional over IPv4 */
    n += 8;

    /* The 11-byte cmsw header: identifier, a length of 11, a zero timestamp -
     * and the sequence number in the last byte, over the timestamp's last. */
    uint8_t *payload = out + n;
    payload[0] = msg_id;
    payload[1] = 0;
    payload[2] = VMC_PBIT_REQ_PAYLOAD_LEN;
    payload[VMC_PBIT_REQ_PAYLOAD_LEN - 1] = vmc_pbit_request_seq(seq);

    /* Padded to the Ethernet minimum, as the reference sends it. */
    return VMC_PBIT_REQ_FRAME_LEN;
}

bool vmc_pbit_request_send(raw_socket_t *link, const vmc_config_t *config,
                           uint8_t side, uint64_t seq)
{
    uint8_t frame[VMC_PBIT_REQ_FRAME_LEN];
    bool vs = side != 0;
    uint16_t vl_id = vs ? config->vs_pbit_request : config->flcs_pbit_request;

    size_t len = vmc_pbit_request_build(frame, link->mac, vl_id,
                                        config->msg_pbit_request, seq);

    if (!raw_socket_send(link, frame, len)) {
        log_line("PBIT request for %s on %s failed to send", vs ? "VS" : "FLCS",
                 link->name);
        return false;
    }

    log_line("PBIT request %llu for %s: VL %u, message id %u, sequence byte %u, "
             "%zu bytes on %s", (unsigned long long)seq, vs ? "VS" : "FLCS", vl_id,
             config->msg_pbit_request, vmc_pbit_request_seq(seq), len, link->name);
    return true;
}

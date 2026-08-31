#include "HealthMonitor.h"

#include <string.h>
#include <time.h>

uint64_t hm_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)(ts.tv_nsec / 1000000L);
}

bool hm_classify(const uint8_t *frame, size_t len, hm_frame_t *out)
{
    memset(out, 0, sizeof *out);

    if (len < HM_UDP_PAYLOAD_OFFSET + 7)
        return false;
    if (frame[12] != 0x08 || frame[13] != 0x00)   /* untagged IPv4 only */
        return false;

    const uint8_t *ip = frame + 14;
    if ((ip[0] >> 4) != 4 || ip[9] != 17)         /* IPv4, UDP */
        return false;
    if (ip[16] != 224 || ip[17] != 224)           /* 224.224.x.x */
        return false;

    const uint8_t *udp = ip + 20;
    uint16_t sport = (uint16_t)((udp[0] << 8) | udp[1]);
    uint16_t dport = (uint16_t)((udp[2] << 8) | udp[3]);
    if (sport != 100 || dport != 100)
        return false;

    uint16_t udp_len = (uint16_t)((udp[4] << 8) | udp[5]);
    if (udp_len < 8)
        return false;

    size_t payload_len = (size_t)udp_len - 8;
    if (HM_UDP_PAYLOAD_OFFSET + payload_len > len)
        payload_len = len - HM_UDP_PAYLOAD_OFFSET;

    out->is_device_frame = true;
    out->vl_id         = (uint16_t)((frame[4] << 8) | frame[5]);
    out->payload       = frame + HM_UDP_PAYLOAD_OFFSET;
    out->payload_len   = payload_len;
    out->operation_type = out->payload[HM_OFF_OPERATION_TYPE];
    out->status_enable  = payload_len > HM_OFF_STATUS_ENABLE
                        ? out->payload[HM_OFF_STATUS_ENABLE] : 0;
    return true;
}

void hm_watch_init(hm_watch_t *watch)
{
    memset(watch, 0, sizeof *watch);
    watch->last_seen_ms = hm_now_ms();
}

void hm_watch_saw_frame(hm_watch_t *watch)
{
    watch->frames++;
    watch->last_seen_ms = hm_now_ms();
}

bool hm_watch_update(hm_watch_t *watch, unsigned timeout_ms)
{
    bool alive = (hm_now_ms() - watch->last_seen_ms) < timeout_ms;

    if (alive == watch->alive)
        return false;
    watch->alive = alive;
    return true;
}

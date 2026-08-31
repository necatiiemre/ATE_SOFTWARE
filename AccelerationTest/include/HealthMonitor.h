/**
 * @file HealthMonitor.h
 * @brief Recognising the DTN's health-monitor stream on the copper link.
 *
 * Once 28 V is applied the FPGAs broadcast a six-packet cycle without being
 * asked - 1187 + 1083 from the assistant FPGA, 1187 + 1083 + 438 from the
 * manager, and 94 bytes from the MCU. Seeing that traffic is how we know the
 * device has booted, and losing it is how we know it has dropped.
 *
 * Field offsets and the full decode of those packets are still to come; this
 * covers recognition and the heartbeat.
 */

#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Offsets into the UDP payload, shared by every device packet. */
#define HM_UDP_PAYLOAD_OFFSET  42   /* 14 eth + 20 ip + 8 udp, no VLAN */
#define HM_OFF_DEVICE_ID        0
#define HM_OFF_OPERATION_TYPE   2
#define HM_OFF_CONFIG_TYPE      3
#define HM_OFF_FRAME_LENGTH     4
#define HM_OFF_STATUS_ENABLE    6

/* Value of status_enable, which says who sent the packet. */
#define HM_SOURCE_MANAGER    0x01
#define HM_SOURCE_ASSISTANT  0x03
#define HM_SOURCE_MCU        0x05

/** What a received frame turned out to be. */
typedef struct {
    bool     is_device_frame;  /**< came from the DTN's management path */
    uint16_t vl_id;            /**< low 16 bits of the destination MAC */
    uint8_t  operation_type;   /**< 0x52 read, 0x57 write */
    uint8_t  status_enable;    /**< HM_SOURCE_*; only meaningful on 1187/94 byte packets */
    size_t   payload_len;
    const uint8_t *payload;
} hm_frame_t;

/**
 * @brief Classify one frame straight off the wire.
 * @return true when it came from the DTN (UDP 100->100 to 224.224.x.x)
 */
bool hm_classify(const uint8_t *frame, size_t len, hm_frame_t *out);

/** Tracks whether the device is still talking to us. */
typedef struct {
    uint64_t frames;
    uint64_t last_seen_ms;
    bool     alive;
} hm_watch_t;

/** Milliseconds on a monotonic clock, for the heartbeat. */
uint64_t hm_now_ms(void);

void hm_watch_init(hm_watch_t *watch);

/** Record a frame; call for every device frame received. */
void hm_watch_saw_frame(hm_watch_t *watch);

/**
 * @brief Update aliveness against the silence threshold.
 * @return true when the state changed, so the caller can log the transition
 */
bool hm_watch_update(hm_watch_t *watch, unsigned timeout_ms);

#endif /* HEALTH_MONITOR_H */

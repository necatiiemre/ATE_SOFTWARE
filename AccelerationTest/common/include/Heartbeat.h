/**
 * @file Heartbeat.h
 * @brief A monotonic clock, and whether the unit under test is still talking.
 *
 * Every unit's health monitor arrives unasked and at a steady rate, so silence
 * is the signal: a gap longer than the threshold means the unit has dropped,
 * and traffic resuming means it is back. On a vibration rig that is the most
 * valuable thing a run can catch, and it is the same question for the DTN, the
 * VMC and the CMC - so it lives here rather than in any one of them.
 */

#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <stdbool.h>
#include <stdint.h>

/** Tracks whether the device is still talking to us. */
typedef struct {
    uint64_t frames;
    uint64_t last_seen_ms;
    bool     alive;
} hm_watch_t;

/** Milliseconds on a monotonic clock, for the heartbeat and for ageing rows. */
uint64_t hm_now_ms(void);

void hm_watch_init(hm_watch_t *watch);

/** Record a frame; call for every device frame received. */
void hm_watch_saw_frame(hm_watch_t *watch);

/**
 * @brief Update aliveness against the silence threshold.
 * @return true when the state changed, so the caller can log the transition
 */
bool hm_watch_update(hm_watch_t *watch, unsigned timeout_ms);

#endif /* HEARTBEAT_H */

#include "Heartbeat.h"

#include <string.h>
#include <time.h>

uint64_t hm_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)(ts.tv_nsec / 1000000L);
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

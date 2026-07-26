#include "ShutdownSnapshot.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Latest rendered text per slot (heap-owned, may be NULL when empty).
static char *g_slots[SNAP_SLOT_COUNT] = {0};
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

// Human-readable slot names for the dump header.
static const char *slot_name(enum snapshot_slot slot)
{
    switch (slot) {
        case SNAP_SLOT_DTN:    return "DTN PORT STATS";
        case SNAP_SLOT_HEALTH: return "HEALTH MONITOR + POWER SUPPLY";
        default:               return "UNKNOWN";
    }
}

void shutdown_snapshot_init(void)
{
    // Mutex is statically initialized; nothing else to do. Provided so callers
    // can be explicit about setup order if they wish.
}

void shutdown_snapshot_store(enum snapshot_slot slot, const char *text)
{
    if ((int)slot < 0 || slot >= SNAP_SLOT_COUNT) {
        return;
    }

    // Copy outside the lock to keep the critical section short.
    char *copy = NULL;
    if (text != NULL && text[0] != '\0') {
        copy = strdup(text);
        if (copy == NULL) {
            return;  // Out of memory: keep the previous snapshot rather than losing it.
        }
    }

    pthread_mutex_lock(&g_lock);
    free(g_slots[slot]);
    g_slots[slot] = copy;  // may be NULL to clear the slot
    pthread_mutex_unlock(&g_lock);
}

int shutdown_snapshot_dump(const char *header_note)
{
    FILE *fp = fopen(SHUTDOWN_SNAPSHOT_PATH, "w");
    if (fp == NULL) {
        return -1;
    }

    // Wall-clock timestamp for the header.
    time_t now = time(NULL);
    struct tm tm_buf;
    char time_str[64];
    if (localtime_r(&now, &tm_buf) != NULL) {
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_buf);
    } else {
        snprintf(time_str, sizeof(time_str), "unknown");
    }

    fprintf(fp,
            "════════════════════════════════════════════════════════════════════\n"
            "  CTRL+C SNAPSHOT - statistics from the last full second before stop\n"
            "  Captured at : %s\n"
            "  Note        : %s\n"
            "════════════════════════════════════════════════════════════════════\n",
            time_str,
            header_note ? header_note : "-");

    pthread_mutex_lock(&g_lock);
    for (int slot = 0; slot < SNAP_SLOT_COUNT; slot++) {
        fprintf(fp,
                "\n---------- %s ----------\n",
                slot_name((enum snapshot_slot)slot));
        if (g_slots[slot] != NULL) {
            fputs(g_slots[slot], fp);
        } else {
            fprintf(fp, "(no data captured)\n");
        }
    }
    pthread_mutex_unlock(&g_lock);

    fflush(fp);
    fclose(fp);
    return 0;
}

void shutdown_snapshot_cleanup(void)
{
    pthread_mutex_lock(&g_lock);
    for (int slot = 0; slot < SNAP_SLOT_COUNT; slot++) {
        free(g_slots[slot]);
        g_slots[slot] = NULL;
    }
    pthread_mutex_unlock(&g_lock);
}

#include "ShutdownSnapshot.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// One retained render.
struct snap_entry {
    char    *text;    // heap-owned, NULL when the entry is unused
    time_t   when;    // wall clock at the moment it was stored
    unsigned second;  // test second the producer was rendering
};

// A ring per slot, holding the last SNAP_HISTORY_DEPTH renders. One render is
// not enough: the interesting window is the whole drain, where the senders are
// already gone and the receivers are still collecting what was in flight, and
// reading that window a second at a time is how you see the in-flight packets
// actually land rather than only their sum.
static struct snap_entry g_hist[SNAP_SLOT_COUNT][SNAP_HISTORY_DEPTH];
// Where the next render goes, and how many of the ring's entries are in use.
static unsigned g_next[SNAP_SLOT_COUNT] = {0};
static unsigned g_count[SNAP_SLOT_COUNT] = {0};
// The test second the producers are currently rendering, published once per
// second by the main loop. The producers themselves do not carry it into
// store(), and wall clock alone cannot be lined up against a table's own
// header - this can.
static unsigned g_test_second = 0;
// Once frozen, store() stops accepting per-second renders so teardown output
// cannot push the drain window out of the ring.
static bool g_frozen = false;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

// Human-readable slot names for the dump header.
static const char *slot_name(enum snapshot_slot slot)
{
    switch (slot) {
        case SNAP_SLOT_DTN:    return "DTN PORT STATS";
        case SNAP_SLOT_PTP:    return "PTP SESSION STATS";
        case SNAP_SLOT_HEALTH: return "HEALTH MONITOR + POWER SUPPLY";
        case SNAP_SLOT_TOTALS: return "END-OF-TEST TOTALS";
        default:               return "UNKNOWN";
    }
}

// Oldest-to-newest index into a slot's ring. `age` runs 0 (oldest retained)
// to g_count[slot] - 1 (newest). Caller holds the lock.
static const struct snap_entry *entry_at(int slot, unsigned age)
{
    const unsigned count = g_count[slot];
    // g_next points one past the newest, so the oldest sits count entries
    // behind it - modulo the ring, and with the depth added so the subtraction
    // stays non-negative in unsigned arithmetic.
    const unsigned oldest =
        (g_next[slot] + SNAP_HISTORY_DEPTH - count) % SNAP_HISTORY_DEPTH;
    return &g_hist[slot][(oldest + age) % SNAP_HISTORY_DEPTH];
}

void shutdown_snapshot_init(void)
{
    // Mutex is statically initialized; nothing else to do. Provided so callers
    // can be explicit about setup order if they wish.
}

void shutdown_snapshot_set_test_second(unsigned second)
{
    pthread_mutex_lock(&g_lock);
    g_test_second = second;
    pthread_mutex_unlock(&g_lock);
}

void shutdown_snapshot_store(enum snapshot_slot slot, const char *text)
{
    if ((int)slot < 0 || slot >= SNAP_SLOT_COUNT) {
        return;
    }
    if (text == NULL || text[0] == '\0') {
        return;  // Nothing rendered this second; leave the ring as it is.
    }

    // Copy outside the lock to keep the critical section short.
    char *copy = strdup(text);
    if (copy == NULL) {
        return;  // Out of memory: keep what is already retained.
    }

    pthread_mutex_lock(&g_lock);
    // SNAP_SLOT_TOTALS is exempt from the freeze: the freeze exists to stop the
    // per-second producers from pushing the drain window out of their rings,
    // whereas the totals block is written exactly once, after the freeze, by
    // the shutdown path itself.
    if (g_frozen && slot != SNAP_SLOT_TOTALS) {
        pthread_mutex_unlock(&g_lock);
        free(copy);
        return;
    }

    struct snap_entry *e = &g_hist[slot][g_next[slot]];
    free(e->text);  // NULL on an unused entry, the oldest render otherwise
    e->text   = copy;
    e->when   = time(NULL);
    e->second = g_test_second;

    g_next[slot] = (g_next[slot] + 1) % SNAP_HISTORY_DEPTH;
    if (g_count[slot] < SNAP_HISTORY_DEPTH) {
        g_count[slot]++;
    }
    pthread_mutex_unlock(&g_lock);
}

void shutdown_snapshot_freeze(void)
{
    pthread_mutex_lock(&g_lock);
    g_frozen = true;
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
            "  SUMMARY LOG - the closing %d seconds of the test, second by second\n"
            "\n"
            "  Ctrl+C stops the senders only; the receivers keep counting for the\n"
            "  whole RX drain that follows, collecting what was still in flight.\n"
            "  Every per-second table from the last table before the stop through\n"
            "  the end of that drain is kept here, so the in-flight packets can be\n"
            "  watched landing rather than only read as a total. The tables are\n"
            "  grouped by second - one second's DTN, PTP and health monitor blocks\n"
            "  together - so each second can be read as one picture.\n"
            "  The END-OF-TEST TOTALS section is rendered once, after the drain\n"
            "  closed and every worker exited and flushed.\n"
            "\n"
            "  Written at  : %s\n"
            "  Run         : %s\n"
            "════════════════════════════════════════════════════════════════════\n",
            SNAP_HISTORY_DEPTH,
            time_str,
            header_note ? header_note : "-");

    pthread_mutex_lock(&g_lock);

    // Grouped by second rather than by slot: one second's DTN table, PTP table
    // and health monitor block together, then the next second's. Reading the
    // drain means asking what all three said at the same moment - a DTN table
    // eleven pages away from the health monitor block that goes with it makes
    // that a paging exercise.
    //
    // The seconds are gathered from the entries themselves rather than assumed
    // to be a contiguous run: the health monitor keeps its own 1 Hz clock, so
    // its renders do not have to line up one-for-one with the main loop's.
    unsigned seconds[SNAP_HISTORY_DEPTH * SNAP_SLOT_COUNT];
    unsigned nseconds = 0;

    for (int slot = 0; slot < SNAP_SLOT_COUNT; slot++) {
        if (slot == SNAP_SLOT_TOTALS) {
            continue;  // Rendered once at the end, not part of the timeline.
        }
        for (unsigned age = 0; age < g_count[slot]; age++) {
            const struct snap_entry *e = entry_at(slot, age);
            if (e->text == NULL) {
                continue;
            }
            // Insertion sort into a distinct, ascending list. At most a few
            // dozen entries, so the cost is irrelevant next to the file write.
            unsigned pos = 0;
            while (pos < nseconds && seconds[pos] < e->second) {
                pos++;
            }
            if (pos < nseconds && seconds[pos] == e->second) {
                continue;  // Already listed.
            }
            for (unsigned i = nseconds; i > pos; i--) {
                seconds[i] = seconds[i - 1];
            }
            seconds[pos] = e->second;
            nseconds++;
        }
    }

    if (nseconds == 0) {
        fprintf(fp, "\n(no per-second data captured)\n");
    }

    for (unsigned s = 0; s < nseconds; s++) {
        const unsigned sec = seconds[s];
        fprintf(fp,
                "\n"
                "════════════════════ TEST SECOND %u%s ════════════════════\n",
                sec, (s + 1 == nseconds) ? "  (last)" : "");

        for (int slot = 0; slot < SNAP_SLOT_COUNT; slot++) {
            if (slot == SNAP_SLOT_TOTALS) {
                continue;
            }
            for (unsigned age = 0; age < g_count[slot]; age++) {
                const struct snap_entry *e = entry_at(slot, age);
                if (e->text == NULL || e->second != sec) {
                    continue;
                }
                char stamp[64];
                struct tm entry_tm;
                if (localtime_r(&e->when, &entry_tm) != NULL) {
                    strftime(stamp, sizeof(stamp), "%H:%M:%S", &entry_tm);
                } else {
                    snprintf(stamp, sizeof(stamp), "--:--:--");
                }
                fprintf(fp, "\n---------- %s  (%s) ----------\n",
                        slot_name((enum snapshot_slot)slot), stamp);
                fputs(e->text, fp);
            }
        }
    }

    // The totals close the file: one render, taken after the drain window shut
    // and every worker had exited and flushed.
    {
        const char *name = slot_name(SNAP_SLOT_TOTALS);
        fprintf(fp, "\n══════════ %s ══════════\n", name);
        if (g_count[SNAP_SLOT_TOTALS] == 0) {
            fprintf(fp, "(no data captured)\n");
        } else {
            for (unsigned age = 0; age < g_count[SNAP_SLOT_TOTALS]; age++) {
                const struct snap_entry *e = entry_at(SNAP_SLOT_TOTALS, age);
                if (e->text != NULL) {
                    fputs(e->text, fp);
                }
            }
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
        for (unsigned i = 0; i < SNAP_HISTORY_DEPTH; i++) {
            free(g_hist[slot][i].text);
            g_hist[slot][i].text = NULL;
        }
        g_next[slot] = 0;
        g_count[slot] = 0;
    }
    pthread_mutex_unlock(&g_lock);
}

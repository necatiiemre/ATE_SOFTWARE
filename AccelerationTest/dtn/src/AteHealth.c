/*
 * The ATE software's DTN health monitor, driven from this test's receive loop.
 *
 * dtn/ate/HealthMonitor.c is dpdk/src/HealthMonitor/HealthMonitor.c, byte for
 * byte - `diff` it against the reference and nothing comes back. That is the
 * point: what this test prints about the DTN has to be what the ATE software
 * prints, and the only way to be sure of it is for it to be the same code.
 *
 * It is #included here rather than compiled on its own and linked, because the
 * two functions worth having - the parser and the renderer - are static. Editing
 * the copy to unstatic them would be two characters and would cost the property
 * that makes the copy worth anything. Including it puts them in scope instead,
 * and the file stays a file you can diff.
 *
 * Three things about it are not used:
 *
 *   - its thread, its sockets and its query timer. This test already has a socket
 *     on each copper link and a loop reading them, and its display clears the
 *     screen every second - a second thread printing 40 lines from under it would
 *     be unreadable. So the cycle is driven from here: the query goes out with the
 *     rest of the loop's work, and the block is printed where the other tables are
 *     printed. HEALTH_MONITOR_INTERFACE never comes into it, which is just as
 *     well: it names eno12409, and the configuration link is now whichever cable
 *     is in.
 *
 *   - is_health_response(), which filters on VL 0x1188. The ATE software's DTN
 *     answers on that VL; one configured from here answers on VL 38, because the
 *     end-system block we were given writes 0x0026 (see DTN_HEALTH_MONITOR_VL).
 *     The caller does the same check against the VL this test actually uses.
 *
 *   - the firmware and 28 V checks in its thread function, which stop the ATE
 *     software's test. Power is the operator's business here and nothing on a
 *     vibration rig should be stopped by a version string.
 *
 * Two of its neighbours are stand-ins rather than copies, both in dtn/ate/:
 * PsuTelemetryReceiver.h, because this test does not touch power, and
 * ShutdownSnapshot.h, because the log is already a tee of stdout.
 */

#include "HealthMonitor.c"      /* verbatim; see above. Must come first: it
                                   defines _GNU_SOURCE for open_memstream. */

#include "AteHealth.h"

#include <string.h>

/* The interval and the buffer size AteHealth.h publishes have to be the copy's.
 * If the ATE software's ever change, this is where it is caught. */
_Static_assert(ATE_HEALTH_QUERY_INTERVAL_MS == HEALTH_MONITOR_QUERY_INTERVAL_MS,
               "the query interval must be the ATE software's");
_Static_assert(ATE_HEALTH_QUERY_MAX == HEALTH_MONITOR_QUERY_SIZE,
               "the query buffer must be the ATE software's query size");
_Static_assert(ATE_HEALTH_EXPECTED_RESPONSES == HEALTH_MONITOR_EXPECTED_RESPONSES,
               "a full cycle must be what the ATE software expects");

/* The cycle being filled, and the last one that completed. Only completed cycles
 * are shown: a table drawn from a cycle still filling would report ports as
 * missing for the half second before their packet arrives, every second. */
static struct health_cycle_data g_filling;
static struct health_cycle_data g_complete;
static bool     g_seen_any;     /**< the device has answered at least once */
static uint8_t  g_sequence      = HEALTH_MONITOR_SEQ_INIT;
static uint64_t g_queries;
static uint64_t g_short_cycles;

/* HealthMonitor.c hands every rendered block to this on its way to stdout. The
 * log is a tee of stdout, so by the time this is called the block is already in
 * the log and there is nothing left to keep. */
void shutdown_snapshot_store(enum snapshot_slot slot, const char *text)
{
    (void)slot;
    (void)text;
}

void ate_health_reset(void)
{
    memset(&g_filling, 0, sizeof g_filling);
    memset(&g_complete, 0, sizeof g_complete);

    /* The copy's own state too. It is written by the thread this test does not
     * run, so nothing else clears it, and a second run in the same process would
     * otherwise start with the first run's baseline. */
    memset(g_dev_ports, 0, sizeof g_dev_ports);
    memset(g_dev_ports_baseline, 0, sizeof g_dev_ports_baseline);
    memset(&g_last_assistant, 0, sizeof g_last_assistant);
    memset(&g_last_manager, 0, sizeof g_last_manager);
    memset(&g_base_assistant, 0, sizeof g_base_assistant);
    memset(&g_base_manager, 0, sizeof g_base_manager);
    g_last_tables_valid = false;
    g_base_tables_valid = false;

    g_seen_any      = false;
    g_sequence      = HEALTH_MONITOR_SEQ_INIT;
    g_queries       = 0;
    g_short_cycles  = 0;
}

int ate_health_build_query(uint8_t *out, size_t cap)
{
    if (cap < HEALTH_MONITOR_QUERY_SIZE)
        return -1;

    /* The template and the sequence rule are the ATE software's, taken from the
     * file rather than retyped. The sequence lands on the last byte, which on
     * this frame is inside the UDP length - the configuration frames carry theirs
     * outside it, and that difference is the device's, not ours. */
    memcpy(out, health_query_template, HEALTH_MONITOR_QUERY_SIZE);
    out[HEALTH_MONITOR_QUERY_SIZE - 1] = g_sequence;
    g_sequence = g_sequence >= 255 ? 1 : (uint8_t)(g_sequence + 1);
    g_queries++;
    return HEALTH_MONITOR_QUERY_SIZE;
}

void ate_health_cycle(void)
{
    /* The cycle rolls over whether or not anything came back. A device that has
     * gone quiet then shows a block of NOT RECEIVED, which is what the ATE
     * software shows and the truth; leaving the last good block on the screen
     * would say the opposite for as long as the silence lasted. */
    g_complete = g_filling;

    /* What the copy's own thread does at this point: keep the per-port counters
     * and the two port tables, so the end of the run can difference them against
     * the baseline. Called from here because that thread is not running. */
    health_store_port_snapshot(&g_filling.assistant);
    health_store_port_snapshot(&g_filling.manager);
    health_store_fpga_table(&g_last_assistant, &g_filling.assistant);
    health_store_fpga_table(&g_last_manager, &g_filling.manager);

    if (g_filling.total_responses_received > 0) {
        /* The first reading is the baseline. The device's counters are absolute -
         * it never clears them - so without one, a port's undefined-VL count is
         * however many it has dropped since it was last powered, which says
         * nothing about this run. The reference takes its baseline in a quiet
         * window; the equivalent here is the first cycle, which closes before the
         * legs have put any real traffic through. */
        if (!g_seen_any)
            health_monitor_mark_port_baseline();
        g_seen_any = true;
    } else if (!g_seen_any) {
        goto clear;   /* nothing has ever answered: not a short cycle yet */
    }
    if (g_filling.total_responses_received < HEALTH_MONITOR_EXPECTED_RESPONSES)
        g_short_cycles++;
clear:
    memset(&g_filling, 0, sizeof g_filling);
}

void ate_health_render_start_end(void)
{
    health_monitor_render_port_tables(stdout);
}

void ate_health_ingest(const uint8_t *frame, size_t len)
{
    health_parse_response(frame, len, &g_filling);
}

bool ate_health_render(void)
{
    if (!g_seen_any)
        return false;
    health_print_tables(&g_complete);
    return true;
}

unsigned ate_health_responses(void)
{
    return (unsigned)g_complete.total_responses_received;
}

void ate_health_counts(uint64_t *queries, uint64_t *short_cycles)
{
    if (queries)
        *queries = g_queries;
    if (short_cycles)
        *short_cycles = g_short_cycles;
}

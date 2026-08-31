#pragma once

#include <stdio.h>

// ============================================================================
// ShutdownSnapshot
// ----------------------------------------------------------------------------
// Keeps the most recently rendered per-second statistics blocks in memory so
// that, the moment a Ctrl+C (SIGINT) / SIGTERM is received, we can dump the
// LAST FULL SECOND captured BEFORE the signal into a single log file.
//
// Three producers, each running on its own thread, overwrite their slot once
// per second with the text they just printed to the console/log:
//   - SNAP_SLOT_DTN    : the DTN port statistics table   (main loop thread)
//   - SNAP_SLOT_PTP    : the PTP session statistics table (main loop thread)
//   - SNAP_SLOT_HEALTH : Health Monitor + PSU telemetry   (health monitor thread)
//
// A fourth slot, SNAP_SLOT_TOTALS, is written once at the end of the run
// instead of every second: the end-of-test totals table (aggregate PRBS
// counters, the non-PRBS purity check, and the PTP / Health Monitor totals).
//
// On shutdown, shutdown_snapshot_dump() writes both slots (in order) to one
// file. Because the slots are only ever refreshed during normal per-second
// printing, dumping them right after the main loop exits captures the state as
// it was in the second just before the stop was requested - not a fresh render
// produced during teardown.
// ============================================================================

// Slot identifiers. Keep SNAP_SLOT_COUNT last.
enum snapshot_slot {
    SNAP_SLOT_DTN = 0,     // DTN statistics table
    SNAP_SLOT_PTP = 1,     // PTP session statistics table
    SNAP_SLOT_HEALTH = 2,  // Health Monitor tables + PSU telemetry table
    SNAP_SLOT_TOTALS = 3,  // End-of-test totals (PRBS totals, purity, PTP, HM)
    SNAP_SLOT_COUNT
};

// Default output file - a SEPARATE "summary log". It never touches the normal
// per-second log (dpdk_app.log); it only holds the single last-second summary.
// MainSoftware fetches /tmp/dpdk_app.log after each run; this lives next to it
// so it can be collected the same way. The file is named with the final test
// result naming scheme so it carries the correct name on the server too.
#ifndef SHUTDOWN_SNAPSHOT_PATH
#define SHUTDOWN_SNAPSHOT_PATH "/tmp/DTN_IRSW_EQ_Test_Result_Summary_Log_Files.log"
#endif

// Initialize internal state (mutex). Safe to call more than once; only the
// first call has an effect. Optional - the store/dump calls self-initialize.
void shutdown_snapshot_init(void);

// Replace the text stored in `slot` with a private copy of `text`.
// Thread-safe. A NULL or empty `text` clears the slot. Called once per second
// by each producer with the block it just rendered.
void shutdown_snapshot_store(enum snapshot_slot slot, const char *text);

// Stop updating the per-second slots. Called once when a stop (Ctrl+C) is
// requested so the summary keeps the last full second captured BEFORE the
// signal, while the normal per-second log keeps printing as usual. Thread-safe;
// irreversible for the remainder of the run. SNAP_SLOT_TOTALS is exempt - it is
// written once by the shutdown path itself, after the freeze.
void shutdown_snapshot_freeze(void);

// Write every non-empty slot (in slot order) to SHUTDOWN_SNAPSHOT_PATH,
// prefixed by a header that includes the wall-clock time and `header_note`
// (may be NULL). Returns 0 on success, -1 on failure. Thread-safe.
int shutdown_snapshot_dump(const char *header_note);

// Free internal buffers. Optional; intended for a clean process exit.
void shutdown_snapshot_cleanup(void);

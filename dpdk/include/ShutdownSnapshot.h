#pragma once

#include <stdio.h>

// ============================================================================
// ShutdownSnapshot
// ----------------------------------------------------------------------------
// Keeps the most recently rendered per-second statistics blocks in memory so
// that, the moment a Ctrl+C (SIGINT) / SIGTERM is received, we can dump the
// LAST FULL SECOND captured BEFORE the signal into a single log file.
//
// Two producers, each running on its own thread, overwrite their slot once per
// second with the text they just printed to the console/log:
//   - SNAP_SLOT_DTN    : the DTN port statistics table   (main loop thread)
//   - SNAP_SLOT_HEALTH : Health Monitor + PSU telemetry   (health monitor thread)
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
    SNAP_SLOT_HEALTH = 1,  // Health Monitor tables + PSU telemetry table
    SNAP_SLOT_COUNT
};

// Default output file. MainSoftware fetches /tmp/dpdk_app.log after each run;
// this snapshot lives next to it so it can be collected the same way.
#ifndef SHUTDOWN_SNAPSHOT_PATH
#define SHUTDOWN_SNAPSHOT_PATH "/tmp/dpdk_ctrlc_snapshot.log"
#endif

// Initialize internal state (mutex). Safe to call more than once; only the
// first call has an effect. Optional - the store/dump calls self-initialize.
void shutdown_snapshot_init(void);

// Replace the text stored in `slot` with a private copy of `text`.
// Thread-safe. A NULL or empty `text` clears the slot. Called once per second
// by each producer with the block it just rendered.
void shutdown_snapshot_store(enum snapshot_slot slot, const char *text);

// Write every non-empty slot (in slot order) to SHUTDOWN_SNAPSHOT_PATH,
// prefixed by a header that includes the wall-clock time and `header_note`
// (may be NULL). Returns 0 on success, -1 on failure. Thread-safe.
int shutdown_snapshot_dump(const char *header_note);

// Free internal buffers. Optional; intended for a clean process exit.
void shutdown_snapshot_cleanup(void);

/**
 * @file ShutdownSnapshot.h
 * @brief Stand-in for the ATE software's Ctrl+C snapshot store.
 *
 * HealthMonitor.c renders each health block into a memory buffer, prints that
 * buffer, and hands the same text here so the ATE software can dump the last full
 * second when it is interrupted. This test has no use for it: its log is a tee of
 * stdout (see common/include/Log.h), so the block is in the log the moment it is
 * printed, every cycle and in order, without anything being stored for later.
 *
 * So the slot enumeration is kept - HealthMonitor.c names SNAP_SLOT_HEALTH - and
 * the store does nothing. See dtn/src/AteHealth.c for the whole arrangement.
 */

#ifndef SHUTDOWN_SNAPSHOT_H
#define SHUTDOWN_SNAPSHOT_H

enum snapshot_slot {
    SNAP_SLOT_DTN = 0,
    SNAP_SLOT_PTP = 1,
    SNAP_SLOT_HEALTH = 2,
    SNAP_SLOT_COUNT
};

void shutdown_snapshot_store(enum snapshot_slot slot, const char *text);

#endif /* SHUTDOWN_SNAPSHOT_H */

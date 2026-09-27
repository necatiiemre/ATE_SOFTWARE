/**
 * @file ShutdownSnapshot.h
 * @brief Stand-in for dpdk_cmc's render capture.
 *
 * The copied health-monitor files redirect every printf through render_out(), so
 * that the ATE software's main loop can capture a second's worth of tables into a
 * memory stream and write them to its summary log without re-rendering anything
 * during teardown. The target is thread-local there, so a capture on the main loop
 * never swallows what another thread prints from the same file.
 *
 * This test has no use for that: its log is a tee of stdout (common/include/Log.h),
 * so a table is in the log the moment it is printed, in order, with nothing stored
 * for later. So render_out() is stdout and always stdout - which is exactly what
 * the reference's own function returns when no capture is running.
 *
 * Nothing is declared that would need a definition, deliberately: the real header
 * carries a __thread FILE * that ShutdownSnapshot.c defines, and there is no
 * ShutdownSnapshot.c here. The point of a stand-in is that it stands in.
 */

#ifndef CMC_HM_SHUTDOWN_SNAPSHOT_H
#define CMC_HM_SHUTDOWN_SNAPSHOT_H

#include <stdio.h>

static inline FILE *render_out(void)
{
    return stdout;
}

#endif /* CMC_HM_SHUTDOWN_SNAPSHOT_H */

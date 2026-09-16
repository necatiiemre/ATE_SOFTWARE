/**
 * @file ShutdownPhase.h
 * @brief The phase the application is in while shutting down.
 *
 * Ctrl+C no longer ends the run immediately. The first SIGINT/SIGTERM stops
 * transmitting and walks the app through a fixed sequence so the counters are
 * settled before anything is reported:
 *
 *   RUNNING   normal TX + RX.
 *   DRAINING  TX halted, RX still running for CMC_DRAIN_SECONDS so packets
 *             already on the wire land and the per-VL counters stop moving.
 *             Health-monitor traffic keeps arriving and printing throughout.
 *   SNAPSHOT  VL-to-VL report and final-result snapshot are written.
 *   MMMS      the read-smmm file handover runs against an otherwise idle wire.
 *   EXIT      workers stop, cleanup runs.
 *
 * A second SIGINT/SIGTERM sets force_quit from any phase as an escape hatch.
 *
 * The variable is written from a signal handler and read from the RX hot path,
 * so it is a plain volatile int rather than an enum-typed object.
 */
#ifndef SHUTDOWN_PHASE_H
#define SHUTDOWN_PHASE_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CMC_PHASE_RUNNING  = 0,
    CMC_PHASE_DRAINING = 1,
    CMC_PHASE_SNAPSHOT = 2,
    CMC_PHASE_MMMS     = 3,
    CMC_PHASE_EXIT     = 4
};

extern volatile int cmc_shutdown_phase;

#ifdef __cplusplus
}
#endif

#endif /* SHUTDOWN_PHASE_H */

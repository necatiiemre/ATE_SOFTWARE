/**
 * @file PsuTelemetryReceiver.h
 * @brief Stand-in for the ATE software's PSU telemetry receiver.
 *
 * HealthMonitor.c appends the power-supply table to the bottom of every health
 * block. In the ATE software those readings are pushed over UDP by MainSoftware,
 * which drives the bench supplies. The acceleration test does not touch power at
 * all - the operator switches the unit on by hand, which is the whole point of a
 * vibration rig test - so there is nothing to append and the table is empty.
 *
 * It prints nothing rather than printing an empty table: an empty table would
 * read as "the supplies answered and everything is zero", which is the one thing
 * it must not say.
 *
 * This is the only piece of HealthMonitor.c's surroundings that the copy does not
 * bring with it. See dtn/src/AteHealth.c.
 */

#ifndef PSU_TELEMETRY_RECEIVER_H
#define PSU_TELEMETRY_RECEIVER_H

#include <stdio.h>

static inline void psu_telem_print_table(FILE *out)
{
    (void)out;
}

#endif /* PSU_TELEMETRY_RECEIVER_H */

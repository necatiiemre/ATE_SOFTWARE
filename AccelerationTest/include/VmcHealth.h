/**
 * @file VmcHealth.h
 * @brief The VMC's health-monitor reports, taken apart.
 *
 * The VMC sends these on its own; nothing has to ask for them. They arrive on
 * one interface, AFDX-framed the way the rest of this rig's traffic is - the VL
 * id in the low two bytes of the destination MAC, over UDP - and they are the
 * same reports dpdk_vmc reads. VmcMessages.h holds the wire format and this
 * sorts, byte-swaps and stores them the way
 * dpdk_vmc/src/health_monitor/health_monitor.c does.
 *
 * Six kinds arrive, from two sides of the VMC (FLCS and VS):
 *
 *   CPU usage   its own VL,  Pcs_profile_stats
 *   PBIT        its own VL,  vmc_pbit_data_t, guarded by a message id because
 *                            other traffic shares the VL
 *   CBIT        one VL carrying four different reports, told apart by the
 *               message id in the first payload byte
 *
 * Which VL is which, and the interface they come in on, are in AppConfig.h -
 * one table, so a different rig is one edit.
 */

#ifndef VMC_HEALTH_H
#define VMC_HEALTH_H

#include "AppConfig.h"
#include "VmcMessages.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Which side of the VMC a report came from. */
typedef enum { VMC_FLCS = 0, VMC_VS = 1, VMC_SIDE_COUNT } vmc_side_t;

/** The six report kinds, in the order the dashboard lists them. */
typedef enum {
    VMC_REPORT_CPU_USAGE = 0,
    VMC_REPORT_PBIT,
    VMC_REPORT_BM_ENGINEERING,
    VMC_REPORT_BM_FLAG,
    VMC_REPORT_DTN_ES,
    VMC_REPORT_DTN_SW,
    VMC_REPORT_COUNT
} vmc_report_t;

/** How much of a given report has been seen, and when. */
typedef struct {
    uint64_t packets;
    uint64_t first_ms;
    uint64_t last_ms;
} vmc_slot_t;

/** One side's latest of everything. */
typedef struct {
    vmc_slot_t                   seen[VMC_REPORT_COUNT];
    Pcs_profile_stats            cpu_usage;
    vmc_pbit_data_t              pbit;
    bm_engineering_cbit_report_t bm_engineering;
    bm_flag_cbit_report_t        bm_flag;
    dtn_es_cbit_report_t         dtn_es;
    dtn_sw_cbit_report_t         dtn_sw;
} vmc_report_set_t;

typedef struct {
    const vmc_config_t *config;
    vmc_report_set_t    side[VMC_SIDE_COUNT];

    uint64_t frames;             /**< frames that reached us at all */
    uint64_t accepted;           /**< frames that became a report */
    uint64_t not_health;         /**< a VL that is not one of ours */
    uint64_t too_short;          /**< the right VL, not enough bytes for the report */
    uint64_t unknown_message;    /**< the right VL, a message id we do not know */
    uint64_t empty;              /**< a DTN report with nothing in it, as dpdk_vmc skips */

    uint16_t last_unknown_vl;    /**< so an unexpected VL can be named, not just counted */
    uint8_t  last_unknown_msg;
} vmc_health_t;

void vmc_health_init(vmc_health_t *health, const vmc_config_t *config);

/**
 * @brief Sort one frame straight off the wire into a report.
 *
 * Takes the whole frame: the VL id is in the destination MAC and the report
 * starts after the Ethernet, IP and UDP headers, tagged or not.
 *
 * @return true when a report was stored
 */
bool vmc_health_ingest(vmc_health_t *health, const uint8_t *frame, size_t len);

/** Redraw the dashboard in place. */
void vmc_health_render(const vmc_health_t *health, uint64_t elapsed_s);

/** Write every field of every report that arrived into the log. */
void vmc_health_log_summary(const vmc_health_t *health);

/** Name of a report kind, for the dashboard and the log. */
const char *vmc_report_name(vmc_report_t report);

/** "FLCS" or "VS". */
const char *vmc_side_name(vmc_side_t side);

/** The VL id a given report arrives on, from the configuration. */
uint16_t vmc_report_vl(const vmc_config_t *config, vmc_side_t side, vmc_report_t report);

#endif /* VMC_HEALTH_H */

/**
 * @file VmcHealth.h
 * @brief The VMC's health-monitor reports, taken apart.
 *
 * The VMC sends these on its own; nothing has to ask for them. They arrive on
 * two interfaces, one per side of the VMC, AFDX-framed the way the rest of this
 * rig's traffic is - the VL id in the low two bytes of the destination MAC,
 * over UDP - and they are the same reports dpdk_vmc reads. VmcMessages.h holds the wire format and this
 * sorts, byte-swaps and stores them the way
 * dpdk_vmc/src/health_monitor/health_monitor.c does.
 *
 * Six kinds arrive, from two sides of the VMC (FLCS and VS):
 *
 *   CPU usage   its own VL,  Pcs_profile_stats
 *   PBIT        its own VL,  vmc_pbit_data_t, guarded by a message id because
 *                            other traffic shares the VL
 *   CBIT        one VL carrying four different reports, told apart by the
 *               message id in the first payload byte - and the end-system one
 *               arrives twice, for the end system and for the switch's embedded
 *               end system, told apart by its network type byte
 *   counters    its own VL,  REPORT_MSG - four counters for each of six PHY
 *               ports, with no header at all, so the VL id is the whole of
 *               what identifies it
 *
 * Which VL is which, and which interface carries which side, are in
 * AppConfig.h - one table, so a different rig is one edit.
 *
 * The side comes from the interface, not the VL id: that is how the rig is
 * wired and it is the thing known for certain, while the ids are dpdk_vmc's and
 * unconfirmed here. A VL id that names the other side does not move the report;
 * it is counted as a mismatch and shown, because a pair of swapped cables looks
 * exactly like that and nothing else does.
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
    VMC_REPORT_DTN_ES,       /**< network type 0: the end system itself */
    VMC_REPORT_DTN_ES_SW,    /**< network type 1: the switch's embedded end system */
    VMC_REPORT_DTN_SW,
    VMC_REPORT_COUNTERS,
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
    dtn_es_cbit_report_t         dtn_es;      /**< network type 0 */
    dtn_es_cbit_report_t         dtn_es_sw;   /**< network type 1 */
    dtn_sw_cbit_report_t         dtn_sw;
    REPORT_MSG                   counters;
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
    uint64_t unknown_net_type;   /**< an end-system report that is neither kind */

    uint8_t  last_unknown_net_type;

    uint64_t side_mismatch;      /**< the VL id named the side the interface did not */

    uint16_t last_unknown_vl;    /**< so an unexpected VL can be named, not just counted */
    uint8_t  last_unknown_msg;
    uint16_t last_mismatch_vl;

    /** Per interface, so a link that has gone quiet is visible on its own. */
    struct {
        uint64_t frames;
        uint64_t accepted;
        uint64_t last_ms;
    } link[APP_MAX_VMC_LINKS];
} vmc_health_t;

void vmc_health_init(vmc_health_t *health, const vmc_config_t *config);

/**
 * @brief Sort one frame straight off the wire into a report.
 *
 * Takes the whole frame: the VL id is in the destination MAC and the report
 * starts after the Ethernet, IP and UDP headers, tagged or not.
 *
 * @param link which of the configured interfaces it arrived on; that is what
 *             decides the side
 * @return true when a report was stored
 */
bool vmc_health_ingest(vmc_health_t *health, uint8_t link,
                       const uint8_t *frame, size_t len);

/**
 * @brief Print the PHY counter report.
 *
 * The one report dpdk_vmc has no printer for - it is newer than that code - so
 * this one is ours, laid out like the printers beside it.
 */
void print_phy_counter_report(const REPORT_MSG *data, const char *device_name);

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

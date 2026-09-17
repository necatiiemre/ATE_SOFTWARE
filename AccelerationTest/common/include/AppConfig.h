/**
 * @file AppConfig.h
 * @brief Rig settings: copper links and timings.
 *
 * Compiled in for now. Everything here describes how the workstation is wired
 * rather than what the test does, so it is the first thing to check when the
 * rig changes.
 *
 * Powering the unit is out of scope: the supply is operated separately, and the
 * application only observes the unit once it is live.
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define APP_MAX_COPPER_LINKS 2

/** One copper link from the workstation to a DTN end-system port. */
typedef struct {
    uint8_t     dtn_port;   /**< 32 (1G) or 33 (100M) */
    const char *iface;
    const char *speed;      /**< for the operator, not used in code */
} copper_link_t;

typedef struct {
    unsigned frame_gap_ms;            /**< pause between configuration frames */
    unsigned device_ready_timeout_s;  /**< how long to wait for the unit to come up */
    unsigned status_reply_timeout_ms;
    unsigned heartbeat_timeout_ms;    /**< silence that counts as "unit lost" */
    unsigned display_interval_ms;     /**< how often the live table is redrawn */
} timing_config_t;

/** The copper links, in DTN port order. */
const copper_link_t *app_config_copper(size_t *count);

/** Interface carrying a given DTN port, or NULL if that port is not copper. */
const char *app_config_iface_for_port(uint8_t dtn_port);

const timing_config_t *app_config_timing(void);

/**
 * @brief Whether to append the DTN's own management VLs to a round.
 *
 * Off by default, which makes the configuration byte-identical to the capture:
 * 122 records, the fibre links and the two taps and nothing else. That is also
 * what stops the DTN's own health monitor, because VL 4488 - port 34 out to the
 * 100M copper port - is one of the records the capture leaves out.
 *
 * Turn it on with --keep-management to append VL 4419-4490 verbatim, so the
 * device keeps its own health monitor and its answer to a 0x52 query. The fibre
 * part of the table is unchanged either way.
 */
bool app_config_management_vls(void);
void app_config_set_management_vls(bool keep);

/**
 * @brief Whether the live port table lists all 35 ports or only the round's.
 *
 * Off by default, which keeps the table short. Turn it on with --all-ports when
 * the question is what the device thinks it has rather than what the round is
 * using - a port the device never reports, or reports with nothing on it, is
 * visible only this way. The end-of-run log always records all 35.
 */
bool app_config_all_ports(void);
void app_config_set_all_ports(bool all);

#define APP_MAX_VMC_LINKS 2

/** One interface, and which side of the VMC it carries. */
typedef struct {
    const char *iface;
    uint8_t     side;                 /**< 0 = FLCS, 1 = VS; see vmc_side_t */
} vmc_link_t;

/**
 * @brief Everything the VMC test needs to find its traffic.
 *
 * One struct, one place. The VMC's health monitor arrives on two interfaces,
 * one per side, and each report is sorted by the VL id in the low two bytes of
 * the destination MAC; the CBIT VLs carry four different reports, told apart by
 * the message id in the first payload byte.
 *
 * The interface says which side a report came from, because that is how the rig
 * is wired and it is the thing known for certain. The VL id says which report
 * it is. When the VL id belongs to the other side the report is still filed
 * under its interface, and the disagreement is counted and shown - two swapped
 * cables look exactly like that and nothing else does.
 *
 * The ids are the ones dpdk_vmc uses. They have not been confirmed against this
 * rig, which is exactly why they are a table rather than constants scattered
 * through the decoder.
 */
/**
 * How to read the PHY counter report's sixty-fours.
 *
 * It arrived without a byte order, so this says which end to read from -
 * or, by default, that the decoder should work it out from the numbers.
 */
typedef enum {
    VMC_COUNTERS_AUTO = 0,   /**< keep whichever reading is plausible */
    VMC_COUNTERS_BIG,        /**< always big-endian, like the other reports */
    VMC_COUNTERS_LITTLE      /**< always little-endian, as a copied struct is */
} vmc_counter_order_t;

typedef struct {
    vmc_link_t links[APP_MAX_VMC_LINKS];
    uint8_t    link_count;

    uint16_t flcs_cpu_usage;          /**< Pcs_profile_stats */
    uint16_t vs_cpu_usage;
    uint16_t flcs_pbit_request;       /**< we do not send these; listed for completeness */
    uint16_t vs_pbit_request;
    uint16_t flcs_pbit_response;      /**< vmc_pbit_data_t */
    uint16_t vs_pbit_response;
    uint16_t flcs_cbit;               /**< four reports, sorted by message id */
    uint16_t vs_cbit;
    uint16_t flcs_counters;           /**< REPORT_MSG, no header and no message id */
    uint16_t vs_counters;

    uint8_t  msg_dtn_es;              /**< dtn_es_cbit_report_t */
    uint8_t  msg_dtn_sw;              /**< dtn_sw_cbit_report_t */
    uint8_t  msg_bm_engineering;      /**< bm_engineering_cbit_report_t */
    uint8_t  msg_bm_flag;             /**< bm_flag_cbit_report_t */
    uint8_t  msg_pbit_response;       /**< guards the PBIT VLs, which carry other traffic */
    uint8_t  msg_pbit_request;        /**< identifier we put in the request we send */
    unsigned pbit_resend_interval_s;  /**< keep asking until both sides answer */

    /* The DTN end-system CBIT report arrives twice, for two different things,
     * and says which by its network type byte. Both come in on the same VL with
     * the same message id, so a decoder that ignores this keeps whichever
     * arrived last and loses the other one entirely. */
    uint8_t  net_type_es;             /**< the end system itself */
    uint8_t  net_type_sw_es;          /**< the switch's embedded end system */

    /**
     * Which DTN switch CBIT report to keep when two arrive together.
     *
     * Both sides send two of these at once, on the same VL with the same
     * message id, and one of them is empty - a report for a link that is not
     * carrying anything. Nothing in the header separates them; comm_status
     * does. So the filled one is the one whose comm_status is
     * sw_comm_status_live, and the other is left out rather than printed over
     * it.
     *
     * The value is here because it is a property of the rig, not of the
     * decoder: change it and nothing else. The dashboard lists every
     * comm_status that actually arrived on each side, with how many of them
     * carried data, so the right value is read off a run rather than guessed -
     * and if the filter keeps nothing at all it says so, loudly, instead of
     * showing an empty panel.
     *
     * Set sw_filter_by_comm_status to false to go back to keeping whichever
     * arrived last.
     */
    bool     sw_filter_by_comm_status;
    uint8_t  sw_comm_status_live;

    /**
     * Byte order of the PHY counter report.
     *
     * Every other VMC report is big-endian and carries a header that says so.
     * This one arrived as a bare packed C struct with neither - which is what a
     * sender that copies its own memory onto the wire produces, and that is
     * host order, whichever the VMC's is. Reading it the wrong way round gives
     * counts around 10^19 rather than something plausible.
     *
     * Rather than pick one and be wrong on a rig that changes, the default is
     * VMC_COUNTERS_AUTO: both readings are taken and the plausible one is kept,
     * per report. A packet counter needs a hundred years at line rate to reach
     * 2^48, so of the two readings of the same bytes at most one can be small -
     * and that is the one the device meant. Force it either way if a rig ever
     * needs it; the choice is named on the dashboard and in the log either way.
     */
    vmc_counter_order_t counters_order;
} vmc_config_t;

const vmc_config_t *app_config_vmc(void);

/** Which copper link the configuration frames go out of. */
const copper_link_t *app_config_config_link(void);

#endif /* APP_CONFIG_H */

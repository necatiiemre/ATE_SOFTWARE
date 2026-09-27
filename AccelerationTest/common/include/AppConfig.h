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
 * @brief What the DTN's copper legs put on the wire.
 *
 * config1 carries the workstation's traffic through the DTN to the VMC and back,
 * on both copper links. Both legs get the same rate: DTN port 33 is the 100M
 * link, so 100 Mbit/s is its ceiling, and holding the 1G leg to the same figure
 * is what keeps the two comparable - a difference between them is then the
 * unit's rather than the cable's.
 *
 * frame_bytes is the whole frame on the wire, which for a data-plane frame is
 * exactly its IP total_length - there is no AFDX byte after it, because the DTN
 * writes its sequence into the last byte of the payload instead.
 *
 * 1509 is what dpdk_vmc's frames are once the switch has stripped their 802.1Q
 * tag, which is to say it is what the VMC already answers: 42 bytes of header
 * and a 1467-byte payload, of which 8 are the sequence and 1459 the PRBS. The VL
 * records declare LMAX 1518, so there is room under the ceiling.
 */
typedef struct {
    double   target_mbps;   /**< per leg */
    uint16_t frame_bytes;   /**< on the wire, AFDX sequence byte included */
} dtn_leg_config_t;

const dtn_leg_config_t *app_config_dtn_legs(void);

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

/* ------------------------------------------------------------------ */
/* CMC                                                                */
/* ------------------------------------------------------------------ */

#define APP_MAX_CMC_NETS 2     /**< DSM-A and DSM-B */
#define APP_MAX_CMC_PMMS 2     /**< PMM1 and PMM2; the rig fits two */

/**
 * @brief One DSM network: an interface, and the L2 identity it carries.
 *
 * The DPDK reference runs both networks down one fibre port and tells them
 * apart by the 802.1Q tag the Cumulus switch adds and strips. Here each
 * network has its own copper interface, so the interface is what tells them
 * apart and the tag is not needed - see cmc_config_t::vlan_tagged.
 */
typedef struct {
    const char *iface;
    const char *label;        /**< "NET-A" / "NET-B", as the tables print it */
    const char *unit_label;   /**< "DSMA" / "DSMB", as the rig is wired */
    uint8_t     src_mac_tail; /**< last byte of the source MAC: 0x20 or 0x40 */
    uint16_t    tx_vlan;      /**< only used when vlan_tagged is on */
    uint16_t    rx_vlan;      /**< only used when vlan_tagged is on */
} cmc_net_link_t;

/**
 * @brief One PMM line. Listen only: the SMMM sends, we count and check.
 *
 * The addresses are a consistency check rather than a filter - a packet whose
 * addresses do not match is still counted, and the mismatch shows in the
 * table, because that is how a wrong assumption here becomes visible instead
 * of silently dropping traffic.
 */
typedef struct {
    const char *iface;
    const char *label;        /**< "PMM1" / "PMM2" */
    const char *unit_ip;      /**< the SMMM, the sender */
    uint16_t    unit_port;
    const char *local_ip;     /**< the PMM, the addressee */
    uint16_t    local_port;
    uint16_t    rx_vlan;      /**< only used when vlan_tagged is on */
} cmc_pmm_link_t;

/**
 * @brief Everything the CMC test needs: four interfaces and the wire format.
 *
 * One place, as with the VMC. The acceleration rig cables each CMC module to
 * its own interface, so which interface is which module is the first thing
 * that changes when the rig is re-cabled.
 */
typedef struct {
    cmc_net_link_t nets[APP_MAX_CMC_NETS];
    uint8_t        net_count;
    cmc_pmm_link_t pmms[APP_MAX_CMC_PMMS];
    uint8_t        pmm_count;

    /**
     * Whether frames carry an 802.1Q tag.
     *
     * Off, because the links are direct. In the DPDK rig the server sends
     * tagged, the switch strips the tag before the CMC sees it, the CMC
     * answers untagged and the switch tags it again on the way back - so the
     * frame the unit handles is the untagged one, and that is the frame this
     * puts on a direct cable. Turn it on only if a switch is put back in
     * between; the payload keeps the VLAN-mode length either way so that what
     * the unit sees stays byte for byte what it sees today.
     */
    bool     vlan_tagged;

    uint16_t tx_vl_start;      /**< 10001: what the server sends */
    uint16_t rx_vl_start;      /**< 10521: what the CMC sends back */
    uint16_t vl_count;         /**< 104 VLs per network */

    double   target_gbps;      /**< for the whole test, split across the networks */
    unsigned warmup_s;         /**< run this long, then zero the counters */
    unsigned stats_interval_s; /**< how often the dashboard is redrawn */
} cmc_config_t;

const cmc_config_t *app_config_cmc(void);



/** Which copper link the configuration frames go out of. */
const copper_link_t *app_config_config_link(void);

#endif /* APP_CONFIG_H */

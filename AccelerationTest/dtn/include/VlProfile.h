/**
 * @file VlProfile.h
 * @brief The VL routing profiles the DTN is configured with.
 *
 * The unit on the other side of the fibre has 12 ports and the DTN has 32, so
 * the fibre links are covered in three rounds. Each round pairs six low ports
 * with six high ports in both directions.
 *
 * Two health-monitor streams reach the workstation, and they are different
 * things:
 *
 *   - the fibre-side unit's own health monitor, which arrives on a DTN fibre
 *     port and is routed to copper by the VLs in vl_hm_t;
 *   - the DTN's own health monitor, which comes from its internal management
 *     port 34 and is routed by the management VLs below.
 *
 * Port 34 is not a physical port: it does not appear in the DTN's port table,
 * but it is the source of the PTP Sync broadcast and of the answer to a 0x52
 * status query, and the health data reports it as the last of 35 ports.
 */

#ifndef VL_PROFILE_H
#define VL_PROFILE_H

#include "DtnConfig.h"

#define VL_PROFILE_MAX_LINKS   16
#define VL_PROFILE_MAX_GROUPS  2
#define VL_PROFILE_MAX_HM      4
#define VL_PROFILE_MAX_COMMS   4
/* The biggest round is the fibre links, two taps, the DTN's own health monitor
 * and the copper legs: 80 + 2 + 1 + 240 for config1. The full management path
 * adds 72 more. */
#define VL_PROFILE_MAX_RECORDS 512

/** One directed fibre link, source port to destination port. */
typedef struct {
    uint8_t src;
    uint8_t dst;
} vl_link_t;

/** A run of links sharing one contiguous VL range. */
typedef struct {
    uint16_t         vl_base;      /**< first VL id of the group */
    uint16_t         vls_per_link; /**< VL ids each link consumes */
    uint8_t          link_count;
    const vl_link_t *links;
} vl_link_group_t;

/**
 * @brief A tap that routes the fibre-side unit's health monitor out to copper.
 *
 * This is not the DTN's own health monitor - see the management VLs.
 */
typedef struct {
    uint16_t vl_id;
    uint8_t  src_port;
    uint8_t  dst_port;
    uint8_t  flags;      /**< flag nibble; 0x9, as in the capture */
} vl_hm_t;

/**
 * @brief One leg of the workstation - DTN - VMC path.
 *
 * Neither a fibre link nor a tap. The workstation generates PRBS traffic on a
 * copper port, the DTN carries it out of a fibre port to the VMC, and the VMC's
 * answer comes back in at that same fibre port and out of the same copper port.
 * Each direction is one of these: a contiguous run of VLs from one port to one
 * other.
 *
 * The three-way path is the point of it - the workstation proves the DTN
 * forwards copper to fibre and back, and the VMC at the far end proves the
 * fibre side is alive, in one flow rather than two separate tests.
 */
typedef struct {
    uint16_t    vl_first;
    uint16_t    vl_count;
    uint8_t     src_port;
    uint8_t     dst_port;
    const char *label;     /**< what the routing display calls it */
} vl_run_t;

typedef struct {
    const char      *name;
    const char      *description;
    uint8_t          group_count;
    vl_link_group_t  groups[VL_PROFILE_MAX_GROUPS];
    uint8_t          hm_count;
    vl_hm_t          hm[VL_PROFILE_MAX_HM];
    /* The copper legs: the workstation's traffic out to the VMC and back. */
    uint8_t          comm_count;
    vl_run_t         comms[VL_PROFILE_MAX_COMMS];

    /* One record for DTN_HEALTH_MONITOR_VL, management port out to copper, so
     * the DTN's own health monitor has a way off the box. The capture's 122
     * records do not carry it; on by default because both health monitors are
     * wanted during a run. */
    bool             dtn_health_monitor;
    /* Which copper port it leaves by. Per round, because it is a choice rather
     * than a property of the device: the capture sends it out of 33, and config1
     * sends it out of 32 so that the 100M link is left to the tap and the copper
     * leg that share it. */
    uint8_t          dtn_hm_port;

    /* Append VL 4419-4490 so the DTN keeps the whole management path the
     * reference configuration gives it. Off by default - it belongs to
     * RemoteConfigSender's end-system block, which answers on VL 4488. */
    bool             management;
} vl_profile_t;

/**
 * @brief The DTN's own management VLs, VL 4419-4490.
 *
 * Copied verbatim from the reference configuration: a broadcast from the
 * internal management port 34 to all 32 fibre ports, a pair per fibre port, and
 * both copper ports wired to it in both directions. The DTN's own health
 * monitor and the reply to a 0x52 status query travel on these.
 */
const dtn_vl_t *vl_profile_management(size_t *count);

/** The same records in their wire form, for comparing against the reference. */
size_t vl_profile_management_records(const uint8_t **raw);

/**
 * @brief The block written at address 0x46, verbatim from the reference.
 *
 * Sent as its own datagram before the switch table; the capture numbers its
 * first switch datagram seq 2, which is only possible if this one and the
 * end-system datagram precede it.
 */
const uint8_t *vl_profile_protocol_block(size_t *len);

/** The built-in profiles, in menu order. */
const vl_profile_t *vl_profile_all(size_t *count);

/**
 * @brief The captured configuration, as its own profile.
 *
 * Not in the menu. This is the configuration real hardware was seen to accept -
 * six fibre pairs 0-5 to 16-21, two taps from ports 15 and 31, the DTN's own
 * health monitor out of port 33 - and it is the only byte-exact evidence there
 * is that the encoder emits valid frames. The rounds change as the rig changes;
 * this does not, so the evidence survives them.
 */
const vl_profile_t *vl_profile_reference(void);

/** What a VL id is, for the routing display. */
typedef enum {
    VL_KIND_FIBRE = 0,   /**< a fibre link under test */
    VL_KIND_HM_TAP,      /**< the VMC's health monitor, fibre in, copper out */
    VL_KIND_DTN_HM,      /**< the DTN's own, management port out to copper */
    VL_KIND_COMM,        /**< a copper leg of the workstation - DTN - VMC path */
    VL_KIND_MANAGEMENT,  /**< VL 4419-4490 */
    VL_KIND_UNKNOWN
} vl_kind_t;

/**
 * @brief Which of those a VL id is, according to the profile that made it.
 *
 * Asked of the profile rather than guessed from the ports, because the ports no
 * longer say: a copper leg's return run is fibre in and copper out, which is
 * exactly what a tap looks like.
 */
vl_kind_t vl_profile_kind_of(const vl_profile_t *profile, uint16_t vl_id);

/** The label a copper leg was given, or NULL if that VL is not one. */
const char *vl_profile_comm_label(const vl_profile_t *profile, uint16_t vl_id);

/**
 * @brief Expand a profile into VL records, in the order the capture writes them.
 *
 * Forward links first, then the reverse links, then the health-monitor taps,
 * then the DTN's own health-monitor record, and finally the management VLs if
 * the profile asks for them. The result is
 * neither contiguous nor sorted by VL id - the capture is not either, which is
 * what proves the device reads the id out of each record instead of indexing
 * its table by position.
 *
 * @return record count, or -1 if the profile does not fit
 */
int vl_profile_expand(const vl_profile_t *profile, dtn_vl_t *out, size_t cap);

/** How many of @p records carry the ENABLE flag. */
size_t vl_profile_enabled_count(const dtn_vl_t *records, size_t count);

/**
 * @brief Reject a profile that cannot work on the hardware.
 *
 * Catches a port carrying both fibre traffic and health-monitor data in the
 * same round, duplicate VL ids, and VL ids in the reserved 0-2 range.
 *
 * @param reason filled with a human-readable explanation on failure
 * @return true when the profile is sound
 */
bool vl_profile_validate(const dtn_vl_t *records, size_t count,
                         char *reason, size_t reason_cap);

#endif /* VL_PROFILE_H */

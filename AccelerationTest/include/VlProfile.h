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
/* A round is 122 records; the DTN's management path adds 72 more. */
#define VL_PROFILE_MAX_RECORDS 256

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

typedef struct {
    const char      *name;
    const char      *description;
    uint8_t          group_count;
    vl_link_group_t  groups[VL_PROFILE_MAX_GROUPS];
    uint8_t          hm_count;
    vl_hm_t          hm[VL_PROFILE_MAX_HM];
    /* Append VL 4419-4490 so the DTN keeps its own health monitor and its
     * answer to a 0x52 query. The captured configuration leaves them out, so a
     * profile with this set is deliberately more than the capture. */
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
 * @brief Expand a profile into VL records, in the order the capture writes them.
 *
 * Forward links first, then the reverse links, then the health-monitor taps,
 * and finally the management VLs if the profile asks for them. The result is
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

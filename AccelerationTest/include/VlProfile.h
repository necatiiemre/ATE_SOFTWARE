/**
 * @file VlProfile.h
 * @brief The VL routing profiles the DTN is configured with.
 *
 * The unit on the other side of the fibre has 12 ports and the DTN has 32, so
 * the fibre links are covered in three rounds. Each round pairs six low ports
 * with six high ports in both directions and adds two health-monitor VLs out to
 * the 100M copper port.
 */

#ifndef VL_PROFILE_H
#define VL_PROFILE_H

#include "DtnConfig.h"

#define VL_PROFILE_MAX_LINKS   16
#define VL_PROFILE_MAX_GROUPS  2
#define VL_PROFILE_MAX_HM      4
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

/** A health-monitor VL: one DTN port streaming out to a copper port. */
typedef struct {
    uint16_t vl_id;
    uint8_t  src_port;
    uint8_t  dst_port;
    uint8_t  flags;      /**< flag nibble; see the note in VlProfile.c */
} vl_hm_t;

typedef struct {
    const char      *name;
    const char      *description;
    uint8_t          group_count;
    vl_link_group_t  groups[VL_PROFILE_MAX_GROUPS];
    uint8_t          hm_count;
    vl_hm_t          hm[VL_PROFILE_MAX_HM];
} vl_profile_t;

/** The built-in profiles, in menu order. */
const vl_profile_t *vl_profile_all(size_t *count);

/**
 * @brief Expand a profile into VL records, sorted by VL id.
 * @return record count, or -1 if the profile is inconsistent
 */
int vl_profile_expand(const vl_profile_t *profile, dtn_vl_t *out, size_t cap);

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

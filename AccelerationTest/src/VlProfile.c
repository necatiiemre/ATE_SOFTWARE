#include "VlProfile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Fibre links, six pairs per round in each direction. Round 3 overlaps round 2
 * on ports 10-11 and 26-27; together the three rounds cover fibre ports 0-31. */
static const vl_link_t C1_FWD[] = {{0,16},{1,17},{2,18},{3,19},{4,20},{5,21}};
static const vl_link_t C1_REV[] = {{16,0},{17,1},{18,2},{19,3},{20,4},{21,5}};
static const vl_link_t C2_FWD[] = {{6,22},{7,23},{8,24},{9,25},{10,26},{11,27}};
static const vl_link_t C2_REV[] = {{22,6},{23,7},{24,8},{25,9},{26,10},{27,11}};
static const vl_link_t C3_FWD[] = {{10,26},{11,27},{12,28},{13,29},{14,30},{15,31}};
static const vl_link_t C3_REV[] = {{26,10},{27,11},{28,12},{29,13},{30,14},{31,15}};

#define FLAGS_NORMAL (DTN_VL_FLAG_ENABLE | DTN_VL_FLAG_RESERVED)
/* The health-monitor taps carry flag nibble 0xD rather than the 0x9 every other
 * record uses, matching VL 4488 in the main ATE software. */
#define HM_FLAGS     (FLAGS_NORMAL | DTN_VL_FLAG_PRIORITY)

/* The DTN's own management path, byte-for-byte from the reference
 * configuration: the internal management port 34 wired to both copper
 * end-system ports, plus the copper ports to each other. Without these the
 * DTN's own health monitor and the reply to a 0x52 status query have nowhere
 * to go once our VL table replaces the device's. */
#define MGMT(id, src, dst, fl) \
    {.vl_id = (id), .src_port = (src), .dest_mask = 1ull << (dst), .lmax = 1518, \
     .lmin = 64, .jitter_ms = 0, .bag_word = DTN_BAG_WORD_1MS, .flags = (fl)}

static const dtn_vl_t g_management[] = {
    MGMT(4484, 32, 34, FLAGS_NORMAL),  /* copper 1G   -> management port     */
    MGMT(4485, 34, 32, FLAGS_NORMAL),  /* management  -> copper 1G           */
    MGMT(4486, 33, 34, FLAGS_NORMAL),  /* copper 100M -> management port     */
    MGMT(4487, 34, 33, FLAGS_NORMAL),  /* management  -> copper 100M         */
    MGMT(4488, 34, 33, HM_FLAGS),      /* status replies land on this VL     */
    MGMT(4489, 32, 33, FLAGS_NORMAL),
    MGMT(4490, 33, 32, FLAGS_NORMAL),
};

const dtn_vl_t *vl_profile_management(size_t *count)
{
    if (count)
        *count = sizeof g_management / sizeof g_management[0];
    return g_management;
}

#define ROUND(nm, desc, fwd, rev, hm0, hm1)                              \
    {                                                                    \
        .name = nm, .description = desc,                                 \
        .group_count = 2,                                                \
        .groups = {                                                      \
            {.vl_base = 1024, .vls_per_link = 10, .link_count = 6, .links = fwd}, \
            {.vl_base = 2024, .vls_per_link = 10, .link_count = 6, .links = rev}, \
        },                                                               \
        .hm_count = 2,                                                   \
        .hm = {                                                          \
            {.vl_id = 100, .src_port = hm0, .dst_port = 33, .flags = HM_FLAGS}, \
            {.vl_id = 101, .src_port = hm1, .dst_port = 33, .flags = HM_FLAGS}, \
        },                                                               \
        .management = true,                                              \
    }

static const vl_profile_t g_profiles[] = {
    ROUND("config1", "fibre ports 0-5 <-> 16-21,   HM from ports 15 and 31",
          C1_FWD, C1_REV, 15, 31),
    ROUND("config2", "fibre ports 6-11 <-> 22-27,  HM from ports 15 and 31",
          C2_FWD, C2_REV, 15, 31),
    /* Round 3 moves the health monitor: ports 15 and 31 carry fibre traffic here. */
    ROUND("config3", "fibre ports 10-15 <-> 26-31, HM from ports 0 and 16",
          C3_FWD, C3_REV, 0, 16),
};

const vl_profile_t *vl_profile_all(size_t *count)
{
    if (count)
        *count = sizeof g_profiles / sizeof g_profiles[0];
    return g_profiles;
}

static int compare_vl_id(const void *a, const void *b)
{
    uint16_t x = ((const dtn_vl_t *)a)->vl_id;
    uint16_t y = ((const dtn_vl_t *)b)->vl_id;
    return (x > y) - (x < y);
}

int vl_profile_expand(const vl_profile_t *profile, dtn_vl_t *out, size_t cap)
{
    size_t n = 0;

    for (uint8_t g = 0; g < profile->group_count; g++) {
        const vl_link_group_t *group = &profile->groups[g];
        uint16_t vl = group->vl_base;

        for (uint8_t l = 0; l < group->link_count; l++)
            for (uint16_t k = 0; k < group->vls_per_link; k++) {
                if (n == cap)
                    return -1;
                dtn_vl_init(&out[n++], vl++, group->links[l].src,
                            1ull << group->links[l].dst);
            }
    }

    for (uint8_t h = 0; h < profile->hm_count; h++) {
        if (n == cap)
            return -1;
        dtn_vl_init(&out[n], profile->hm[h].vl_id, profile->hm[h].src_port,
                    1ull << profile->hm[h].dst_port);
        out[n].flags = profile->hm[h].flags;
        n++;
    }

    if (profile->management) {
        size_t mgmt_count;
        const dtn_vl_t *mgmt = vl_profile_management(&mgmt_count);

        if (n + mgmt_count > cap)
            return -1;
        memcpy(out + n, mgmt, mgmt_count * sizeof *mgmt);
        n += mgmt_count;
    }

    qsort(out, n, sizeof out[0], compare_vl_id);
    return (int)n;
}

/* Ports 0-31 carry fibre traffic; 32 and 33 are the copper end-system ports and
 * 34 is the DTN's internal management port. */
#define FIBRE_PORT_MASK  0x00000000FFFFFFFFull
#define COPPER_PORT_MASK ((1ull << 32) | (1ull << 33))

bool vl_profile_validate(const dtn_vl_t *records, size_t count,
                         char *reason, size_t reason_cap)
{
    uint64_t fibre_ports = 0, hm_tap_ports = 0;

    for (size_t i = 0; i < count; i++) {
        const dtn_vl_t *r = &records[i];

        if (r->vl_id < 3) {
            snprintf(reason, reason_cap,
                     "VL %u is in the reserved range 0-2", r->vl_id);
            return false;
        }
        if (r->dest_mask == 0) {
            snprintf(reason, reason_cap, "VL %u has no destination port", r->vl_id);
            return false;
        }
        for (size_t j = i + 1; j < count; j++)
            if (records[j].vl_id == r->vl_id) {
                snprintf(reason, reason_cap, "VL %u appears twice", r->vl_id);
                return false;
            }

        /* VLs that start on a copper or management port are the DTN's own
         * management path and share no ports with the fibre links. */
        if (!(1ull << r->src_port & FIBRE_PORT_MASK))
            continue;

        if (r->dest_mask & COPPER_PORT_MASK)
            hm_tap_ports |= 1ull << r->src_port;   /* a fibre-side HM tap */
        else
            fibre_ports |= (1ull << r->src_port) | (r->dest_mask & FIBRE_PORT_MASK);
    }

    uint64_t clash = fibre_ports & hm_tap_ports;
    if (clash) {
        for (int p = 0; p < DTN_PORT_COUNT; p++)
            if (clash >> p & 1) {
                snprintf(reason, reason_cap,
                         "port %d carries both fibre traffic and health monitor", p);
                return false;
            }
    }
    return true;
}

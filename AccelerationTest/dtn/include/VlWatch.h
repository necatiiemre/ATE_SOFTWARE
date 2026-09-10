/**
 * @file VlWatch.h
 * @brief What actually arrived on the copper links, by VL.
 *
 * Until the health-monitor payloads are decoded, the question worth answering
 * is simpler and more useful: did the configuration take? A VL that the profile
 * routes to copper should show up on that copper link. One that never appears
 * is either not being generated or not being routed, and either way the run has
 * found something.
 *
 * The contents of the packets are ignored. Only the VL id in the destination
 * MAC, the link it arrived on, and how much of it there was.
 */

#ifndef VL_WATCH_H
#define VL_WATCH_H

#include "VlProfile.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VL_WATCH_MAX_ENTRIES 64
#define VL_WATCH_MAX_SIZES    4

typedef struct {
    uint16_t vl_id;
    uint8_t  dtn_port;                     /**< copper port it arrived on */
    bool     expected;                     /**< the profile routes it here */
    uint64_t packets;
    uint64_t bytes;
    uint64_t first_ms;
    uint64_t last_ms;
    uint16_t sizes[VL_WATCH_MAX_SIZES];    /**< distinct frame sizes seen */
    uint8_t  size_count;
} vl_sighting_t;

typedef struct {
    vl_sighting_t entries[VL_WATCH_MAX_ENTRIES];
    size_t        count;
    uint64_t      unclassified;   /**< frames that were not from the device */
    bool          overflow;       /**< more distinct VLs than the table holds */
} vl_watch_t;

/**
 * @brief Seed the table with the VLs the profile routes to copper.
 *
 * They start at zero packets, so a VL that never arrives is visible as a row
 * rather than as an absence.
 */
void vl_watch_init(vl_watch_t *watch, const dtn_vl_t *records, size_t count);

/** Record one frame that arrived on @p dtn_port. */
void vl_watch_saw(vl_watch_t *watch, uint8_t dtn_port, uint16_t vl_id, size_t len);

/** Count the frame as arriving but not recognised as coming from the device. */
void vl_watch_unclassified(vl_watch_t *watch);

/** How many expected VLs have been seen at least once. */
size_t vl_watch_expected_seen(const vl_watch_t *watch, size_t *expected_total);

/** Redraw the table in place. */
void vl_watch_render(const vl_watch_t *watch, uint64_t elapsed_s,
                     const char *profile_name, bool unit_alive,
                     unsigned interruptions);

/** Write a plain summary into the log at the end of a run. */
void vl_watch_log_summary(const vl_watch_t *watch);

#endif /* VL_WATCH_H */

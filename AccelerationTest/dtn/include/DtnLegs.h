/**
 * @file DtnLegs.h
 * @brief The copper legs: workstation through the DTN to the VMC and back.
 *
 * config1 routes each copper link out of a fibre port to the VMC and back again:
 *
 *   copper 32 --VL 3024-3083--> DTN --> fibre 8 --> VMC
 *   copper 32 <--VL 4024-4083-- DTN <-- fibre 8 <-- VMC
 *   copper 33 --VL 5024-5083--> DTN --> fibre 9 --> VMC
 *   copper 33 <--VL 6024-6083-- DTN <-- fibre 9 <-- VMC
 *
 * One flow, three devices. It proves the DTN forwards copper to fibre and back
 * *and* that the VMC at the far end is alive - where two separate tests would
 * each prove half of it and neither would catch a unit that answers on one
 * direction only.
 *
 * The legs are read out of the profile rather than written here: a leg is a run
 * of VLs whose source is a copper port, paired with the run that comes back the
 * other way between the same two ports. Change the routing in VlProfile.c and
 * this follows, which is the point of keeping the two in one place.
 *
 * Every payload is [8-byte sequence, big-endian][PRBS-31], the stream generated
 * once at the start of a run and read at an offset the sequence decides. So the
 * receiver regenerates what it should have got from the sequence alone and
 * remembers nothing between frames: a frame that arrives late still verifies,
 * and loss is the gap between the sequence that arrived and the one expected.
 *
 * Each VL has its own sequence, because each VL is its own AFDX stream with its
 * own BAG - and because the DTN may reorder between VLs but not within one.
 */

#ifndef DTN_LEGS_H
#define DTN_LEGS_H

#include "AppConfig.h"
#include "Prbs31.h"
#include "RawSocket.h"
#include "VlProfile.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** One leg per copper link. */
#define DTN_LEG_MAX 2

/** VLs per leg. config1 uses sixty. */
#define DTN_LEG_MAX_VLS 64

/**
 * @brief One leg's counters.
 *
 * Single writer each: the leg's sender for the tx_ fields, the receive loop for
 * the rest, and the display only reads. Plain integers for that reason - a torn
 * read would cost one redraw of one number and locking the path would cost more.
 */
typedef struct {
    uint64_t tx_frames;
    uint64_t tx_bytes;
    uint64_t tx_refused;    /**< the link would not take the frame - this end */

    uint64_t rx_frames;     /**< arrived on a return VL of this leg */
    uint64_t rx_bytes;
    uint64_t good;
    uint64_t bad;
    uint64_t splitmix_fail;   /**< the SplitMix zone did not regenerate */
    uint64_t crc_fail;        /**< the CRC over the sequence and that zone */
    uint64_t bit_errors;
    uint64_t lost;          /**< gaps in a VL's sequence */
    uint64_t wrong_length;  /**< a return VL carrying something else */
    uint64_t late;          /**< arrived behind what was expected, not loss */
} dtn_leg_stats_t;

typedef struct dtn_legs dtn_legs_t;

/**
 * @brief Read the legs out of a profile. NULL when the profile has none.
 *
 * The PRBS stream is borrowed, not owned, and must already be generated with
 * the stride dtn_legs_prbs_stride() gives for this configuration.
 */
dtn_legs_t *dtn_legs_create(const vl_profile_t *profile,
                            const dtn_leg_config_t *config,
                            const prbs31_cache_t *prbs,
                            volatile bool *stop);

void dtn_legs_destroy(dtn_legs_t *legs);

/** How many bytes of the stream one frame takes; the cache needs this as stride. */
size_t dtn_legs_prbs_stride(const dtn_leg_config_t *config);

/** Hand a leg the already-open socket for its copper port. */
bool dtn_legs_bind(dtn_legs_t *legs, uint8_t copper_port, raw_socket_t *sock);

/** One sender thread per leg. Every leg must be bound first. */
bool dtn_legs_start(dtn_legs_t *legs);

/** Join the senders. Idempotent. */
void dtn_legs_stop(dtn_legs_t *legs);

/** Stop sending without stopping the run - the unit went quiet and is coming back. */
void dtn_legs_pause(dtn_legs_t *legs, bool paused);

/** Zero the counters and every sequence, both ends of every VL. */
void dtn_legs_reset(dtn_legs_t *legs);

/**
 * @brief Offer a received frame to the legs.
 *
 * @return true when the frame was a leg's, so the caller does not go on to treat
 *         it as health-monitor traffic
 *
 * Takes the whole frame. The VL id comes out of the destination MAC, which is
 * where every frame on this rig carries it.
 */
bool dtn_legs_ingest(dtn_legs_t *legs, uint8_t copper_port,
                     const uint8_t *frame, size_t len);

uint8_t dtn_legs_count(const dtn_legs_t *legs);
const dtn_leg_stats_t *dtn_legs_stats(const dtn_legs_t *legs, uint8_t leg);

/** The live table: one row per leg. */
void dtn_legs_print_table(dtn_legs_t *legs);

/** Everything the log should keep at the end of a run. */
void dtn_legs_log_summary(const dtn_legs_t *legs);

/**
 * @brief What the configured rate works out to, and whether AFDX allows it.
 *
 * Every VL record declares BAG 1 ms, which is one frame per VL per millisecond.
 * A rate that asks for more than that is a rate the DTN will police, and policed
 * traffic looks exactly like loss - so it is worth saying before the run rather
 * than puzzling over the table during it.
 *
 * @param fps_per_leg  frames per second the whole leg will send
 * @param fps_per_vl   frames per second each VL will see
 * @return false when fps_per_vl exceeds what BAG allows
 */
bool dtn_legs_rate_plan(const dtn_legs_t *legs, double *fps_per_leg,
                        double *fps_per_vl);

#endif /* DTN_LEGS_H */

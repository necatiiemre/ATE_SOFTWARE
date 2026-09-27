/**
 * @file CmcDataPlane.h
 * @brief The CMC data plane: two links, twin frames, and what comes back.
 *
 * One sender and one receiver per DSM link. The sender walks the 104 VL ids in
 * turn and, for each, puts the *same* frame on both links - same VL id, same
 * sequence, same payload, same DTN_SEQ - differing only in the last byte of the
 * source MAC, which is what names the network. That is the point of the test:
 * two networks carrying identical traffic, so a difference in what comes back
 * is a difference in the unit rather than in what it was given.
 *
 * Each VL id has its own sequence, shared across the two networks, and it only
 * advances once the frame is actually on the wire. A link that refuses a frame
 * therefore retries the same sequence rather than skipping one, and the one
 * case where the pair comes apart - network A went out and B did not - is left
 * to show up as loss on B, which is what it is.
 *
 * The receiver checks each returned frame against what it can regenerate from
 * the sequence in it, and counts loss from the gap between the sequence it got
 * and the one it expected, per VL and per network. Nothing is kept between
 * frames but those two numbers.
 *
 * Health-monitor and MMMS traffic shares the links. Neither goes through
 * verification, so both are pulled out first, by VL id, and handed to the sink
 * the caller installs - see cmc_sink_t. The data plane knows nothing about
 * either beyond which VL ids belong to them.
 */

#ifndef CMC_DATA_PLANE_H
#define CMC_DATA_PLANE_H

#include "AppConfig.h"
#include "CmcPacket.h"
#include "RawSocket.h"

#include <stdbool.h>
#include <stdint.h>

/** The highest VL id a tracker is kept for; the reference's MAX_VL_ID. */
#define CMC_MAX_VL_ID 10700

/**
 * @brief Where the traffic that is not data-plane traffic goes.
 *
 * Installed by the caller, so the data plane does not have to know what a
 * health monitor is. A sink with NULL callbacks counts those frames and
 * discards them, which is what a test wants.
 */
typedef struct {
    bool     (*is_health_vl)(uint16_t vl_id);
    void     (*health)(uint16_t vl_id, const uint8_t *payload, uint16_t len);
    uint16_t mmms_vl_id;      /**< 0 when there is no MMMS channel */
    void     (*mmms)(const uint8_t *payload, uint16_t len);
} cmc_sink_t;

/**
 * @brief One network's counters.
 *
 * Every field has exactly one writer - the sender for the tx_ ones, that
 * network's receiver for the rest - and the dashboard only reads them, which is
 * the arrangement the reference's own listener uses. So they are plain
 * integers: a torn read would cost one redraw of one number, and locking the
 * hot path to prevent that would cost more than it is worth.
 */
typedef struct {
    uint64_t frames;          /**< everything that arrived on the link */
    uint64_t frame_bytes;
    uint64_t tx_pkts;
    uint64_t tx_bytes;
    uint64_t tx_refused;      /**< the link would not take the frame */
    int      tx_errno;        /**< errno of the first refusal; 0 if none */

    uint64_t total_rx_pkts;   /**< frames that reached verification */
    uint64_t good;
    uint64_t bad;
    uint64_t splitmix_fail;
    uint64_t crc32_fail;
    uint64_t xor_fail;
    uint64_t bit_errors;
    uint64_t lost;

    uint64_t short_pkts;      /**< too short to be a data-plane frame */
    uint64_t health_frames;   /**< went to the health monitor instead */
    uint64_t mmms_frames;
    uint64_t other_vl;        /**< a VL id that is none of the above */
    uint16_t last_other_vl;
} cmc_net_stats_t;

typedef struct cmc_data_plane cmc_data_plane_t;

/** Allocate. The PRBS cache must already be generated; it is borrowed, not owned. */
cmc_data_plane_t *cmc_data_plane_create(const cmc_config_t *config,
                                        const prbs31_cache_t *prbs,
                                        const cmc_sink_t *sink,
                                        volatile bool *stop);

void cmc_data_plane_destroy(cmc_data_plane_t *dp);

/** Open the two DSM links. Prints what it opened, and why not when it fails. */
bool cmc_data_plane_open(cmc_data_plane_t *dp);

/** Receivers first, then the sender - the order the reference starts them in. */
bool cmc_data_plane_start(cmc_data_plane_t *dp);

/** Join the threads. Idempotent. */
void cmc_data_plane_stop(cmc_data_plane_t *dp);

/**
 * @brief Stop sending data-plane traffic, without stopping the receivers.
 *
 * What the first Ctrl+C does: the MMMS handover needs an otherwise idle wire,
 * and the receivers have to stay up to collect the answer.
 */
void cmc_data_plane_pause_tx(cmc_data_plane_t *dp, bool paused);

/** Zero every counter and every sequence tracker - the warm-up boundary. */
void cmc_data_plane_reset(cmc_data_plane_t *dp);

const cmc_net_stats_t *cmc_data_plane_stats(const cmc_data_plane_t *dp, uint8_t net);

/**
 * @brief One network's open link.
 *
 * The data plane owns the sockets, and the MMMS handover needs to put one frame
 * on network A's. Rather than open a second socket on the same interface, it
 * borrows this one - the traffic is paused by then, so nothing else is using it.
 * NULL before the links are open.
 */
struct raw_socket;
raw_socket_t *cmc_data_plane_link(cmc_data_plane_t *dp, uint8_t net);

/** The rate each network is paced at, in Gbps - half the configured target. */
double cmc_data_plane_net_gbps(const cmc_data_plane_t *dp);

/**
 * @brief Take one received frame apart, as the receiver does.
 *
 * Exposed so a test can drive the whole classification and accounting path -
 * the health-monitor branch, the short-frame filter, the loss arithmetic and
 * the verification - without a wire. Returns true when the frame became a
 * verified data-plane packet.
 */
bool cmc_data_plane_ingest(cmc_data_plane_t *dp, uint8_t net,
                           const uint8_t *frame, size_t len);

#endif /* CMC_DATA_PLANE_H */

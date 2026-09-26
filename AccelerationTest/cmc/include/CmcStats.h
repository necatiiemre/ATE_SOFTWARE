/**
 * @file CmcStats.h
 * @brief The loss tables, one per network.
 *
 * The same table the reference prints, with the same columns in the same order
 * and the same box around it: whoever reads this on the rig has read the other
 * one, and a column that has moved is a column that gets misread.
 *
 * Two changes, both forced by there being no DPDK underneath:
 *
 *   - the "Server Port" column holds the interface name rather than a DPDK port
 *     number. The reference prints 0 in every row, because every flow is on the
 *     one fibre port; here the interface *is* which port a flow is on, and it is
 *     the thing an operator needs when a row goes quiet. Same width, same place.
 *   - the packet and byte counts are the ones this program counted, not a NIC's
 *     hardware counters. On the receive side that means everything that arrived
 *     on the link, which is what the hardware queue counter meant too.
 *
 * Where the reference prints these first and the health monitor after, this
 * prints them last - see CmcTest.c. That is the one thing about the output that
 * was asked to be different.
 */

#ifndef CMC_STATS_H
#define CMC_STATS_H

#include "AppConfig.h"
#include "CmcDataPlane.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief The per-second baseline the Gbps columns are a delta against.
 *
 * Held by the caller rather than in a static, so the warm-up reset can zero it
 * at the same moment it zeros the counters - otherwise the first table of the
 * test window shows a second's rate computed against the whole warm-up.
 */
typedef struct {
    uint64_t prev_rx_bytes[APP_MAX_CMC_NETS];
    uint64_t prev_tx_bytes[APP_MAX_CMC_NETS];
} cmc_stats_view_t;

void cmc_stats_view_reset(cmc_stats_view_t *view);

/** The banner: which phase, and how far into it. */
void cmc_stats_print_banner(const cmc_config_t *config, bool warmup_complete,
                            unsigned elapsed_s);

/** One network's table, banner excluded. */
void cmc_stats_print_net(cmc_stats_view_t *view, const cmc_data_plane_t *dp,
                         const cmc_config_t *config, uint8_t net);

/** Every network's table, each under its own heading. */
void cmc_stats_print_all(cmc_stats_view_t *view, const cmc_data_plane_t *dp,
                         const cmc_config_t *config);

/** The warnings block: what is not zero that should be. */
void cmc_stats_print_warnings(const cmc_data_plane_t *dp, const cmc_config_t *config);

/** Everything the log should keep about a network at the end of a run. */
void cmc_stats_log_net(const cmc_data_plane_t *dp, const cmc_config_t *config,
                       uint8_t net);

#endif /* CMC_STATS_H */

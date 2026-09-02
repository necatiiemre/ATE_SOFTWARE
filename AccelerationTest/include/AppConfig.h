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
 * @brief Whether to write a contiguous VL table with the unused ids disabled.
 *
 * On by default. The reference configuration's ids run from 3 to 4490 with no
 * gap, which suggests the device indexes its table rather than searching it -
 * a sparse table would then leave every VL above the record count unreachable.
 * Turn it off with --sparse-table to send only the profile's own VLs.
 */
bool app_config_dense_table(void);
void app_config_set_dense_table(bool dense);

/** Which copper link the configuration frames go out of. */
const copper_link_t *app_config_config_link(void);

#endif /* APP_CONFIG_H */

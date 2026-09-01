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

/** Which copper link the configuration frames go out of. */
const copper_link_t *app_config_config_link(void);

#endif /* APP_CONFIG_H */

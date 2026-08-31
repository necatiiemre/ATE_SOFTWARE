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
#include <stdint.h>

/** The copper links from the workstation to the DTN end-system ports. */
typedef struct {
    const char *config_interface;   /**< configuration frames go out here */
    const char *monitor_interface;  /**< the health-monitor stream arrives here */
    unsigned    frame_gap_ms;       /**< pause between configuration frames */
    unsigned    device_ready_timeout_s; /**< how long to wait for the unit to come up */
    unsigned    status_reply_timeout_ms;
    unsigned    heartbeat_timeout_ms;   /**< silence that counts as "unit lost" */
} net_config_t;

const net_config_t *app_config_net(void);

#endif /* APP_CONFIG_H */

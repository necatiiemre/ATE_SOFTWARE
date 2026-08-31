#include "AppConfig.h"

/* Interface names from the existing rig. The health monitor arrives on the
 * 100M link, which is where the DTN's port 33 lands - the port the profiles
 * route their health-monitor VLs to. */
static const net_config_t g_net = {
    .config_interface        = "eno12409",
    .monitor_interface       = "eno12409",
    .frame_gap_ms            = 4,
    .device_ready_timeout_s  = 90,
    .status_reply_timeout_ms = 2000,
    .heartbeat_timeout_ms    = 5000,
};

const net_config_t *app_config_net(void)
{
    return &g_net;
}

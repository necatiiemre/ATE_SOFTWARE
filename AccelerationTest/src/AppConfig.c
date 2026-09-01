#include "AppConfig.h"

#include <stddef.h>

/* Interface names from the existing rig; they will change on the workstation.
 * The DTN port numbers will not - they are what the VL table routes to, so the
 * application derives which link to listen on from the profile rather than
 * from a hard-coded interface name. */
static const copper_link_t g_copper[APP_MAX_COPPER_LINKS] = {
    {32, "eno12399", "1G"},
    {33, "eno12409", "100M"},
};

static const timing_config_t g_timing = {
    .frame_gap_ms            = 4,
    .device_ready_timeout_s  = 90,
    .status_reply_timeout_ms = 2000,
    .heartbeat_timeout_ms    = 5000,
    .display_interval_ms     = 1000,
};

const copper_link_t *app_config_copper(size_t *count)
{
    if (count)
        *count = APP_MAX_COPPER_LINKS;
    return g_copper;
}

const char *app_config_iface_for_port(uint8_t dtn_port)
{
    for (size_t i = 0; i < APP_MAX_COPPER_LINKS; i++)
        if (g_copper[i].dtn_port == dtn_port)
            return g_copper[i].iface;
    return NULL;
}

const timing_config_t *app_config_timing(void)
{
    return &g_timing;
}

const copper_link_t *app_config_config_link(void)
{
    /* The 100M link is the proven management path: it is where the main ATE
     * software polls the device and where the reference status query goes. */
    return &g_copper[1];
}

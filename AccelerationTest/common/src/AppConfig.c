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

static bool g_management_vls;

bool app_config_management_vls(void) { return g_management_vls; }
void app_config_set_management_vls(bool keep) { g_management_vls = keep; }

static bool g_all_ports;

bool app_config_all_ports(void) { return g_all_ports; }
void app_config_set_all_ports(bool all) { g_all_ports = all; }

/* The VMC rig. Interface name and every VL id in one place - see AppConfig.h.
 * The ids come from dpdk_vmc/include/health_monitor/health_monitor.h. */
static const vmc_config_t g_vmc = {
    /* First link is FLCS, second is VS - the rig is wired that way. */
    .links = {
        {"eno12399", 0},
        {"eno12409", 1},
    },
    .link_count         = 2,

    .flcs_cpu_usage     = 0x0009,
    .vs_cpu_usage       = 0x0010,
    .flcs_pbit_request  = 0x000c,
    .vs_pbit_request    = 0x000f,
    .flcs_pbit_response = 0x000a,
    .vs_pbit_response   = 0x000d,
    .flcs_cbit          = 0x000b,
    .vs_cbit            = 0x000e,
    .flcs_counters      = 100,
    .vs_counters        = 101,

    .msg_dtn_es         = 2,
    .msg_dtn_sw         = 3,
    .msg_bm_engineering = 5,
    .msg_bm_flag        = 6,
    .msg_pbit_response  = 100,
    .msg_pbit_request   = 50,
    .pbit_resend_interval_s = 2,

    .net_type_es        = 0,
    .net_type_sw_es     = 1,
};

const vmc_config_t *app_config_vmc(void)
{
    return &g_vmc;
}

const copper_link_t *app_config_config_link(void)
{
    /* The 100M link is the proven management path: it is where the main ATE
     * software polls the device and where the reference status query goes. */
    return &g_copper[1];
}

#include "CumulusSetup.h"

#include "FibreMap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CMD_BUF 1024

static bool run(const char *description, const char *command)
{
    int status = system(command);

    if (status != 0) {
        fprintf(stderr, "[cumulus] %s failed (exit %d)\n", description, status);
        return false;
    }
    return true;
}

bool cumulus_breakout_name(uint8_t dtn_port, char *out, size_t cap)
{
    if (dtn_port >= FIBRE_DTN_PORT_COUNT)
        return false;
    return snprintf(out, cap, "swp%us%u", 25u + dtn_port / 4u, dtn_port % 4u) > 0;
}

static bool ssh_execute(const char *remote_command, bool use_sudo, const char *description)
{
    char cmd[CMD_BUF];

    snprintf(cmd, sizeof cmd,
             "sshpass -p '%s' ssh -o StrictHostKeyChecking=no -o ConnectTimeout=10 "
             "%s@%s \"%s%s\" > /dev/null 2>&1",
             CUMULUS_PASSWORD, CUMULUS_USER, CUMULUS_HOST,
             use_sudo ? "echo '" CUMULUS_PASSWORD "' | sudo -S " : "",
             remote_command);
    return run(description, cmd);
}

bool cumulus_reachable(void)
{
    return ssh_execute("true", false, "connection test");
}

bool cumulus_deploy_interfaces(const char *interfaces_path)
{
    char cmd[CMD_BUF];

    printf("[cumulus] copying %s to the switch\n", interfaces_path);
    snprintf(cmd, sizeof cmd,
             "sshpass -p '%s' scp -o StrictHostKeyChecking=no -o ConnectTimeout=10 "
             "%s %s@%s:/tmp/interfaces > /dev/null 2>&1",
             CUMULUS_PASSWORD, interfaces_path, CUMULUS_USER, CUMULUS_HOST);
    if (!run("interfaces copy", cmd))
        return false;

    if (!ssh_execute("mv /tmp/interfaces /etc/network/interfaces", true, "interfaces install"))
        return false;

    printf("[cumulus] reloading interfaces\n");
    /* ifreload often reports non-zero while still applying the change, so a
     * failure here is worth saying out loud but not worth stopping for. */
    if (!ssh_execute("ifreload -a", true, "ifreload"))
        fprintf(stderr, "[cumulus] ifreload complained; the change may still have applied\n");
    return true;
}

bool cumulus_configure_vlans(void)
{
    printf("[cumulus] setting egress VLANs on %d breakout ports\n", FIBRE_DTN_PORT_COUNT);

    for (uint8_t port = 0; port < FIBRE_DTN_PORT_COUNT; port++) {
        char swp[16], remote[128], description[64];

        if (!cumulus_breakout_name(port, swp, sizeof swp))
            return false;
        snprintf(remote, sizeof remote, "bridge vlan add dev %s vid %u untagged",
                 swp, fibre_tx_vlan(port));
        snprintf(description, sizeof description, "VLAN %u on %s",
                 fibre_tx_vlan(port), swp);
        if (!ssh_execute(remote, true, description))
            return false;
    }
    return true;
}

bool cumulus_setup(const char *interfaces_path)
{
    if (!cumulus_reachable()) {
        fprintf(stderr, "[cumulus] %s is not answering - is sshpass installed and the "
                        "switch reachable?\n", CUMULUS_HOST);
        return false;
    }
    if (!cumulus_deploy_interfaces(interfaces_path))
        return false;
    return cumulus_configure_vlans();
}

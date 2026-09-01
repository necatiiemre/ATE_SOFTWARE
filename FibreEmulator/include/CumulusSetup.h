/**
 * @file CumulusSetup.h
 * @brief Bringing up the Mellanox switch between the server and the DTN.
 *
 * The fibre path is not point to point: a Mellanox switch fans the server's
 * eight ports out to the DTN's 32, and it decides where a frame goes purely
 * from its VLAN tag. Without it configured, nothing this application sends
 * reaches the DTN at all.
 *
 * Setup is the same two steps the rest of the rig uses - push an interfaces
 * file and reload, then make every breakout port egress its VLAN untagged -
 * driven over ssh with sshpass, which is what the switch is set up for.
 */

#ifndef CUMULUS_SETUP_H
#define CUMULUS_SETUP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CUMULUS_HOST     "10.1.33.3"
#define CUMULUS_USER     "cumulus"
#define CUMULUS_PASSWORD "%T86Ovk7RCH%h@CC"

/** Breakout port carrying a DTN fibre port, e.g. port 0 -> "swp25s0". */
bool cumulus_breakout_name(uint8_t dtn_port, char *out, size_t cap);

/** True if the switch answers over ssh. */
bool cumulus_reachable(void);

/**
 * @brief Push the interfaces file and reload it.
 * @param interfaces_path local file to copy to /etc/network/interfaces
 */
bool cumulus_deploy_interfaces(const char *interfaces_path);

/**
 * @brief Make each of the 32 breakout ports egress its VLAN untagged.
 *
 * VLAN 97 + dtn_port on swp(25 + dtn_port/4)s(dtn_port%4). The rule reproduces
 * the rig's own list for all 32 ports.
 */
bool cumulus_configure_vlans(void);

/** Reachability, interfaces, VLANs - the whole sequence. */
bool cumulus_setup(const char *interfaces_path);

#endif /* CUMULUS_SETUP_H */

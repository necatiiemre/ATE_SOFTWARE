#ifndef VMC_STANDALONE_CUMULUS_CONFIG_H
#define VMC_STANDALONE_CUMULUS_CONFIG_H

// ============================================================================
// Cumulus switch connection settings.
// Must match MainSoftware (SSHDeployer.cpp -> g_ssh_deployer_cumulus).
// ============================================================================
#define CUMULUS_HOST      "10.1.33.3"
#define CUMULUS_USER      "cumulus"
#define CUMULUS_PASSWORD  "%T86Ovk7RCH%h@CC"

// Remote location of the interfaces file on the switch.
#define CUMULUS_REMOTE_INTERFACES_PATH "/etc/network/interfaces"

// The interfaces file location depends on the selected unit: see
// UnitConfig.h (CumulusInterfaces/DPM/interfaces, CumulusInterfaces/DSM/interfaces).
// Both are part of this project; CMake copies them next to the executable.

#endif // VMC_STANDALONE_CUMULUS_CONFIG_H

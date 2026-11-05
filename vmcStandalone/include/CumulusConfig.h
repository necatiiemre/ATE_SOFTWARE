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

// Relative location of the VMC interfaces file. The file is part of this
// project (VmcStandalone/CumulusInterfaces/VMC/interfaces); CMake copies it
// next to the executable (build/bin/). Nothing outside VmcStandalone/ is used.
#define VMC_INTERFACES_RELATIVE_PATH "CumulusInterfaces/VMC/interfaces"

#endif // VMC_STANDALONE_CUMULUS_CONFIG_H

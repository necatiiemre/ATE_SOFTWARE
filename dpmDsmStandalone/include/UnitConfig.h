#ifndef DPM_DSM_STANDALONE_UNIT_CONFIG_H
#define DPM_DSM_STANDALONE_UNIT_CONFIG_H

// ============================================================================
// DPM / DSM selection. Everything that differs between the two units on the
// PC side lives here; the unit is chosen at start-up (menu or --unit).
//
// Both units use the same Cumulus configuration (derived from the VMC file):
//   A line = swp9s0, VLAN 97 (returns untagged -> PVID 225)
//   B line = swp9s1, VLAN 98 (returns untagged -> PVID 226)
// The PTP sequence differs and is selected inside dpm_dsm_test (--unit).
// ============================================================================

struct UnitConfig {
    const char *name;            // "DPM" / "DSM": log folder, log header, messages
    const char *testArg;         // dpm_dsm_test --unit <testArg>
    const char *interfacesPath;  // Cumulus interfaces file, relative to the exe dir
};

static const UnitConfig UNIT_DPM = { "DPM", "dpm", "CumulusInterfaces/DPM/interfaces" };
static const UnitConfig UNIT_DSM = { "DSM", "dsm", "CumulusInterfaces/DSM/interfaces" };

#endif // DPM_DSM_STANDALONE_UNIT_CONFIG_H

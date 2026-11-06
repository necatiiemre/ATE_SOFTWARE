#ifndef VMC_STANDALONE_CUMULUS_VMC_H
#define VMC_STANDALONE_CUMULUS_VMC_H

#include <string>
#include <vector>

#include "SshClient.h"

/**
 * @brief One row of "bridge vlan show" state.
 *
 * Used both as "expected" (what the VMC sequence wants the switch to look
 * like) and "actual" (what is parsed back from the switch) so the two can be
 * diffed before issuing any configuration commands.
 */
struct BridgeVlanEntry {
    std::string interface;
    int         vlan_id = 0;
    bool        egress_untagged = false;   // "Egress Untagged" flag
    bool        pvid = false;              // "PVID" flag (ingress untagged)

    BridgeVlanEntry() = default;
    BridgeVlanEntry(std::string iface, int vid, bool untagged, bool is_pvid = false)
        : interface(std::move(iface)), vlan_id(vid),
          egress_untagged(untagged), pvid(is_pvid) {}
};

/**
 * @brief Cumulus switch configuration for the DPM / DSM unit (A/B lines).
 *
 * Copy of vmcStandalone's class; the expected VLANs are the A/B lines
 * (swp9s0 / swp9s1, like MainSoftware's CMC) and messages carry the unit
 * name. Originally extracted from MainSoftware's CumulusHelper (VMC path)
 * (Vmc::configureSequence -> deployNetworkInterfaces + configureSequenceVmc).
 * DTN / CMC / HSN sequences are intentionally left out.
 *
 * Full flow (configure()):
 *   1. SSH connection test
 *   2. Copy the unit's interfaces file -> /etc/network/interfaces (sudo) + ifreload -a
 *   3. Read 'bridge -j vlan show'; if both A/B entries are present, stop
 *   4. Otherwise apply all 'bridge vlan add ... untagged' in one SSH batch
 */
class CumulusVmc {
public:
    CumulusVmc(SshClient& ssh, std::string unitName);

    /** @brief Full VMC Cumulus configuration (steps 1-4 above). */
    bool configure(const std::string& local_interfaces_path);

    /** @brief Test SSH connection to the switch. */
    bool connect();

    /** @brief scp interfaces file to the switch and run 'ifreload -a'. */
    bool deployNetworkInterfaces(const std::string& local_interfaces_path);

    /** @brief Idempotent VMC bridge-VLAN configuration (steps 3-4 above). */
    bool configureVlans();

    /**
     * @brief Read the switch state back and check every VMC entry is present.
     *        Used as a post-configuration verification.
     */
    bool verifyVlans();

    /**
     * @brief The expected bridge-vlan state of the A/B lines (DPM and DSM):
     *   swp9s0 -> vid 97 untagged  (A line, returns via PVID 225)
     *   swp9s1 -> vid 98 untagged  (B line, returns via PVID 226)
     */
    static std::vector<BridgeVlanEntry> expectedVlans();

    /**
     * @brief Fetch and parse 'bridge -j vlan show' (one SSH round-trip).
     * @param raw optional: the unparsed JSON as returned by the switch
     */
    bool fetchBridgeVlanState(std::vector<BridgeVlanEntry>& out, std::string* raw = nullptr);

    /**
     * @brief Entries of 'expected' that are missing from 'actual' (matching
     *        interface, vlan and egress-untagged flag). Extra entries in
     *        'actual' are ignored.
     */
    static std::vector<BridgeVlanEntry> missingVlans(
        const std::vector<BridgeVlanEntry>& expected,
        const std::vector<BridgeVlanEntry>& actual);

    /** @brief Run many commands in a single SSH session ('set -e'). */
    bool executeBatch(const std::vector<std::string>& commands, bool use_sudo = false);

private:
    SshClient& m_ssh;
    std::string m_unit;          // "DPM" / "DSM", used in messages

    static std::string logPrefix() { return "[Cumulus]"; }
};

#endif // VMC_STANDALONE_CUMULUS_VMC_H

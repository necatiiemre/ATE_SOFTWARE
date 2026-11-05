#include "CumulusVmc.h"
#include "CumulusConfig.h"
#include "Log.h"
#include "Logger.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

// ====================================================================
// Local helpers
// ====================================================================

namespace {

// Parse the JSON produced by `bridge -j vlan show` into a flat entry list.
// Small, predictable format -> a hand-rolled scanner avoids a JSON
// dependency. One entry per (interface, vlan) pair. Same parser as
// MainSoftware's CumulusHelper.
bool parseBridgeVlanJson(const std::string& json,
                         std::vector<BridgeVlanEntry>& out)
{
    size_t pos = 0;
    const std::string k_ifname = "\"ifname\":\"";
    const std::string k_vlans  = "\"vlans\":[";
    const std::string k_vlan   = "\"vlan\":";
    const std::string k_untag  = "\"Egress Untagged\"";
    const std::string k_pvid   = "\"PVID\"";

    while (pos < json.size()) {
        size_t name_key = json.find(k_ifname, pos);
        if (name_key == std::string::npos) break;

        size_t name_start = name_key + k_ifname.size();
        size_t name_end = json.find('"', name_start);
        if (name_end == std::string::npos) break;
        std::string iface = json.substr(name_start, name_end - name_start);

        size_t vlans_key = json.find(k_vlans, name_end);
        if (vlans_key == std::string::npos) { pos = name_end + 1; continue; }
        size_t vlans_start = vlans_key + k_vlans.size();

        // Matching ']' for this vlans array ("flags":[...] nests inside).
        int depth = 1;
        size_t p = vlans_start;
        while (p < json.size() && depth > 0) {
            char c = json[p];
            if (c == '[') depth++;
            else if (c == ']') { depth--; if (depth == 0) break; }
            ++p;
        }
        if (depth != 0) break;
        size_t vlans_end = p;

        // Walk the vlan objects one by one.
        size_t vp = vlans_start;
        while (vp < vlans_end) {
            size_t obj_start = json.find('{', vp);
            if (obj_start == std::string::npos || obj_start >= vlans_end) break;

            int obj_depth = 1;
            size_t op = obj_start + 1;
            while (op < vlans_end && obj_depth > 0) {
                char c = json[op];
                if (c == '{') obj_depth++;
                else if (c == '}') { obj_depth--; if (obj_depth == 0) break; }
                ++op;
            }
            if (obj_depth != 0) break;
            size_t obj_end = op;

            size_t vlan_key = json.find(k_vlan, obj_start);
            if (vlan_key == std::string::npos || vlan_key > obj_end) {
                vp = obj_end + 1;
                continue;
            }
            size_t num_start = vlan_key + k_vlan.size();
            size_t num_end = num_start;
            while (num_end < obj_end &&
                   std::isdigit(static_cast<unsigned char>(json[num_end]))) {
                ++num_end;
            }
            if (num_end == num_start) { vp = obj_end + 1; continue; }
            int vid = std::stoi(json.substr(num_start, num_end - num_start));

            std::string obj_slice = json.substr(obj_start, obj_end - obj_start + 1);
            bool untagged = obj_slice.find(k_untag) != std::string::npos;
            bool pvid     = obj_slice.find(k_pvid)  != std::string::npos;

            out.emplace_back(iface, vid, untagged, pvid);
            vp = obj_end + 1;
        }

        pos = vlans_end + 1;
    }

    return !out.empty();
}

// Each group is a base port name ("swp9") paired with the VLAN id of its
// s0 sub-port; s1..s3 get id+1..id+3. All entries are egress-untagged.
std::vector<BridgeVlanEntry> expandGroups(
    const std::vector<std::pair<std::string, int>>& groups)
{
    std::vector<BridgeVlanEntry> out;
    out.reserve(groups.size() * 4);
    for (const auto& g : groups) {
        for (int i = 0; i < 4; ++i) {
            out.emplace_back(g.first + "s" + std::to_string(i),
                             g.second + i,
                             /*egress_untagged=*/true,
                             /*pvid=*/false);
        }
    }
    return out;
}

// Save a copy of something into this run's log directory (no-op without a log).
void saveToLogDir(const std::string& name, const std::string& content)
{
    if (!logx::active()) return;
    const std::string path = (std::filesystem::path(logx::dir()) / name).string();
    std::ofstream f(path);
    if (f) {
        f << content;
        FILE_LOG("[Cumulus] Saved " << path);
    }
}

void printBanner(const std::string& text)
{
    std::cout << "\n========================================" << std::endl;
    std::cout << text << std::endl;
    std::cout << "========================================" << std::endl;
}

} // namespace

// ====================================================================
// CumulusVmc
// ====================================================================

CumulusVmc::CumulusVmc(SshClient& ssh)
    : m_ssh(ssh)
{
}

bool CumulusVmc::connect()
{
    return m_ssh.testConnection();
}

bool CumulusVmc::configure(const std::string& local_interfaces_path)
{
    if (!deployNetworkInterfaces(local_interfaces_path)) {
        ErrorPrinter::error("CUMULUS", "VMC: Failed to deploy network configuration!");
        return false;
    }

    if (!configureVlans()) {
        ErrorPrinter::error("CUMULUS", "VMC: Cumulus VLAN configuration failed!");
        return false;
    }

    if (!verifyVlans()) {
        ErrorPrinter::error("CUMULUS", "VMC: Cumulus VLAN verification failed!");
        return false;
    }

    return true;
}

bool CumulusVmc::deployNetworkInterfaces(const std::string& local_interfaces_path)
{
    printBanner(logPrefix() + " Deploying VMC Network Interfaces");

    if (!std::filesystem::exists(local_interfaces_path)) {
        std::cerr << logPrefix() << " Deploy failed: interfaces file not found: "
                  << local_interfaces_path << std::endl;
        return false;
    }
    std::cout << logPrefix() << " Using interfaces file: " << local_interfaces_path << std::endl;

    // Keep an exact copy of what is sent to the switch with this run's log.
    if (logx::active()) {
        std::error_code ec;
        const auto dst = std::filesystem::path(logx::dir()) / "cumulus_interfaces_deployed";
        std::filesystem::copy_file(local_interfaces_path, dst,
                                   std::filesystem::copy_options::overwrite_existing, ec);
        FILE_LOG(logPrefix() << " Interfaces copy for the log: " << dst.string()
                             << (ec ? " FAILED: " + ec.message() : ""));
    }

    std::cout << logPrefix() << " [1/3] Testing connection..." << std::endl;
    if (!connect()) {
        std::cerr << logPrefix() << " Deploy failed: Cannot connect to switch" << std::endl;
        return false;
    }

    std::cout << logPrefix() << " [2/3] Copying interfaces file -> "
              << CUMULUS_REMOTE_INTERFACES_PATH << std::endl;
    if (!m_ssh.copyFileToPath(local_interfaces_path, CUMULUS_REMOTE_INTERFACES_PATH,
                              /*use_sudo=*/true)) {
        std::cerr << logPrefix() << " Deploy failed: Cannot copy interfaces file" << std::endl;
        return false;
    }

    std::cout << logPrefix() << " [3/3] Reloading network interfaces (ifreload -a)..." << std::endl;
    if (!m_ssh.execute("ifreload -a", nullptr, /*use_sudo=*/true)) {
        // ifreload may return non-zero but still apply the changes
        // (same tolerance as MainSoftware).
        ErrorPrinter::warn("CUMULUS",
            "ifreload -a returned non-zero exit code (changes may still be applied)");
    }

    std::cout << logPrefix() << " Network interfaces deployed successfully." << std::endl;
    return true;
}

bool CumulusVmc::configureVlans()
{
    printBanner(logPrefix() + " Starting VLAN Configuration Sequence (VMC)");

    if (!connect()) {
        std::cerr << logPrefix() << " Configuration failed: Cannot connect to switch" << std::endl;
        return false;
    }

    // Idempotent skip: if the switch already has every VMC entry, do nothing.
    const auto expected = expectedVlans();
    std::vector<BridgeVlanEntry> actual;
    if (fetchBridgeVlanState(actual) && missingVlans(expected, actual).empty()) {
        ErrorPrinter::info("CUMULUS",
            "VMC VLAN state already matches expected (" +
            std::to_string(expected.size()) + " entries), skipping configuration");
        return true;
    }

    // Apply all entries in a single SSH session.
    std::vector<std::string> batch;
    batch.reserve(expected.size());
    for (const auto& e : expected) {
        std::string cmd = "bridge vlan add dev " + e.interface +
                          " vid " + std::to_string(e.vlan_id);
        if (e.egress_untagged) cmd += " untagged";
        batch.push_back(cmd);
        VERBOSE_LOG(logPrefix() << "   " << cmd);
    }

    if (!executeBatch(batch, /*use_sudo=*/true)) {
        std::cerr << logPrefix() << " VMC batch VLAN apply failed" << std::endl;
        return false;
    }

    std::cout << logPrefix() << " VMC VLAN batch applied ("
              << expected.size() << " entries)" << std::endl;
    return true;
}

bool CumulusVmc::verifyVlans()
{
    std::cout << logPrefix() << " Verifying VMC bridge VLAN state..." << std::endl;

    std::vector<BridgeVlanEntry> actual;
    std::string raw;
    if (!fetchBridgeVlanState(actual, &raw)) {
        std::cerr << logPrefix() << " Verification failed: cannot read 'bridge -j vlan show'"
                  << std::endl;
        return false;
    }

    // Final switch state goes with the log, plus a readable table of the
    // VMC ports in the log file.
    saveToLogDir("cumulus_bridge_vlan.json", raw);
    {
        std::ostringstream t;
        t << logPrefix() << " Bridge VLAN state of VMC ports (swp9-swp12):";
        for (const auto& a : actual) {
            if (a.interface.rfind("swp9s", 0) != 0 && a.interface.rfind("swp10s", 0) != 0 &&
                a.interface.rfind("swp11s", 0) != 0 && a.interface.rfind("swp12s", 0) != 0)
                continue;
            t << "\n    " << a.interface << "  vid " << a.vlan_id
              << (a.pvid ? "  PVID" : "") << (a.egress_untagged ? "  Egress-Untagged" : "");
        }
        FILE_LOG(t.str());
    }

    const auto expected = expectedVlans();
    const auto missing = missingVlans(expected, actual);
    if (!missing.empty()) {
        std::cerr << logPrefix() << " Verification failed: " << missing.size()
                  << "/" << expected.size() << " entries missing:" << std::endl;
        for (const auto& m : missing) {
            std::cerr << "    " << m.interface << " vid " << m.vlan_id
                      << (m.egress_untagged ? " untagged" : "") << std::endl;
        }
        return false;
    }

    std::cout << logPrefix() << " Verification OK (" << expected.size()
              << "/" << expected.size() << " VMC entries present)" << std::endl;
    return true;
}

std::vector<BridgeVlanEntry> CumulusVmc::expectedVlans()
{
    static const std::vector<std::pair<std::string, int>> groups = {
        {"swp9",   97}, {"swp10", 101}, {"swp11", 105}, {"swp12", 109},
    };
    return expandGroups(groups);
}

bool CumulusVmc::fetchBridgeVlanState(std::vector<BridgeVlanEntry>& out, std::string* raw)
{
    out.clear();
    std::string output;
    // Non-login shells on Cumulus don't have /sbin in PATH ('bridge' lives there).
    // "\$PATH" is escaped so the SWITCH's shell expands it - the remote
    // command travels inside double quotes and would otherwise be expanded
    // with the local PC's PATH.
    if (!m_ssh.execute("PATH=/sbin:/usr/sbin:\\$PATH bridge -j vlan show",
                       &output, /*use_sudo=*/false, /*silent=*/true)) {
        VERBOSE_LOG(logPrefix() << " bridge -j vlan show: SSH failed");
        return false;
    }
    if (raw) *raw = output;
    if (output.empty()) {
        VERBOSE_LOG(logPrefix() << " bridge -j vlan show: empty output");
        return false;
    }
    if (!parseBridgeVlanJson(output, out)) {
        VERBOSE_LOG(logPrefix() << " bridge -j vlan show: parse produced 0 entries");
        return false;
    }
    VERBOSE_LOG(logPrefix() << " bridge -j vlan show: parsed " << out.size() << " entries");
    return true;
}

std::vector<BridgeVlanEntry> CumulusVmc::missingVlans(
    const std::vector<BridgeVlanEntry>& expected,
    const std::vector<BridgeVlanEntry>& actual)
{
    std::vector<BridgeVlanEntry> missing;
    for (const auto& exp : expected) {
        bool found = false;
        for (const auto& act : actual) {
            if (act.interface == exp.interface &&
                act.vlan_id == exp.vlan_id &&
                act.egress_untagged == exp.egress_untagged) {
                found = true;
                break;
            }
        }
        if (!found) missing.push_back(exp);
    }
    return missing;
}

bool CumulusVmc::executeBatch(const std::vector<std::string>& commands, bool use_sudo)
{
    if (commands.empty()) return true;

    // Wrapped in `sh -c '...'` so that:
    //  1) with sudo, `echo <pw> | sudo -S sh -c '...'` has ONE exec target
    //     (`set` is a builtin, sudo can't run it directly);
    //  2) `set -e` aborts the batch on the first failing command.
    // Callers must not use raw single quotes inside the commands.
    std::ostringstream body;
    // "\$PATH": expanded on the switch, not locally (see fetchBridgeVlanState).
    body << "export PATH=/sbin:/usr/sbin:/bin:/usr/bin:\\$PATH";
    body << "; set -e";
    for (const auto& cmd : commands) {
        body << "; " << cmd;
    }
    const std::string wrapped = "sh -c '" + body.str() + "'";

    return m_ssh.execute(wrapped, nullptr, use_sudo, /*silent=*/true);
}

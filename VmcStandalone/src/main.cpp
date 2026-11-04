// ============================================================================
// VmcStandalone - standalone VMC test application
//
// Independent of MainSoftware: no PSU, server or DPDK handling. Steps are
// executed in order; the first failing step stops the run.
//
//   Step 1: Configure the Cumulus switch for VMC
//           (interfaces deploy + ifreload + VMC bridge VLAN sequence)
//   Step 2: Deploy the prebuilt vmc_test to the ATE server and run it
//           (vmc_test is compiled on the server during the build)
//
// New steps are added to the `steps` table in main().
// ============================================================================

#include "CumulusConfig.h"
#include "CumulusVmc.h"
#include "Log.h"
#include "ServerConfig.h"
#include "SshClient.h"
#include "SystemCommand.h"
#include "Version.h"
#include "VmcTestRunner.h"

#include <csignal>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace logcfg {
    bool verbose = false;
}

namespace {

struct Options {
    std::string interfaces_path;   // empty -> auto-resolve
    bool verify_only = false;
    bool skip_cumulus = false;
    bool skip_vmc_test = false;
    std::string vmc_test_args;     // everything after "--"
};

struct Step {
    std::string name;
    std::function<bool()> run;
};

void printUsage(const char* prog)
{
    std::cout
        << "Usage: " << prog << " [options] [-- vmc_test args...]\n"
        << "\n"
        << "Standalone VMC application. Steps run in order:\n"
        << "  Step 1: configure the Cumulus switch (" << CUMULUS_HOST << ") for VMC\n"
        << "  Step 2: deploy vmc_test to the server (" << SERVER_HOST << ") and run it\n"
        << "\n"
        << "Options:\n"
        << "  -i, --interfaces PATH  VMC interfaces file to deploy\n"
        << "                         (default: auto-locate " << VMC_INTERFACES_RELATIVE_PATH << ")\n"
        << "      --verify-only      Only check the VMC VLAN state on the switch, change nothing\n"
        << "      --skip-cumulus     Skip step 1 (Cumulus configuration)\n"
        << "      --skip-vmc-test    Skip step 2 (vmc_test on the server)\n"
        << "  --                     Pass the remaining arguments to vmc_test\n"
        << "  -v, --verbose          Print every SSH command and its output\n"
        << "      --version          Print version and exit\n"
        << "  -h, --help             Show this help\n";
}

// Returns false (and prints why) on bad arguments. `exit_now` is set for
// --help / --version.
bool parseArgs(int argc, char* argv[], Options& opt, bool& exit_now)
{
    exit_now = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            exit_now = true;
            return true;
        } else if (arg == "--version") {
            std::cout << "VmcStandalone " << Version << std::endl;
            exit_now = true;
            return true;
        } else if (arg == "-v" || arg == "--verbose") {
            logcfg::verbose = true;
        } else if (arg == "--verify-only") {
            opt.verify_only = true;
        } else if (arg == "--skip-cumulus") {
            opt.skip_cumulus = true;
        } else if (arg == "--skip-vmc-test") {
            opt.skip_vmc_test = true;
        } else if (arg == "--") {
            for (int j = i + 1; j < argc; ++j) {
                if (!opt.vmc_test_args.empty()) opt.vmc_test_args += " ";
                opt.vmc_test_args += argv[j];
            }
            break;
        } else if (arg == "-i" || arg == "--interfaces") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for " << arg << std::endl;
                return false;
            }
            opt.interfaces_path = argv[++i];
        } else {
            std::cerr << "Unknown option: " << arg << "\n\n";
            printUsage(argv[0]);
            return false;
        }
    }
    return true;
}

std::filesystem::path executableDir()
{
    std::error_code ec;
    auto exe = std::filesystem::canonical("/proc/self/exe", ec);
    return ec ? std::filesystem::current_path() : exe.parent_path();
}

// Locate the VMC interfaces file. Search order:
//   <exe_dir>/          build/bin/ copy (CMake puts it there) / portable layout
//   <exe_dir>/../..     VmcStandalone/ itself, when running from build/bin/
//   <cwd>/              current directory
// Deliberately never looks outside VmcStandalone/ (the app is independent).
std::string resolveInterfacesPath(const std::string& user_path)
{
    if (!user_path.empty()) {
        return std::filesystem::absolute(user_path).string();
    }

    const std::filesystem::path rel(VMC_INTERFACES_RELATIVE_PATH);
    const auto exe_dir = executableDir();
    const std::vector<std::filesystem::path> candidates = {
        exe_dir / rel,
        exe_dir / ".." / ".." / rel,
        std::filesystem::current_path() / rel,
    };

    for (const auto& c : candidates) {
        if (std::filesystem::exists(c)) {
            return std::filesystem::weakly_canonical(c).string();
        }
    }

    std::cerr << "[SYSTEM] VMC interfaces file not found. Searched:" << std::endl;
    for (const auto& c : candidates) {
        std::cerr << "  - " << c.lexically_normal().string() << std::endl;
    }
    return "";
}

bool checkLocalTools()
{
    bool ok = true;
    for (const char* tool : {"sshpass", "ssh", "scp"}) {
        if (!syscmd::isToolAvailable(tool)) {
            ErrorPrinter::error("SYSTEM", std::string("Required tool not found on PATH: ") + tool);
            ok = false;
        }
    }
    return ok;
}

} // namespace

int main(int argc, char* argv[])
{
    // A closed SSH pipe must not kill the process.
    std::signal(SIGPIPE, SIG_IGN);

    Options opt;
    bool exit_now = false;
    if (!parseArgs(argc, argv, opt, exit_now)) return 2;
    if (exit_now) return 0;

    std::cout << "========================================" << std::endl;
    std::cout << " VmcStandalone v" << Version << std::endl;
    std::cout << "========================================" << std::endl;

    if (!checkLocalTools()) return 1;

    SshClient cumulus_ssh(CUMULUS_HOST, CUMULUS_USER, CUMULUS_PASSWORD, "Cumulus");
    CumulusVmc cumulus(cumulus_ssh);

    SshClient server_ssh(SERVER_HOST, SERVER_USER, SERVER_PASSWORD, "Server");
    VmcTestRunner vmc_test(server_ssh);

    std::vector<Step> steps;

    if (opt.verify_only) {
        steps.push_back({"Verify Cumulus VMC VLAN state", [&] {
            return cumulus.connect() && cumulus.verifyVlans();
        }});
    } else {
        if (!opt.skip_cumulus) {
            const std::string interfaces_path = resolveInterfacesPath(opt.interfaces_path);
            if (interfaces_path.empty()) return 1;

            steps.push_back({"Configure Cumulus for VMC", [&cumulus, interfaces_path] {
                return cumulus.configure(interfaces_path);
            }});
        }

        if (!opt.skip_vmc_test) {
            steps.push_back({"Run vmc_test on server", [&vmc_test, &opt] {
                return vmc_test.deployAndRun(opt.vmc_test_args);
            }});
        }
        // Next steps go here, e.g.:
        // steps.push_back({"...", [&] { return ...; }});
    }

    if (steps.empty()) {
        ErrorPrinter::warn("SYSTEM", "Nothing to do (all steps skipped)");
        return 0;
    }

    for (size_t i = 0; i < steps.size(); ++i) {
        const std::string tag = "Step " + std::to_string(i + 1) + "/" +
                                std::to_string(steps.size());
        ErrorPrinter::info("SYSTEM", tag + ": " + steps[i].name);

        if (!steps[i].run()) {
            ErrorPrinter::error("SYSTEM", tag + " FAILED: " + steps[i].name);
            return 1;
        }
        ErrorPrinter::info("SYSTEM", tag + " OK: " + steps[i].name);
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << " All steps completed successfully" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}

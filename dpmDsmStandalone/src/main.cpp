// ============================================================================
// dpmDsmStandalone - standalone DPM / DSM test application
// (copy of vmcStandalone; the unit is selected at start-up, see UnitConfig.h)
//
// Independent of MainSoftware: no PSU, server or DPDK handling. Steps are
// executed in order; the first failing step stops the run.
//
//   Step 1: Configure the Cumulus switch for the selected unit
//           (interfaces deploy + ifreload + A/B line bridge VLAN sequence)
//   Step 2: Deploy the prebuilt dpm_dsm_test to the ATE server and run it
//           (dpm_dsm_test is compiled on the server during the build)
//
// New steps are added to the `steps` table in main().
// ============================================================================

#include "CumulusConfig.h"
#include "CumulusVmc.h"
#include "Log.h"
#include "Logger.h"
#include "ServerConfig.h"
#include "SshClient.h"
#include "SystemCommand.h"
#include "UnitConfig.h"
#include "Version.h"
#include "VmcTestRunner.h"

#include <chrono>
#include <csignal>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unistd.h>
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
    std::string log_dir;           // empty -> <exe_dir>/LOGS/<UNIT>
    std::string unit;              // "dpm" / "dsm"; empty -> ask
};

// Selected unit (set in main() before anything else runs).
const UnitConfig* g_unit = &UNIT_DSM;

struct Step {
    std::string name;
    std::function<bool()> run;
};

void printUsage(const char* prog)
{
    std::cout
        << "Usage: " << prog << " [options] [-- dpm_dsm_test args...]\n"
        << "\n"
        << "Standalone DPM / DSM application. Steps run in order:\n"
        << "  Step 1: configure the Cumulus switch (" << CUMULUS_HOST << ") for the unit\n"
        << "  Step 2: deploy dpm_dsm_test to the server (" << SERVER_HOST << ") and run it\n"
        << "\n"
        << "Options:\n"
        << "  -u, --unit dpm|dsm     Unit under test (default: ask at start-up)\n"
        << "  -i, --interfaces PATH  Interfaces file to deploy\n"
        << "                         (default: auto-locate " << UNIT_DPM.interfacesPath
        << " / " << UNIT_DSM.interfacesPath << ")\n"
        << "      --verify-only      Only check the A/B line VLAN state on the switch, change nothing\n"
        << "      --skip-cumulus     Skip step 1 (Cumulus configuration)\n"
        << "      --skip-dpm-dsm-test  Skip step 2 (dpm_dsm_test on the server)\n"
        << "  --                     Pass the remaining arguments to dpm_dsm_test\n"
        << "  -v, --verbose          Also print every SSH command and its output on the console\n"
        << "                         (they are always written to the log file)\n"
        << "      --log-dir DIR      Where run logs go (default: <exe dir>/LOGS/<DPM|DSM>)\n"
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
            std::cout << "dpmDsmStandalone " << Version << std::endl;
            exit_now = true;
            return true;
        } else if (arg == "-v" || arg == "--verbose") {
            logcfg::verbose = true;
        } else if (arg == "--verify-only") {
            opt.verify_only = true;
        } else if (arg == "--skip-cumulus") {
            opt.skip_cumulus = true;
        } else if (arg == "--skip-dpm-dsm-test") {
            opt.skip_vmc_test = true;
        } else if (arg == "--") {
            for (int j = i + 1; j < argc; ++j) {
                if (!opt.vmc_test_args.empty()) opt.vmc_test_args += " ";
                opt.vmc_test_args += argv[j];
            }
            break;
        } else if (arg == "--log-dir") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for " << arg << std::endl;
                return false;
            }
            opt.log_dir = argv[++i];
        } else if (arg == "-u" || arg == "--unit") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for " << arg << std::endl;
                return false;
            }
            opt.unit = argv[++i];
            if (opt.unit != "dpm" && opt.unit != "dsm") {
                std::cerr << "Unknown unit: " << opt.unit << " (use dpm or dsm)" << std::endl;
                return false;
            }
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

// Locate the selected unit's interfaces file. Search order:
//   <exe_dir>/          build/bin/ copy (CMake puts it there) / portable layout
//   <exe_dir>/../..     dpmDsmStandalone/ itself, when running from build/bin/
//   <cwd>/              current directory
// Deliberately never looks outside dpmDsmStandalone/ (the app is independent).
std::string resolveInterfacesPath(const std::string& user_path)
{
    if (!user_path.empty()) {
        return std::filesystem::absolute(user_path).string();
    }

    const std::filesystem::path rel(g_unit->interfacesPath);
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

    std::cerr << "[SYSTEM] " << g_unit->name << " interfaces file not found. Searched:" << std::endl;
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


struct StepResult {
    std::string name;
    std::string status = "NOT RUN";   // PASS / FAIL / NOT RUN
    double seconds = 0.0;
};

std::string readFirstLine(const std::string& path)
{
    std::ifstream f(path);
    std::string line;
    std::getline(f, line);
    return line;
}

std::string localHostName()
{
    char buf[256] = {0};
    if (gethostname(buf, sizeof(buf) - 1) != 0) return "unknown";
    return buf;
}

std::string buildLogHeader(int argc, char* argv[], const std::string& log_path)
{
    std::string cmdline;
    for (int i = 0; i < argc; ++i) {
        if (i) cmdline += " ";
        cmdline += argv[i];
    }
    const std::string vmc_test_ver =
        readFirstLine(VmcTestRunner::localPrebuiltDir() + "/VERSION.txt");

    std::ostringstream h;
    h << "========================================\n";
    h << "         " << g_unit->name << " STANDALONE LOG\n";
    h << "========================================\n";
    h << "Software Start Time : " << logx::timestamp() << "\n";
    h << "Unit Name           : " << g_unit->name << "\n";
    h << "Software Version    : " << Version << "\n";
    h << "dpm_dsm_test Version: " << (vmc_test_ver.empty() ? "N/A" : vmc_test_ver) << "\n";
    h << "PC Host             : " << localHostName() << "\n";
    h << "Command Line        : " << cmdline << "\n";
    h << "Cumulus Switch      : " << CUMULUS_USER << "@" << CUMULUS_HOST << "\n";
    h << "ATE Server          : " << SERVER_USER << "@" << SERVER_HOST << "\n";
    h << "Log File            : " << log_path << "\n";
    h << "========================================\n\n";
    return h.str();
}

void printSummary(const std::vector<StepResult>& results, bool ok, double total_s)
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "         SUMMARY" << std::endl;
    std::cout << "========================================" << std::endl;
    for (size_t i = 0; i < results.size(); ++i) {
        std::cout << "Step " << (i + 1) << "  " << std::left << std::setw(32) << results[i].name
                  << std::setw(8) << results[i].status;
        if (results[i].status != "NOT RUN") {
            std::cout << std::right << std::fixed << std::setprecision(1)
                      << std::setw(7) << results[i].seconds << " s";
        }
        std::cout << std::endl;
    }
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Result              : " << (ok ? "PASS" : "FAIL") << std::endl;
    std::cout << "Software End Time   : " << logx::timestamp() << std::endl;
    std::cout << "Total Duration      : " << std::fixed << std::setprecision(1)
              << total_s << " s" << std::endl;
    if (logx::active()) {
        std::cout << "Log Directory       : " << logx::dir() << std::endl;
    }
    std::cout << "========================================" << std::endl;
}

double secondsSince(std::chrono::steady_clock::time_point t0)
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

// Everything after argument parsing; runs with the session log active.
int runSteps(const Options& opt)
{
    std::cout << "========================================" << std::endl;
    std::cout << " dpmDsmStandalone v" << Version << "  -  unit " << g_unit->name << std::endl;
    std::cout << "========================================" << std::endl;
    if (logx::active()) {
        ErrorPrinter::info("LOG", "Logging to " + logx::filePath());
    }

    if (!checkLocalTools()) return 1;

    SshClient cumulus_ssh(CUMULUS_HOST, CUMULUS_USER, CUMULUS_PASSWORD, "Cumulus");
    CumulusVmc cumulus(cumulus_ssh, g_unit->name);

    SshClient server_ssh(SERVER_HOST, SERVER_USER, SERVER_PASSWORD, "Server");
    VmcTestRunner vmc_test(server_ssh);

    std::vector<Step> steps;

    if (opt.verify_only) {
        steps.push_back({std::string("Verify Cumulus ") + g_unit->name + " VLAN state", [&] {
            return cumulus.connect() && cumulus.verifyVlans();
        }});
    } else {
        if (!opt.skip_cumulus) {
            const std::string interfaces_path = resolveInterfacesPath(opt.interfaces_path);
            if (interfaces_path.empty()) return 1;

            steps.push_back({std::string("Configure Cumulus for ") + g_unit->name, [&cumulus, interfaces_path] {
                return cumulus.configure(interfaces_path);
            }});
        }

        if (!opt.skip_vmc_test) {
            steps.push_back({"Run dpm_dsm_test on server", [&vmc_test, &opt] {
                // dpm_dsm_test runs the selected unit's PTP sequence.
                std::string args = std::string("--unit ") + g_unit->testArg;
                if (!opt.vmc_test_args.empty()) args += " " + opt.vmc_test_args;
                return vmc_test.deployAndRun(args);
            }});
        }
        // Next steps go here, e.g.:
        // steps.push_back({"...", [&] { return ...; }});
    }

    if (steps.empty()) {
        ErrorPrinter::warn("SYSTEM", "Nothing to do (all steps skipped)");
        return 0;
    }

    std::vector<StepResult> results(steps.size());
    for (size_t i = 0; i < steps.size(); ++i) results[i].name = steps[i].name;

    const auto run_t0 = std::chrono::steady_clock::now();
    bool ok = true;
    for (size_t i = 0; i < steps.size(); ++i) {
        const std::string tag = "Step " + std::to_string(i + 1) + "/" +
                                std::to_string(steps.size());
        ErrorPrinter::info("SYSTEM", tag + ": " + steps[i].name);

        const auto t0 = std::chrono::steady_clock::now();
        const bool step_ok = steps[i].run();
        results[i].seconds = secondsSince(t0);
        results[i].status = step_ok ? "PASS" : "FAIL";

        if (!step_ok) {
            ErrorPrinter::error("SYSTEM", tag + " FAILED: " + steps[i].name);
            ok = false;
            break;
        }
        ErrorPrinter::info("SYSTEM", tag + " OK: " + steps[i].name);
    }

    printSummary(results, ok, secondsSince(run_t0));
    return ok ? 0 : 1;
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

    // Unit selection: --unit, otherwise ask.
    if (opt.unit.empty()) {
        if (!isatty(STDIN_FILENO)) {
            std::cerr << "No terminal to ask for the unit - pass --unit dpm or --unit dsm" << std::endl;
            return 2;
        }
        while (opt.unit.empty()) {
            std::cout << "Select unit:\n  1) DPM\n  2) DSM\n> " << std::flush;
            std::string ans;
            if (!std::getline(std::cin, ans)) return 2;
            if (ans == "1" || ans == "dpm" || ans == "DPM") opt.unit = "dpm";
            else if (ans == "2" || ans == "dsm" || ans == "DSM") opt.unit = "dsm";
            else std::cout << "Please enter 1 or 2." << std::endl;
        }
    }
    g_unit = (opt.unit == "dpm") ? &UNIT_DPM : &UNIT_DSM;

    // Passwords never end up in the log or on the console.
    logx::addSecret(CUMULUS_PASSWORD);
    logx::addSecret(SERVER_PASSWORD);

    // One directory per run: <log root>/<YYYY-MM-DD_HH-MM-SS>/
    const std::filesystem::path log_root = opt.log_dir.empty()
        ? executableDir() / "LOGS" / g_unit->name
        : std::filesystem::absolute(opt.log_dir);
    const std::string run_dir = (log_root / logx::fileSafeTimestamp()).string();
    const std::string log_name = std::string(g_unit->name) + "_Standalone_Log.log";
    logx::start(run_dir, log_name,
                buildLogHeader(argc, argv, (std::filesystem::path(run_dir) / log_name).string()));

    const int rc = runSteps(opt);

    const std::string log_path = logx::filePath();
    logx::stop();
    if (!log_path.empty()) {
        std::cout << "Log saved: " << log_path << std::endl;
    }
    return rc;
}

#include "Dtn.h"
#include "SSHDeployer.h"
#include "CumulusHelper.h"
#include "SerialTimeForwarder.h"
#include "ReportManager.h"
#include "SafeShutdown.h"
#include "ErrorPrinter.h"
#include "PsuTelemetry.h"
#include "PsuTelemetryPublisher.h"
#include <iostream>
#include <unistd.h>
#include <iomanip>
#include <filesystem>
#include <limits>
#include <csignal>
#include <atomic>
#include <sstream>
#include <fstream>
#include <map>
#include "Utils.h"

// LogPaths::rootDir() - runtime root detection (replaces compile-time PROJECT_ROOT)
std::string LogPaths::rootDir() {
    return SSHDeployer::getSourceRoot();
}

// Global flag for Ctrl+C handling in DPDK monitoring
static std::atomic<bool> g_dpdk_monitoring_running{true};

static void dpdk_monitor_signal_handler(int sig)
{
    (void)sig;
    g_dpdk_monitoring_running = false;
}

// ==================== Server-side NIC counters ====================

// The two server NICs wired straight to the DTN, bypassing the Cumulus
// switch: Ports 12 and 13 of the DPDK application's raw socket ports. The
// switch counter reports cannot see a single frame of this traffic, so
// without these the only account of it is the test's own.
static const char *const kServerDirectIfaces[] = { "eno12399", "eno12409" };

// The baseline reading, kept so the end-of-test report can subtract it. What
// the switch gets from clearing its counters, these get from differencing two
// readings - so the report says what the test did, not what the server has
// forwarded since it booted.
typedef std::map<std::string, unsigned long long> NicCounterMap;
static std::map<std::string, NicCounterMap> g_server_nic_baseline;

// Pull the /sys/class/net lines out of a report body. They come back as
//   /sys/class/net/eno12399/statistics/rx_packets:1234
// which is the one of the three views that is a plain name and a plain number
// - ip -s and ethtool -S are laid out for reading, not for subtracting.
static std::map<std::string, NicCounterMap> parseNicCounters(const std::string &raw)
{
    std::map<std::string, NicCounterMap> out;
    const std::string prefix = "/sys/class/net/";
    size_t pos = 0;
    while ((pos = raw.find(prefix, pos)) != std::string::npos) {
        const size_t line_end = raw.find('\n', pos);
        const std::string line = raw.substr(pos, line_end == std::string::npos
                                                 ? std::string::npos
                                                 : line_end - pos);
        pos = (line_end == std::string::npos) ? raw.size() : line_end + 1;

        // .../<iface>/statistics/<name>:<value>
        const size_t iface_start = prefix.size();
        const size_t iface_end = line.find('/', iface_start);
        if (iface_end == std::string::npos) continue;
        const std::string marker = "/statistics/";
        if (line.compare(iface_end, marker.size(), marker) != 0) continue;
        const size_t name_start = iface_end + marker.size();
        const size_t colon = line.find(':', name_start);
        if (colon == std::string::npos) continue;

        const std::string iface = line.substr(iface_start, iface_end - iface_start);
        const std::string name = line.substr(name_start, colon - name_start);
        const std::string value = line.substr(colon + 1);
        try {
            out[iface][name] = std::stoull(value);
        } catch (const std::exception &) {
            // A line that is not a number is not a counter; skip it rather
            // than letting one malformed row lose the whole interface.
        }
    }
    return out;
}

// Kernel interface counters have no equivalent of the switch's "nv action
// clear" - short of reloading the driver they cannot be zeroed, and that
// would take down every port the driver owns. So both ends are read instead
// and the end-of-test report leads with the difference.
//
// Quoting, as in CumulusHelper::saveCounterReport: SSHDeployer wraps the
// remote command in double quotes and hands it to the local shell, so the
// body must contain no double quotes and no '$' - both would be eaten locally
// before ssh ever saw them.
static bool saveServerNicCounterReport(const std::string &local_path,
                                       const std::string &title,
                                       bool is_baseline)
{
    std::ostringstream body;
    for (const char *iface : kServerDirectIfaces) {
        body << "echo SERVER_NIC_MARKER_" << iface << "; ";
        // Three views, because they fail differently: ip -s is the kernel's
        // own summary, /sys/class/net is the same numbers unformatted and
        // trivial to diff between two reports, and ethtool -S is the driver's,
        // which is the only one that names a packet the NIC itself dropped.
        body << "ip -s -s link show " << iface << " 2>&1; ";
        body << "grep . /sys/class/net/" << iface << "/statistics/* 2>&1; ";
        body << "ethtool -S " << iface << " 2>&1; ";
    }
    const std::string command = "sh -c '" + body.str() + "'";

    std::string raw;
    if (!g_ssh_deployer_server.execute(command, &raw, /*use_sudo=*/false,
                                       /*silent=*/true, /*timeout_ms=*/120000)
        || raw.empty())
    {
        ErrorPrinter::warn("SSH",
            "DTN: Could not read the server NIC counters");
        return false;
    }

    // A marker per interface has to come back, or the report would look
    // complete while holding nothing.
    size_t found = 0;
    for (const char *iface : kServerDirectIfaces) {
        if (raw.find(std::string("SERVER_NIC_MARKER_") + iface) != std::string::npos) {
            found++;
        }
    }
    if (found == 0) {
        ErrorPrinter::warn("SSH",
            "DTN: Server NIC counter output held no interface data");
        return false;
    }

    std::ofstream file(local_path);
    if (!file.is_open()) {
        ErrorPrinter::warn("SSH",
            "DTN: Could not open server NIC counter report for writing: "
            + local_path);
        return false;
    }
    file << "========================================\n";
    file << " " << title << "\n";
    file << " Collected: " << ReportManager::getCurrentTimestamp() << "\n";
    file << " Interfaces: ";
    for (size_t i = 0; i < sizeof(kServerDirectIfaces) / sizeof(kServerDirectIfaces[0]); ++i) {
        file << (i ? ", " : "") << kServerDirectIfaces[i];
    }
    file << "\n";
    file << " These are direct server-to-DTN links; the switch never sees\n";
    file << " this traffic, so its counter reports do not cover them.\n";
    file << "========================================\n\n";

    const std::map<std::string, NicCounterMap> now = parseNicCounters(raw);
    if (is_baseline) {
        g_server_nic_baseline = now;
    } else if (!g_server_nic_baseline.empty()) {
        // The part worth reading, so it goes first: what this test did, with
        // the two readings it came from beside it.
        file << "---------- CHANGE SINCE THE BASELINE (this test only) ----------\n\n";
        for (const auto &iface_entry : now) {
            const auto base_it = g_server_nic_baseline.find(iface_entry.first);
            file << iface_entry.first << "\n";
            if (base_it == g_server_nic_baseline.end()) {
                file << "  (no baseline reading for this interface)\n\n";
                continue;
            }
            for (const auto &counter : iface_entry.second) {
                const auto base_counter = base_it->second.find(counter.first);
                file << "  " << std::left << std::setw(22) << counter.first
                     << std::right;
                if (base_counter == base_it->second.end()) {
                    file << std::setw(18) << "n/a"
                         << "   (not in the baseline reading)\n";
                    continue;
                }
                // A counter that went backwards means the interface was reset
                // under us; saying so beats printing a number near 2^64.
                if (counter.second < base_counter->second) {
                    file << std::setw(18) << "?"
                         << "   (went backwards: " << base_counter->second
                         << " -> " << counter.second << ")\n";
                    continue;
                }
                file << std::setw(18) << (counter.second - base_counter->second)
                     << "   (" << base_counter->second << " -> "
                     << counter.second << ")\n";
            }
            file << "\n";
        }
        file << "---------- THE READINGS THEMSELVES ----------\n\n";
    } else {
        file << "(no baseline reading was taken, so no difference can be shown)\n\n";
    }

    file << raw;
    file.close();

    std::cout << "DTN: Server NIC counters (" << found << "/"
              << sizeof(kServerDirectIfaces) / sizeof(kServerDirectIfaces[0])
              << " interfaces) saved to: " << local_path << std::endl;
    return true;
}

// PSU Configuration: 28V 3.0A

Dtn g_dtn;

Dtn::Dtn()
{
}

Dtn::~Dtn()
{
}

bool Dtn::latencyTestSequence()
{
    bool valid_test = false;

    if (askQuestion("Do you want to run HW Timestamp Latency Test (Default measured latency : 14us)"))
    {
        while (!valid_test)
        {
            bool answer = askQuestion("You need to install the LoopBack connectors for this test. Check before starting the test. Should I start the test?");
            if (answer)
            {
                valid_test = true;
                runLatencyTest("-n 1 -vvv");
            }
            else
            {
                if (askQuestion("Do you want to skip the test?"))
                {
                    valid_test = true;
                    return true;
                }
            }
        }
    }
    return 0;
}

bool Dtn::askQuestion(const std::string &question)
{
    char response;
    while (true)
    {
        std::cout << question << " [y/n]: ";
        std::cin >> response;

        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input! Please enter 'y' or 'n'.\n";
            continue;
        }

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (response == 'y' || response == 'Y')
        {
            return true;
        }
        else if (response == 'n' || response == 'N')
        {
            return false;
        }
        else
        {
            std::cout << "Invalid input! Please enter 'y' or 'n'.\n";
        }
    }
}

uint8_t Dtn::ask28VoltStatus()
{
    int choice = 0;
    while (true)
    {
        std::cout << std::endl;
        std::cout << "======================================" << std::endl;
        std::cout << "  28V Power Status Selection" << std::endl;
        std::cout << "======================================" << std::endl;
        std::cout << "  1. Primary SUCCESS, Secondary FAIL" << std::endl;
        std::cout << "  2. Primary FAIL, Secondary SUCCESS" << std::endl;
        std::cout << "  3. Both SUCCESS" << std::endl;
        std::cout << "======================================" << std::endl;
        std::cout << "Select option [1-3]: ";
        std::cin >> choice;

        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input! Please enter 1, 2, or 3." << std::endl;
            continue;
        }

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice)
        {
            case 1:
                // Primary SUCCESS (bit0=0), Secondary FAIL (bit1=1) → 0x02
                std::cout << "Selected: Primary=SUCCESS, Secondary=FAIL (0x02)" << std::endl;
                return 0x02;
            case 2:
                // Primary FAIL (bit0=1), Secondary SUCCESS (bit1=0) → 0x01
                std::cout << "Selected: Primary=FAIL, Secondary=SUCCESS (0x01)" << std::endl;
                return 0x01;
            case 3:
                // Both SUCCESS → 0x00
                std::cout << "Selected: Primary=SUCCESS, Secondary=SUCCESS (0x00)" << std::endl;
                return 0x00;
            default:
                std::cout << "Invalid option! Please enter 1, 2, or 3." << std::endl;
                break;
        }
    }
}

bool Dtn::ensureLogDirectories()
{
    try
    {
        std::filesystem::create_directories(LogPaths::CMC());
        std::filesystem::create_directories(LogPaths::VMC());
        std::filesystem::create_directories(LogPaths::MMC());
        std::filesystem::create_directories(LogPaths::DTN());
        std::filesystem::create_directories(LogPaths::HSN());
        DEBUG_LOG("DTN: Log directories created/verified at " << LogPaths::baseDir());
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "DTN: Failed to create log directories: " << e.what() << std::endl;
        return false;
    }
}

bool Dtn::runLatencyTest(const std::string &run_args, int timeout_seconds)
{
    std::cout << "======================================" << std::endl;
    std::cout << "DTN: HW Timestamp Latency Test" << std::endl;
    std::cout << "======================================" << std::endl;

    // Ensure log directories exist
    if (!ensureLogDirectories())
    {
        std::cerr << "DTN: Failed to create log directories!" << std::endl;
        return false;
    }

    // Build local log path
    std::string local_log_path = g_ReportManager.getTestLogDir() + "/latency_test.log";

    DEBUG_LOG("DTN: Run arguments: " << (run_args.empty() ? "(default)" : run_args));
    DEBUG_LOG("DTN: Timeout: " << timeout_seconds << " seconds");
    DEBUG_LOG("DTN: Log output: " << local_log_path);

    // Run the test using SSHDeployer (prebuilt binary, no compilation)
    bool result = g_ssh_deployer_server.deployPrebuiltRunAndFetchLog(
        "latency_test", // Prebuilt folder (inside prebuilt/)
        "latency_test", // Executable name
        run_args,       // Arguments (e.g., "-n 10 -v")
        local_log_path, // Local log path
        timeout_seconds // Timeout
    );

    if (result)
    {
        std::cout << "======================================" << std::endl;
        std::cout << "DTN: Latency Test COMPLETED" << std::endl;
        std::cout << "DTN: Log saved to: " << local_log_path << std::endl;
        std::cout << "======================================" << std::endl;
    }
    else
    {
        std::cerr << "DTN: Latency Test FAILED!" << std::endl;
    }

    return result;
}

bool Dtn::runDpdkInteractive(const std::string &eal_args, const std::string &make_args)
{
    std::cout << "======================================" << std::endl;
    std::cout << "DTN: DPDK Interactive Deployment" << std::endl;
    std::cout << "======================================" << std::endl;

    // Step 1: Test connection
    if (!g_ssh_deployer_server.testConnection())
    {
        std::cerr << "DTN: Cannot connect to server!" << std::endl;
        return false;
    }

    // Step 2: Deploy prebuilt DPDK binary (no compilation on server)
    DEBUG_LOG("DTN: Deploying prebuilt DPDK binary...");
    if (!g_ssh_deployer_server.deployPrebuilt(
            "dpdk",  // prebuilt folder (inside prebuilt/)
            "",      // app name (auto-detect: dpdk_app)
            false,   // DON'T run after deploy (we'll run interactively)
            false,   // no sudo for deploy
            "",      // no run args (not running yet)
            false    // not background
            ))
    {
        std::cerr << "DTN: DPDK prebuilt deploy failed!" << std::endl;
        return false;
    }

    // Step 3: Run DPDK interactively
    // User can answer y/n prompts for latency tests
    // After tests complete, DPDK will fork to background automatically
    std::cout << std::endl;
    std::cout << "======================================" << std::endl;
    DEBUG_LOG("DTN: Starting DPDK Interactive Mode");
    DEBUG_LOG("DTN: You can answer latency test prompts (y/n)");
    DEBUG_LOG("DTN: After tests, DPDK will continue in background");
    std::cout << "======================================" << std::endl;
    std::cout << std::endl;

    std::string remote_dir = g_ssh_deployer_server.getRemoteDirectory();

    // IMPORTANT: Don't pipe password to sudo, it breaks stdin for interactive input!
    // Instead: First authenticate sudo (caches credentials), then run DPDK
    // sudo -v = validate/refresh sudo timestamp without running a command
    // sudo -S = read password from stdin (only for the -v part)
    // After -v succeeds, subsequent sudo commands don't need password (within timeout)
    // --daemon flag: tells DPDK to fork to background after latency tests
    std::string dpdk_command = "cd " + remote_dir + "/dpdk && "
                                                    "echo 'q' | sudo -S -v && " // Authenticate sudo first
                                                    "sudo ./dpdk_app --daemon " +
                               eal_args; // --daemon for background mode

    bool result = g_ssh_deployer_server.executeInteractive(dpdk_command, false);

    if (result)
    {
        std::cout << std::endl;
        std::cout << "======================================" << std::endl;
        std::cout << "DTN: DPDK started successfully!" << std::endl;
        std::cout << "DTN: Running in background on server" << std::endl;
        std::cout << "DTN: Log file: /tmp/dpdk_app.log" << std::endl;
        std::cout << "======================================" << std::endl;
    }
    else
    {
        std::cerr << "DTN: DPDK interactive execution failed!" << std::endl;
    }

    return result;
}

bool Dtn::configureSequence()
{
    auto& shutdown = SafeShutdown::getInstance();

    // Server power on
    if (!g_Server.onWithWait(3))
    {
        ErrorPrinter::error("SERVER", "DTN: Server could not be started!");
        shutdown.executeShutdown();
        return false;
    }
    shutdown.registerServerOn();

    // Send serial command
    g_systemCommand.execute("echo \"VMC_ID 38\" > /dev/ttyACM0");
    sleep(2);

    // Create PSU G30 (30V, 56A)
    if (!g_DeviceManager.create(PSUG30))
    {
        ErrorPrinter::error("PSU", "DTN: Failed to create PSU G30!");
        shutdown.executeShutdown();
        return false;
    }

    // Connect to PSU G30
    if (!g_DeviceManager.connect(PSUG30))
    {
        ErrorPrinter::error("PSU", "DTN: Failed to connect to PSU G30!");
        shutdown.executeShutdown();
        return false;
    }
    shutdown.registerPsuConnected(PSUG30);

    if (!g_DeviceManager.setCurrent(PSUG30, 3.0))
    {
        ErrorPrinter::error("PSU", "DTN: Failed to set current on PSU G30!");
        shutdown.executeShutdown();
        return false;
    }

    if (!g_DeviceManager.setVoltage(PSUG30, 28.0))
    {
        ErrorPrinter::error("PSU", "DTN: Failed to set voltage on PSU G30!");
        shutdown.executeShutdown();
        return false;
    }

    // Ask user for expected 28V power status BEFORE enabling power output
    // This way the user declares expected state before power sources are turned on
    uint8_t expected_power_status = ask28VoltStatus();
    std::string power_status_arg = "--expected-power-status " + std::to_string(expected_power_status);

    if (!g_DeviceManager.enableOutput(PSUG30, true))
    {
        ErrorPrinter::error("PSU", "DTN: Failed to enable output on PSU G30!");
        shutdown.executeShutdown();
        return false;
    }
    shutdown.registerPsuOutputEnabled(PSUG30);

    // Record Unit Power On Time when PSU output is enabled
    g_ReportManager.recordUnitPowerOnTime();

    // Start PSU telemetry publisher. Sends V/I/W to the DPDK process every
    // second over UDP so DPDK can print a power-supply row in its health
    // table. Also acts as a heartbeat that keeps the PSU TCP connection
    // alive during long idle periods - fixes the "after 2h the PSU doesn't
    // turn off" failure mode. RAII: destructor stops it on any return path.
    PsuTelemetryPublisher psu_publisher(
        PSUG30,
        g_ssh_deployer_server.getHost(),
        PSU_TELEM_PORT);
    if (!psu_publisher.start()) {
        ErrorPrinter::warn("PSU-TELEM",
            "DTN: Failed to start PSU telemetry publisher - "
            "DPDK will not receive power-supply readings, "
            "but the test will continue.");
    }

    sleep(2);
    if (!g_cumulus.deployNetworkInterfaces(SSHDeployer::getPrebuiltRoot() + "/CumulusInterfaces/DTNIRSW/interfaces"))
    {
        ErrorPrinter::error("CUMULUS", "DTN: Failed to deploy network configuration!");
        shutdown.executeShutdown();
        return false;
    }
    DEBUG_LOG("DTN: Network configuration deployed successfully.");

    sleep(1);

    // Configure Cumulus switch VLANs
    if (!g_cumulus.configureSequence())
    {
        ErrorPrinter::error("CUMULUS", "DTN: Cumulus configuration failed!");
        shutdown.executeShutdown();
        return false;
    }

    // Clear the switch-side interface counters now that the VLAN state is
    // final and before any test traffic is generated. Best-effort: a switch
    // that refuses the clear only means its counters carry over from the
    // previous run, which must not abort the test.
    if (!g_cumulus.resetCounters())
    {
        ErrorPrinter::warn("CUMULUS",
            "DTN: Switch interface counters could not be cleared - "
            "they may include traffic from a previous run.");
    }

    sleep(1);

    // Deploy prebuilt RemoteConfigSender binary and run with sudo (required for raw sockets)
    // NOTE: RemoteConfigSender is a fire-and-forget app (runs, completes its task, exits).
    // Do NOT register it with SafeShutdown - it will have already exited by the time
    // shutdown runs, and trying to stop a non-existent process can cause hangs.
    if (!g_ssh_deployer_server.deployPrebuilt("RemoteConfigSender", "", true, true))
    {
        ErrorPrinter::error("SSH", "DTN: RemoteConfigSender deployment unsuccessful!");
        shutdown.executeShutdown();
        return false;
    }

    // Snapshot the switch counters now that RemoteConfigSender has run. This
    // is the post-configuration baseline: counters were cleared right after
    // the VLAN setup, so whatever shows up here is what the configuration
    // traffic itself produced, before any test traffic exists. Only the
    // Ingress Buffer / Egress Queue tables are kept.
    ensureLogDirectories();
    const std::string after_config_log =
        g_ReportManager.getTestLogDir() + "/After_DTN_Config_Log.log";
    if (!g_cumulus.saveCounterReport(after_config_log,
                                     "AFTER DTN CONFIG - CUMULUS INTERFACE COUNTERS"))
    {
        ErrorPrinter::warn("CUMULUS",
            "DTN: After-config counter log could not be written.");
    }

    // The same moment, for the two NICs the switch cannot see. Its counters
    // were cleared just above; these cannot be, so this reading is their
    // baseline and the one taken at the end of the run is the test.
    saveServerNicCounterReport(
        g_ReportManager.getTestLogDir() + "/After_DTN_Config_Server_NIC_Log.log",
        "AFTER DTN CONFIG - SERVER NIC COUNTERS (baseline)",
        /*is_baseline=*/true);

    sleep(2);
    // Start SerialTimeForwarder after remote_config_sender is running
    // Reads time from MicroChip SyncServer (USB0), forwards to USB1, verifies on USB2
    // utc_enable = true  -> incoming local time (GMT+3) is converted to UTC before sending
    // utc_enable = false -> time is forwarded as received (no conversion)
    const bool utc_enable = true;
    serial::SerialTimeForwarder timeForwarder("/dev/ttyUSB0", "/dev/ttyUSB1", "", utc_enable);
    if (timeForwarder.start())
    {
        DEBUG_LOG("DTN: SerialTimeForwarder started successfully.");
    }
    else
    {
        ErrorPrinter::warn("SERIAL", "DTN: Failed to start SerialTimeForwarder: "
                  + timeForwarder.getLastError());
    }

    // DPDK - Interactive mode with embedded latency test
    // Note: expected_power_status was already asked before PSU output was enabled
    if (!runDpdkInteractive(power_status_arg + " -l 0-255 -n 16"))
    {
        ErrorPrinter::error("DPDK", "DTN: DPDK deployment unsuccessful!");
        if (timeForwarder.isRunning()) timeForwarder.stop();
        shutdown.executeShutdown();
        return false;
    }
    shutdown.registerDpdkRunning();

    // DPDK is now running in background on server
    std::cout << "DTN: DPDK is running in background, continuing..." << std::endl;

    // Monitor DPDK stats every 10 seconds until Ctrl+C
    std::cout << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "DTN: Monitoring DPDK (every 10 seconds)" << std::endl;
    std::cout << "DTN: Press Ctrl+C to stop" << std::endl;
    std::cout << "======================================" << std::endl;

    // Setup signal handler for Ctrl+C (DPDK monitoring only)
    g_dpdk_monitoring_running = true;
    struct sigaction sa;
    sa.sa_handler = dpdk_monitor_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);

    while (g_dpdk_monitoring_running)
    {
        // Wait 10 seconds (check flag each second)
        for (int i = 0; i < 10 && g_dpdk_monitoring_running; i++)
        {
            sleep(1);
        }

        if (!g_dpdk_monitoring_running)
            break;

        std::string raw;
        g_ssh_deployer_server.execute(
            "tail -n 300 /tmp/dpdk_app.log", &raw, false, true);

        if (!raw.empty())
        {
            // Check if DPDK has exited or is shutting down by looking for markers in the log
            if (raw.find("Application exited cleanly") != std::string::npos ||
                raw.find("=== Shutting down ===") != std::string::npos)
            {
                std::cout << "\033[2J\033[H";
                std::cout << "======================================" << std::endl;
                std::cout << "DTN: DPDK has exited on server." << std::endl;
                std::cout << "DTN: Auto-exiting monitoring loop..." << std::endl;
                std::cout << "======================================" << std::endl;
                g_dpdk_monitoring_running = false;
                break;
            }

            const std::string sep = "========== [";
            size_t last = raw.rfind(sep);
            size_t prev = (last != std::string::npos && last > 0)
                              ? raw.rfind(sep, last - 1)
                              : std::string::npos;

            std::string block;
            if (prev != std::string::npos && last != std::string::npos)
                block = raw.substr(prev, last - prev);
            else if (last != std::string::npos)
                block = raw.substr(last);
            else
                block = raw;

            std::cout << "\033[2J\033[H";
            std::cout << "=== DPDK Live Stats (Press Ctrl+C to stop) ===" << std::endl;
            std::cout << block << std::endl;
        }
        else
        {
            DEBUG_LOG("(No log output yet - DPDK might still be starting)");
        }
    }

    // Re-install SafeShutdown signal handlers after DPDK monitoring
    shutdown.installSignalHandlers();

    // Stop SerialTimeForwarder and show stats
    if (timeForwarder.isRunning())
    {
        DEBUG_LOG("DTN: SerialTimeForwarder stats:");
        DEBUG_LOG("  Packets sent: " << timeForwarder.getPacketsSent());
        DEBUG_LOG("  Last timestamp: " << timeForwarder.getLastTimestamp());
        DEBUG_LOG("  Last time string: " << timeForwarder.getLastTimeString());
        timeForwarder.stop();
        DEBUG_LOG("DTN: SerialTimeForwarder stopped.");
    }

    // Stop the PSU telemetry publisher BEFORE stopping DPDK so DPDK's final
    // log lines aren't interleaved with PSU telem warnings. Idempotent, so
    // the RAII destructor later is a harmless no-op.
    psu_publisher.stop();

    // Stop DPDK on server
    std::cout << "DTN: Stopping DPDK on server..." << std::endl;
    if (g_ssh_deployer_server.isApplicationRunning("dpdk_app"))
    {
        g_ssh_deployer_server.stopApplication("dpdk_app", true);
        std::cout << "DTN: DPDK stopped." << std::endl;
    }
    else
    {
        DEBUG_LOG("DTN: DPDK was not running.");
    }
    shutdown.unregisterDpdkRunning();

    // Fetch DPDK log from server to local PC
    DEBUG_LOG("DTN: Fetching DPDK log from server...");
    ensureLogDirectories();
    std::string local_dpdk_log = g_ReportManager.getTestLogFilePath();
    if (g_ssh_deployer_server.fetchFile("/tmp/dpdk_app.log", local_dpdk_log))
    {
        std::cout << "DTN: DPDK log saved to: " << local_dpdk_log << std::endl;
    }
    else
    {
        ErrorPrinter::warn("SSH", "DTN: Failed to fetch DPDK log (file may not exist)");
    }

    // Also fetch the separate summary log (DTN table + Health Monitor + PSU
    // block from the last full second before Ctrl+C). Written by dpdk on stop;
    // may be absent if the run crashed before the snapshot was dumped.
    std::string local_summary_log = g_ReportManager.getSummaryLogFilePath();
    if (g_ssh_deployer_server.fetchFile("/tmp/DTN_IRSW_EQ_Test_Result_Summary_Log_Files.log", local_summary_log))
    {
        std::cout << "DTN: Summary log saved to: " << local_summary_log << std::endl;
    }
    else
    {
        ErrorPrinter::warn("SSH", "DTN: Failed to fetch summary log (file may not exist)");
    }

    // And the per VL-ID counter table: one line per VL-ID that carried
    // traffic, with what was sent and what came back validated. Too long to
    // print during the run - roughly 4,400 of them - but it is the finest
    // grain the test produces, so it belongs with the run's other logs rather
    // than only on the server.
    const std::string local_vl_log =
        g_ReportManager.getTestLogDir() + "/VL_ID_Counters.log";
    if (g_ssh_deployer_server.fetchFile("/tmp/DTN_IRSW_EQ_VL_ID_Counters.log",
                                        local_vl_log))
    {
        std::cout << "DTN: VL-ID counter log saved to: " << local_vl_log << std::endl;
    }
    else
    {
        ErrorPrinter::warn("SSH",
            "DTN: Failed to fetch VL-ID counter log (file may not exist)");
    }

    // The switch's own counters, now that the traffic is over. They were
    // cleared before the test and read once straight after the configuration,
    // so that reading is the baseline and this one is the test: the difference
    // between the two is what the switch says it forwarded, from a third party
    // that is neither end of the link. Taken after dpdk has stopped so nothing
    // is still in flight, and best-effort - a switch that will not answer must
    // not fail a test whose traffic has already been counted.
    const std::string end_of_test_log =
        g_ReportManager.getTestLogDir() + "/End_Of_DTN_Test_Log.log";
    if (g_cumulus.saveCounterReport(end_of_test_log,
                                    "END OF DTN TEST - CUMULUS INTERFACE COUNTERS"))
    {
        std::cout << "DTN: End-of-test switch counters saved to: "
                  << end_of_test_log << std::endl;
    }
    else
    {
        ErrorPrinter::warn("CUMULUS",
            "DTN: End-of-test counter log could not be written.");
    }

    // And the two direct NICs, against the baseline taken after the config.
    saveServerNicCounterReport(
        g_ReportManager.getTestLogDir() + "/End_Of_DTN_Test_Server_NIC_Log.log",
        "END OF DTN TEST - SERVER NIC COUNTERS",
        /*is_baseline=*/false);

    // Disable PSU output
    if (!g_DeviceManager.enableOutput(PSUG30, false))
    {
        ErrorPrinter::error("PSU", "DTN: Failed to disable output on PSU G30!");
        shutdown.executeShutdown();
        return false;
    }

    // Verify PSU output is actually off, retry if not
    for (int retry = 0; retry < 3; retry++)
    {
        usleep(500000);
        if (!g_DeviceManager.isOutputEnabled(PSUG30))
        {
            std::cout << "DTN: PSU G30 output verified OFF." << std::endl;
            break;
        }
        ErrorPrinter::warn("PSU", "DTN: PSU G30 still ON, retry " + std::to_string(retry + 1) + "/3...");
        g_DeviceManager.enableOutput(PSUG30, false);
    }
    shutdown.unregisterPsuOutputEnabled(PSUG30);

    // Record Power Off Time when PSU output is disabled
    g_ReportManager.recordPowerOffTime();

    if (!g_DeviceManager.disconnect(PSUG30))
    {
        ErrorPrinter::error("PSU", "DTN: Failed to disconnect PSU G30!");
        shutdown.executeShutdown();
        return false;
    }
    shutdown.unregisterPsuConnected(PSUG30);

    // g_Server.offWithWait(300);

    std::cout << "DTN: PSU configured successfully." << std::endl;
    return true;
}
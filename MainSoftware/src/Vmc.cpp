#include "Vmc.h"
#include "ReportManager.h"
#include "SafeShutdown.h"
#include "ErrorPrinter.h"
#include "CumulusHelper.h"
#include "SSHDeployer.h"
#include "Dtn.h"
#include "PsuTelemetry.h"
#include "PsuTelemetryPublisher.h"
#include "Utils.h"
#include <iostream>
#include <unistd.h>
#include <iomanip>
#include <filesystem>
#include <csignal>
#include <atomic>
#include <cstdio>
#include <termios.h>
// 28V 3.0A

// Global flag for Ctrl+C handling in DPDK VMC monitoring
static std::atomic<bool> g_dpdk_vmc_monitoring_running{true};

static void dpdk_vmc_monitor_signal_handler(int sig)
{
    (void)sig;
    g_dpdk_vmc_monitoring_running = false;
}

Vmc g_vmc;

Vmc::Vmc()
{
}

Vmc::~Vmc()
{
}

bool Vmc::ensureLogDirectories()
{
    try
    {
        std::filesystem::create_directories(LogPaths::VMC());
        DEBUG_LOG("VMC: Log directories created/verified at " << LogPaths::VMC());
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "VMC: Failed to create log directories: " << e.what() << std::endl;
        return false;
    }
}

bool Vmc::runDpdkVmcInteractive(const std::string &eal_args, const std::string &make_args)
{
    std::cout << "======================================" << std::endl;
    std::cout << "VMC: DPDK VMC Interactive Deployment" << std::endl;
    std::cout << "======================================" << std::endl;

    // Step 1: Test connection
    if (!g_ssh_deployer_server.testConnection())
    {
        std::cerr << "VMC: Cannot connect to server!" << std::endl;
        return false;
    }

    // Step 2: Deploy prebuilt DPDK VMC binary (no compilation on server)
    DEBUG_LOG("VMC: Deploying prebuilt DPDK VMC binary...");
    if (!g_ssh_deployer_server.deployPrebuilt(
            "dpdk_vmc",  // prebuilt folder (inside prebuilt/)
            "",          // app name (auto-detect: dpdk_app)
            false,       // DON'T run after deploy (we'll run interactively)
            false,       // no sudo for deploy
            "",          // no run args (not running yet)
            false        // not background
            ))
    {
        std::cerr << "VMC: DPDK VMC prebuilt deploy failed!" << std::endl;
        return false;
    }

    // Step 3: Run DPDK VMC interactively
    // User can answer y/n prompts for latency tests
    // After tests complete, DPDK VMC will fork to background automatically
    std::cout << std::endl;
    std::cout << "======================================" << std::endl;
    DEBUG_LOG("VMC: Starting DPDK VMC Interactive Mode");
    DEBUG_LOG("VMC: You can answer latency test prompts (y/n)");
    DEBUG_LOG("VMC: After tests, DPDK VMC will continue in background");
    std::cout << "======================================" << std::endl;
    std::cout << std::endl;

    std::string remote_dir = g_ssh_deployer_server.getRemoteDirectory();

    std::string dpdk_command = "cd " + remote_dir + "/dpdk_vmc && "
                                                    "echo 'q' | sudo -S -v && "
                                                    "sudo ./dpdk_app --daemon " +
                               eal_args;

    bool result = g_ssh_deployer_server.executeInteractive(dpdk_command, false);

    if (result)
    {
        std::cout << std::endl;
        std::cout << "======================================" << std::endl;
        std::cout << "VMC: DPDK VMC started successfully!" << std::endl;
        std::cout << "VMC: Running in background on server" << std::endl;
        std::cout << "VMC: Log file: /tmp/dpdk_app.log" << std::endl;
        std::cout << "======================================" << std::endl;
    }
    else
    {
        std::cerr << "VMC: DPDK VMC interactive execution failed!" << std::endl;
    }

    return result;
}

bool Vmc::configureSequence()
{
    auto& shutdown = SafeShutdown::getInstance();

    if (!g_Server.onWithWait(3))
    {
        ErrorPrinter::error("SERVER", "VMC: Server could not be started!");
        shutdown.executeShutdown();
        return false;
    }
    shutdown.registerServerOn();

    // Create PSU G30 (30V, 56A)
    if (!g_DeviceManager.create(PSUG30))
    {
        ErrorPrinter::error("PSU", "VMC: Failed to create PSU G30!");
        shutdown.executeShutdown();
        return false;
    }

    // Connect to PSU G30
    if (!g_DeviceManager.connect(PSUG30))
    {
        ErrorPrinter::error("PSU", "VMC: Failed to connect to PSU G30!");
        shutdown.executeShutdown();
        return false;
    }
    shutdown.registerPsuConnected(PSUG30);

    if (!g_DeviceManager.setCurrent(PSUG30, 5.0))
    {
        ErrorPrinter::error("PSU", "VMC: Failed to set current on PSU G30!");
        shutdown.executeShutdown();
        return false;
    }

    if (!g_DeviceManager.setVoltage(PSUG30, 28.0))
    {
        ErrorPrinter::error("PSU", "VMC: Failed to set voltage on PSU G30!");
        shutdown.executeShutdown();
        return false;
    }

    serial::sendSerialCommand("/dev/ttyACM0", "VMC_ID 27");

    if (!g_DeviceManager.enableOutput(PSUG30, true))
    {
        ErrorPrinter::error("PSU", "VMC: Failed to enable output on PSU G30!");
        shutdown.executeShutdown();
        return false;
    }
    shutdown.registerPsuOutputEnabled(PSUG30);

    // Record Unit Power On Time when PSU output is enabled
    g_ReportManager.recordUnitPowerOnTime();

    // Start PSU telemetry publisher (same mechanism as DTN mode):
    // push PSU V/I/W over UDP to the server-side DPDK VMC app so the
    // health dashboard can show live PSU telemetry and detect staleness.
    PsuTelemetryPublisher psu_publisher(
        PSUG30,
        g_ssh_deployer_server.getHost(),
        PSU_TELEM_PORT);
    if (!psu_publisher.start()) {
        ErrorPrinter::warn("PSU-TELEM",
            "VMC: Failed to start PSU telemetry publisher - "
            "DPDK VMC will continue without PSU telemetry rows.");
    }

    sleep(30);

    sleep(2);
    if (!g_cumulus.deployNetworkInterfaces(SSHDeployer::getPrebuiltRoot() + "/CumulusInterfaces/VMC/interfaces"))
    {
        ErrorPrinter::error("CUMULUS", "VMC: Failed to deploy network configuration!");
        shutdown.executeShutdown();
        return false;
    }
    DEBUG_LOG("VMC: Network configuration deployed successfully.");

    sleep(1);

    // Configure Cumulus switch VLANs (VMC-specific)
    if (!g_cumulus.configureSequenceVmc())
    {
        ErrorPrinter::error("CUMULUS", "VMC: Cumulus configuration failed!");
        shutdown.executeShutdown();
        return false;
    }

    sleep(1);

    // Health-monitor handshake gate before starting the DPDK test machinery.
    // test_starter (VMC) runs on the ATE server, listens on ens1f0np0 (still
    // kernel-owned at this point — DPDK VMC hasn't taken it yet) over a raw
    // socket: it waits for CBIT reports from the DUT, sends PBIT requests,
    // collects the PBIT responses, runs the acceptance checks and reports its
    // result on stdout. The whole handshake must complete within this timeout;
    // otherwise the test fails and everything is shut down.
    // {
    //     const int test_starter_timeout_s = 600;

    //     // Remove any stale PBIT snapshot files from a previous run so the
    //     // dashboard can only ever show this run's data. test_starter rewrites
    //     // them on success; dpdk_vmc preloads them at startup.
    //     g_ssh_deployer_server.execute(
    //         "rm -f /tmp/vmc_pbit_flcs.bin /tmp/vmc_pbit_vs.bin",
    //         nullptr, false, true);

    //     DEBUG_LOG("VMC: Deploying test_starter on server...");
    //     if (!g_ssh_deployer_server.deployPrebuilt(
    //             "test_starter_vmc",
    //             "test_starter",
    //             /*run_after_deploy=*/false,
    //             /*use_sudo=*/false,
    //             /*run_args=*/"",
    //             /*run_in_background=*/false))
    //     {
    //         ErrorPrinter::error("TEST_STARTER", "VMC: Failed to deploy test_starter!");
    //         // shutdown.executeShutdown();
    //         return false;
    //     }

    //     std::string remote_dir = g_ssh_deployer_server.getRemoteDirectory();
    //     // No PTY over this SSH path, so pipe the password straight into
    //     // sudo -S on the test_starter invocation (same pattern as CMC).
    //     // Pipe through `stdbuf -oL tee` so (a) output is line-buffered all the
    //     // way to the PC (no stdio full-buffering on the SSH pipe) and (b) the
    //     // full log is also written on the SERVER at /tmp/test_starter_vmc.log,
    //     // so it can be inspected there independently of the PC stream.
    //     std::string cmd = "cd " + remote_dir + "/test_starter_vmc && "
    //                       "echo 'q' | sudo -S ./test_starter "
    //                       "--interface=ens1f0np0 --timeout="
    //                       + std::to_string(test_starter_timeout_s)
    //                       + " 2>&1 | stdbuf -oL tee /tmp/test_starter_vmc.log";

    //     std::cout << "VMC: Waiting for health-monitor handshake ("
    //               << test_starter_timeout_s << "s timeout)..." << std::endl;

    //     const int ssh_timeout_ms = (test_starter_timeout_s + 30) * 1000;
    //     std::string out;
    //     // Stream test_starter's progress live (step markers, per-packet CBIT
    //     // counts, PBIT responses, check results) while capturing it so we can
    //     // parse the final result line.
    //     std::cout << "------------- test_starter (VMC) live output -------------" << std::endl;
    //     g_ssh_deployer_server.executeStreaming(cmd, &out, false, ssh_timeout_ms);
    //     std::cout << "----------------------------------------------------------" << std::endl;

    //     bool got_ok       = out.find("TEST_STARTER_RESULT=OK")           != std::string::npos;
    //     bool got_bm_fault = out.find("TEST_STARTER_RESULT=BM_FLAG_FAULT") != std::string::npos;

    //     if (!got_ok && !got_bm_fault)
    //     {
    //         // Timeout, failed hard check, error, or unrecognized output is a
    //         // hard failure: stop everything, no operator override.
    //         ErrorPrinter::error("TEST_STARTER",
    //             "VMC: Health-monitor handshake did not pass within "
    //             + std::to_string(test_starter_timeout_s)
    //             + "s — failing test and shutting down.");
    //         //shutdown.executeShutdown();
    //         return true;
    //     }

    //     if (got_bm_fault)
    //     {
    //         // Hard checks passed, but the BM flag reports faults. These are
    //         // usually non-blocking, so let the operator decide whether to
    //         // proceed. Pull the detail summary the starter emitted.
    //         std::string detail;
    //         const std::string tag = "TEST_STARTER_BM_FLAG=";
    //         auto p = out.find(tag);
    //         if (p != std::string::npos)
    //         {
    //             detail = out.substr(p + tag.size());
    //             detail = detail.substr(0, detail.find('\n'));
    //             if (!detail.empty() && detail.back() == '\r') detail.pop_back();
    //         }
    //         ErrorPrinter::warn("TEST_STARTER",
    //             "VMC: BM flag reports fault(s): "
    //             + (detail.empty() ? std::string("(see test_starter output above)") : detail));

    //         // Talk to /dev/tty directly (same rationale as Cmc.cpp): the SSH
    //         // streaming above leaves std::cin's view of the keyboard unreliable,
    //         // while /dev/tty always points at the controlling terminal.
    //         std::string ans;
    //         FILE *tty_in  = fopen("/dev/tty", "r");
    //         FILE *tty_out = fopen("/dev/tty", "w");
    //         if (tty_in && tty_out)
    //         {
    //             tcflush(fileno(tty_in), TCIFLUSH);
    //             fprintf(tty_out, "VMC: BM flag reports faults. Continue test anyway? [y/N]: ");
    //             fflush(tty_out);
    //             char buf[64];
    //             if (fgets(buf, sizeof(buf), tty_in))
    //             {
    //                 ans = buf;
    //                 if (!ans.empty() && ans.back() == '\n') ans.pop_back();
    //             }
    //         }
    //         else
    //         {
    //             std::cin.clear();
    //             std::cout << "VMC: BM flag reports faults. Continue test anyway? [y/N]: " << std::flush;
    //             std::getline(std::cin, ans);
    //         }
    //         if (tty_in)  fclose(tty_in);
    //         if (tty_out) fclose(tty_out);

    //         if (ans.empty() || (ans[0] != 'y' && ans[0] != 'Y'))
    //         {
    //             std::cout << "VMC: Aborting at operator request (BM flag faults)." << std::endl;
    //             // shutdown.executeShutdown();
    //             return false;
    //         }
    //         std::cout << "VMC: Continuing despite BM flag faults (operator override)." << std::endl;
    //     }
    //     else
    //     {
    //         std::cout << "VMC: Health-monitor handshake complete, proceeding to test." << std::endl;
    //     }
    // }

    sleep(1);

    // DPDK VMC - Interactive mode with embedded latency test
    if (!runDpdkVmcInteractive("-l 0-255 -n 16"))
    {
        ErrorPrinter::error("DPDK", "VMC: DPDK VMC deployment unsuccessful!");
        shutdown.executeShutdown();
        return false;
    }
    shutdown.registerDpdkRunning();

    // DPDK VMC is now running in background on server
    std::cout << "VMC: DPDK VMC is running in background, continuing..." << std::endl;

    // Monitor DPDK VMC stats every 10 seconds until Ctrl+C
    std::cout << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "VMC: Monitoring DPDK VMC (every 10 seconds)" << std::endl;
    std::cout << "VMC: Press Ctrl+C to stop" << std::endl;
    std::cout << "======================================" << std::endl;

    // Setup signal handler for Ctrl+C (DPDK VMC monitoring only)
    g_dpdk_vmc_monitoring_running = true;
    struct sigaction sa;
    sa.sa_handler = dpdk_vmc_monitor_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);

    while (g_dpdk_vmc_monitoring_running)
    {
        // Wait 10 seconds (check flag each second)
        for (int i = 0; i < 10 && g_dpdk_vmc_monitoring_running; i++)
        {
            sleep(1);
        }

        if (!g_dpdk_vmc_monitoring_running)
            break;

        std::string raw;
        // tail -n 4000: dpdk_vmc her saniye bir stats tablosu (~50 satır) +
        // health-monitor dashboard (~1400 satır) yazıyor. 300 satır tek bir
        // HM tick'ine bile yetmiyordu; 4000 ile 2-3 tam cycle yakalanır ve
        // rfind iki stats marker'ını bulabilir.
        g_ssh_deployer_server.execute(
            "tail -n 4000 /tmp/dpdk_app.log", &raw, false, true);

        if (!raw.empty())
        {
            // Check if DPDK VMC has exited or is shutting down
            if (raw.find("Application exited cleanly") != std::string::npos ||
                raw.find("=== Shutting down ===") != std::string::npos)
            {
                std::cout << "\033[2J\033[H";
                std::cout << "======================================" << std::endl;
                std::cout << "VMC: DPDK VMC has exited on server." << std::endl;
                std::cout << "VMC: Auto-exiting monitoring loop..." << std::endl;
                std::cout << "======================================" << std::endl;
                g_dpdk_vmc_monitoring_running = false;
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
            std::cout << "=== DPDK VMC Live Stats (Press Ctrl+C to stop) ===" << std::endl;
            std::cout << block << std::endl;
        }
        else
        {
            DEBUG_LOG("(No log output yet - DPDK VMC might still be starting)");
        }
    }

    // Re-install SafeShutdown signal handlers after DPDK VMC monitoring
    shutdown.installSignalHandlers();

    // Stop DPDK VMC on server
    std::cout << "VMC: Stopping DPDK VMC on server..." << std::endl;
    if (g_ssh_deployer_server.isApplicationRunning("dpdk_app"))
    {
        g_ssh_deployer_server.stopApplication("dpdk_app", true);
        std::cout << "VMC: DPDK VMC stopped." << std::endl;
    }
    else
    {
        DEBUG_LOG("VMC: DPDK VMC was not running.");
    }
    shutdown.unregisterDpdkRunning();

    // Fetch DPDK VMC log from server to local PC
    DEBUG_LOG("VMC: Fetching DPDK VMC log from server...");
    ensureLogDirectories();
    std::string local_dpdk_log = g_ReportManager.getTestLogFilePath();
    if (g_ssh_deployer_server.fetchFile("/tmp/dpdk_app.log", local_dpdk_log))
    {
        std::cout << "VMC: DPDK VMC log saved to: " << local_dpdk_log << std::endl;
    }
    else
    {
        ErrorPrinter::warn("SSH", "VMC: Failed to fetch DPDK VMC log (file may not exist)");
    }

    if (!g_DeviceManager.enableOutput(PSUG30, false))
    {
        ErrorPrinter::error("PSU", "VMC: Failed to disable output on PSU G30!");
        shutdown.executeShutdown();
        return false;
    }

    // Verify PSU output is actually off, retry if not
    for (int retry = 0; retry < 3; retry++)
    {
        usleep(500000); // 500ms wait before checking
        if (!g_DeviceManager.isOutputEnabled(PSUG30))
        {
            std::cout << "VMC: PSU G30 output verified OFF." << std::endl;
            break;
        }
        ErrorPrinter::warn("PSU", "VMC: PSU G30 still ON, retry " + std::to_string(retry + 1) + "/3...");
        g_DeviceManager.enableOutput(PSUG30, false);
    }
    shutdown.unregisterPsuOutputEnabled(PSUG30);

    // Record Power Off Time when PSU output is disabled
    g_ReportManager.recordPowerOffTime();

    if (!g_DeviceManager.disconnect(PSUG30))
    {
        ErrorPrinter::error("PSU", "VMC: Failed to disconnect PSU G30!");
        shutdown.executeShutdown();
        return false;
    }
    shutdown.unregisterPsuConnected(PSUG30);

    // g_Server.offWithWait(300);
    // shutdown.unregisterServerOn();

    std::cout << "VMC: PSU configured successfully." << std::endl;
    return true;
}
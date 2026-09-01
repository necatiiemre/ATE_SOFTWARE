#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "Config.h"  // RX_DRAIN_SECONDS, TEST_START_QUIET_SECONDS
#include "Helpers.h" // helper_reset_stats, helper_print_stats, signal_handler / force_quit
#include "PortManager.h"
#include "EalInit.h"
#include "Socket.h"
#include "Packet.h"
#include "TxRxManager.h"
#include "RawSocketPort.h"  // Raw socket port support (non-DPDK NICs)
#include "DpdkExternalTx.h" // DPDK External TX (independent system)
#include "EmbeddedLatency/EmbeddedLatency.h"  // Embedded HW timestamp latency test
#include "PtpSlave.h"        // PTP slave for IEEE 1588v2 synchronization
#include "HealthMonitor.h"   // Health monitor for DTN status queries
#include "PsuTelemetry.h"          // wire format (shared with MainSoftware)
#include "PsuTelemetryReceiver.h"  // receiver API for MainSoftware UDP pushes
#include "ShutdownSnapshot.h"      // dump last-second stats on Ctrl+C

// Config.h is the authority for this; the fallback only covers a build tree
// whose Config.h predates it, which is otherwise a confusing "undeclared
// identifier" on a plain integer constant. Keep the two values in step.
#ifndef TEST_START_QUIET_SECONDS
#define TEST_START_QUIET_SECONDS 3
#endif

// Enable/disable raw socket ports
#ifndef ENABLE_RAW_SOCKET_PORTS
#define ENABLE_RAW_SOCKET_PORTS 1
#endif

// Enable/disable embedded HW timestamp latency test (runs BEFORE DPDK EAL init)
#ifndef EMBEDDED_HW_LATENCY_TEST
#define EMBEDDED_HW_LATENCY_TEST 1
#endif

// Check if --daemon flag is present and remove it from argv
// Returns true if --daemon was found, also updates argc
static bool check_and_remove_daemon_flag(int *argc, char const *argv[]) {
    bool found = false;
    int new_argc = 0;

    for (int i = 0; i < *argc; i++) {
        if (strcmp(argv[i], "--daemon") == 0 || strcmp(argv[i], "-d") == 0) {
            found = true;
            // Skip this argument (don't copy to new position)
        } else {
            // Keep this argument
            argv[new_argc] = argv[i];
            new_argc++;
        }
    }

    *argc = new_argc;
    return found;
}

// Stop flag for the RX workers only. force_quit (Common.h) cuts TX, the health
// monitor, PTP and the PSU listener the moment Ctrl+C arrives; RX keeps running
// off this flag for RX_DRAIN_SECONDS afterwards so the packets still in flight
// are received and counted before the final table is rendered.
static volatile bool force_quit_rx = false;

// Expected 28V power status from MainSoftware (0xFF = check disabled)
static uint8_t g_expected_power_status = 0xFF;
static bool    g_power_status_check_enabled = false;

// Check if --expected-power-status <value> flag is present and remove it from argv
// Returns true if flag was found, stores value in g_expected_power_status
static bool check_and_remove_power_status_flag(int *argc, char const *argv[]) {
    bool found = false;
    int new_argc = 0;

    for (int i = 0; i < *argc; i++) {
        if (strcmp(argv[i], "--expected-power-status") == 0 && (i + 1) < *argc) {
            g_expected_power_status = (uint8_t)atoi(argv[i + 1]);
            g_power_status_check_enabled = true;
            found = true;
            i++; // Skip the value argument too
        } else {
            argv[new_argc] = argv[i];
            new_argc++;
        }
    }

    *argc = new_argc;
    return found;
}

// force_quit and signal_handler are typically declared/defined in helpers.h.
// If not present in your helpers.h, you can uncomment these lines:
// volatile bool force_quit = false;
// static void signal_handler(int sig) { (void)sig; force_quit = true; }

int main(int argc, char const *argv[])
{
    // Check for --daemon flag BEFORE anything else, and remove it from argv
    // so it doesn't confuse DPDK EAL argument parser
    bool daemon_mode = check_and_remove_daemon_flag(&argc, argv);

    // Check for --expected-power-status flag and remove from argv
    check_and_remove_power_status_flag(&argc, argv);

    // Set daemon mode flag for helper functions (disables ANSI escape codes in logs)
    helper_set_daemon_mode(daemon_mode);

    printf("=== DPDK TX/RX Application with PRBS-31 & Sequence Validation ===\n");
    if (daemon_mode) {
        printf("Mode: DAEMON (will fork to background after latency tests)\n");
    } else {
        printf("Mode: FOREGROUND (use --daemon for background mode)\n");
    }
    printf("TX Cores: %d | RX Cores: %d | VLAN: %s\n",
           NUM_TX_CORES, NUM_RX_CORES,
#if VLAN_ENABLED
           "Enabled"
#else
           "Disabled"
#endif
    );
    printf("PRBS Method: Sequence-based with ~268MB cache per port\n");
    printf("Payload format: [8-byte sequence][PRBS-31 data]\n");
    printf("WARM-UP: Disabled (stats are zeroed once, the test starts immediately)\n");
    printf("Sequence Validation: Enabled (Lost/Out-of-Order/Duplicate detection)\n");
#if ENABLE_RAW_SOCKET_PORTS
    printf("Raw Socket Ports: Enabled (%d ports, multi-target)\n", MAX_RAW_SOCKET_PORTS);
    printf("  - Port 12 (1G): 5 targets (960 Mbps total)\n");
    printf("      -> P13: 80 Mbps, P5/P4/P7/P6: 220 Mbps each\n");
    printf("  - Port 13 (100M): 1 target\n");
    printf("      -> P12: 80 Mbps\n");
#endif
#if EMBEDDED_HW_LATENCY_TEST
    printf("Embedded HW Latency Test: Enabled (runs before DPDK init)\n");
#endif
    printf("\n");

    // =========================================================================
    // EMBEDDED HW TIMESTAMP LATENCY TEST (runs BEFORE DPDK takes over NICs!)
    // Full sequence: Loopback (switch) + Unit Test (device) + Combined Results
    // =========================================================================
#if EMBEDDED_HW_LATENCY_TEST
    // Full interactive sequence:
    // 1. Loopback test (Mellanox switch latency) - or use default 14µs
    // 2. Unit test (device latency) - port pairs 0↔1, 2↔3, 4↔5, 6↔7
    // 3. Combined results: unit_latency = total - switch
    int latency_fails = emb_latency_full_sequence();

    // Load appropriate VLAN config based on ATE mode selection
    port_vlans_load_config(ate_mode_enabled());

    if (emb_latency_completed()) {
        if (latency_fails > 0) {
            printf("\n*** WARNING: %d test(s) failed! ***\n\n", latency_fails);
        } else {
            printf("\n*** All latency tests PASSED! ***\n\n");
        }

        // Example: Get all latency values for port pair 0-1
        // double switch_us, total_us, unit_us;
        // if (emb_latency_get_all_us(0, &switch_us, &total_us, &unit_us)) {
        //     printf("Port 0-1: Switch=%.2f µs, Total=%.2f µs, Unit=%.2f µs\n",
        //            switch_us, total_us, unit_us);
        // }

        printf("=== Latency test sequence complete ===\n");

        // daemon_mode was already set at the start of main()
        // If yes: fork to background (for remote execution from main PC)
        // If no: continue in foreground (for direct server execution)
        if (daemon_mode) {
            printf("=== Switching to background mode for DPDK operation ===\n\n");
            fflush(stdout);
            fflush(stderr);

            // Fork to background: Parent exits (SSH closes), Child continues
            pid_t pid = fork();
            if (pid < 0) {
                // Fork failed
                perror("fork failed");
                printf("Continuing in foreground mode...\n");
            } else if (pid > 0) {
                // Parent process - exit so SSH connection closes
                printf("DPDK continuing in background (PID: %d)\n", pid);
                printf("Log file: /tmp/dpdk_app.log\n");
                printf("To monitor: ssh user@server 'tail -f /tmp/dpdk_app.log'\n");
                printf("To stop: ssh user@server 'sudo pkill -f dpdk_app'\n");
                fflush(stdout);
                _exit(0);  // Use _exit to avoid flushing stdio buffers twice
            } else {
                // Child process - continue running DPDK in background
                // Create new session to detach from terminal
                setsid();

                // Redirect stdout/stderr to log file
                int log_fd = open("/tmp/dpdk_app.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (log_fd >= 0) {
                    dup2(log_fd, STDOUT_FILENO);
                    dup2(log_fd, STDERR_FILENO);
                    close(log_fd);
                }

                // Close stdin
                close(STDIN_FILENO);
                open("/dev/null", O_RDONLY);

                printf("\n=== DPDK Background Mode Started (PID: %d) ===\n", getpid());
                printf("Initializing DPDK EAL...\n\n");

                // Re-print embedded latency results to log file
                // (They were printed to terminal before fork, now save to log)
                printf("=== Embedded Latency Test Results (from interactive session) ===\n");
                emb_latency_print_loopback();
                emb_latency_print_unit();
                if (latency_fails > 0) {
                    printf("WARNING: %d test(s) failed!\n", latency_fails);
                } else {
                    printf("All latency tests PASSED!\n");
                }
                printf("=== End of Latency Results ===\n\n");

                fflush(stdout);
            }
        } else {
            // Foreground mode - continue normally
            printf("=== Continuing in foreground mode ===\n\n");
        }
    } else {
        printf("=== Latency test skipped, initializing DPDK ===\n\n");
    }
#endif

    // Initialize DPDK EAL
    initialize_eal(argc, argv);

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Remove any stale summary log from a previous run so that, if this run
    // ends without producing a fresh one (e.g. a crash), MainSoftware won't
    // fetch old data. dpdk runs as root, so it can delete the root-owned file.
    remove(SHUTDOWN_SNAPSHOT_PATH);

    // Print basic EAL info
    print_eal_info();

    // Initialize ports
    int nb_ports = initialize_ports(&ports_config);
    if (nb_ports < 0)
    {
        printf("Error: Failed to initialize ports\n");
        cleanup_eal();
        return -1;
    }

    printf("Found %d ports\n", nb_ports);

    // Setup port configuration
    set_manual_pci_addresses(&ports_config);
    portNumaNodesMatch(&ports_config);

    // Setup socket to lcore mapping
    socketToLcore();

    // Assign lcores to ports
    lcorePortAssign(&ports_config);

    // Initialize VLAN configuration + print
    init_vlan_config();
    print_vlan_config();

    // Initialize RX verification stats (PRBS good/bad/bit_errors + sequence stats)
    init_rx_stats();

#if STATS_MODE_DTN
    // Initialize DTN port mapping and statistics
    init_dtn_port_map();
    init_dtn_stats();
#endif

    // *** PRBS-31 CACHE INITIALIZATION ***
    printf("\n=== Initializing PRBS-31 Cache ===\n");
    printf("This will take a few minutes as we generate ~%u MB per port...\n",
           (unsigned)(PRBS_CACHE_SIZE / (1024 * 1024)));

    init_prbs_cache_for_all_ports((uint16_t)nb_ports, &ports_config);

    printf("PRBS-31 cache initialization complete!\n\n");

    // Configure TX/RX for each port
    printf("\n=== Configuring Ports ===\n");
    struct txrx_config txrx_configs[MAX_PORTS];

    for (uint16_t i = 0; i < (uint16_t)nb_ports; i++)
    {
        uint16_t port_id = ports_config.ports[i].port_id;
        uint16_t socket_id = ports_config.ports[i].numa_node;

        // Create mbuf pool
        struct rte_mempool *mbuf_pool = create_mbuf_pool(socket_id, port_id);
        if (mbuf_pool == NULL)
        {
            printf("Failed to create mbuf pool for port %u\n", port_id);
            cleanup_prbs_cache();
            cleanup_ports(&ports_config);
            cleanup_eal();
            return -1;
        }

        // Setup TX/RX configuration
        txrx_configs[i].port_id = port_id;

        // Calculate number of TX queues needed
        // Base: NUM_TX_CORES (0 to NUM_TX_CORES-1)
        // External TX: +1 queue (queue 4)
        // PTP: +1 queue (queue 5)
        uint16_t num_tx_queues = NUM_TX_CORES;

#if DPDK_EXT_TX_ENABLED
        // External TX ports need an extra queue (queue 4) for external TX
        // Port 2,3,4,5 → Port 12 | Port 0,6 → Port 13
        bool is_ext_tx_port = (port_id == 0 || port_id == 2 || port_id == 3 ||
                               port_id == 4 || port_id == 5 || port_id == 6);
        if (is_ext_tx_port) {
            num_tx_queues = NUM_TX_CORES + 1;  // Extra queue for external TX (queue 4)
        }
#endif

#if PTP_ENABLED
        // PTP needs queue 5 for TX on all ports
        // Queue 5 comes after external TX queue 4
        num_tx_queues = (num_tx_queues < 6) ? 6 : num_tx_queues;  // Ensure queue 5 exists
#endif

        txrx_configs[i].nb_tx_queues = num_tx_queues;

        // Calculate number of RX queues needed
        // Base: NUM_RX_CORES (0 to NUM_RX_CORES-1)
        // PTP: +1 queue (queue 5)
        uint16_t num_rx_queues = NUM_RX_CORES;

#if PTP_ENABLED
        // PTP needs queue 5 for RX on all ports
        num_rx_queues = (num_rx_queues < 6) ? 6 : num_rx_queues;  // Ensure queue 5 exists
#endif

        txrx_configs[i].nb_rx_queues = num_rx_queues;
        txrx_configs[i].mbuf_pool = mbuf_pool;

        // Initialize port TX/RX
        int ret = init_port_txrx(port_id, &txrx_configs[i]);
        if (ret < 0)
        {
            printf("Failed to initialize TX/RX for port %u\n", port_id);
            cleanup_prbs_cache();
            cleanup_ports(&ports_config);
            cleanup_eal();
            return -1;
        }
    }

    print_ports_info(&ports_config);

    printf("All ports configured\n");

#if ENABLE_RAW_SOCKET_PORTS
    // *** RAW SOCKET PORTS INITIALIZATION ***
    // Load ATE or normal config before initializing ports
    raw_socket_ports_load_config(ate_mode_enabled());

    printf("\n=== Initializing Raw Socket Ports (Non-DPDK) ===\n");
    printf("These ports use AF_PACKET with zero-copy (PACKET_MMAP)\n");
    printf("VLAN header: Disabled for raw socket ports\n\n");

    bool raw_ports_initialized = false;
    int raw_ret = init_raw_socket_ports();
    if (raw_ret < 0)
    {
        printf("Warning: Failed to initialize raw socket ports\n");
        printf("Continuing with DPDK ports only...\n");
    }
    else
    {
        printf("Raw socket ports initialized successfully\n");
        raw_ports_initialized = true;
    }
#endif

#if DPDK_EXT_TX_ENABLED
    if (!ate_mode_enabled()) {
        // *** DPDK EXTERNAL TX INITIALIZATION (BEFORE start_txrx_workers!) ***
        // Must be called before start_txrx_workers so ext_tx_enabled can be set
        printf("\n=== Initializing DPDK External TX System ===\n");

        // Gather mbuf pools for external TX ports
        // Port order in ext_tx_configs: Port 2,3,4,5 (→P12), Port 0,6 (→P13)
        static struct dpdk_ext_tx_port_config ext_configs[] = DPDK_EXT_TX_PORTS_CONFIG_INIT;
        struct rte_mempool *ext_mbuf_pools[DPDK_EXT_TX_PORT_COUNT];
        for (int i = 0; i < DPDK_EXT_TX_PORT_COUNT; i++) {
            uint16_t port_id = ext_configs[i].port_id;
            if (port_id < nb_ports) {
                ext_mbuf_pools[i] = txrx_configs[port_id].mbuf_pool;
                printf("  Ext TX Port %u: mbuf_pool from txrx_configs[%u]\n", port_id, port_id);
            } else {
                ext_mbuf_pools[i] = NULL;
                printf("  Ext TX Port %u: mbuf_pool = NULL (port_id >= nb_ports)\n", port_id);
            }
        }

        if (dpdk_ext_tx_init(ext_mbuf_pools) != 0) {
            printf("Warning: DPDK External TX initialization failed\n");
        }
    } else {
        printf("\n=== DPDK External TX DISABLED (ATE mode) ===\n");
    }
#endif

    // Start TX/RX workers
    printf("\n=== Starting Workers ===\n");
    printf("Configuration Check:\n");
    printf("  Ports detected: %d\n", nb_ports);
    printf("  TX cores per port: %d\n", NUM_TX_CORES);
    printf("  RX cores per port: %d\n", NUM_RX_CORES);
    printf("  Expected TX workers: %d\n", nb_ports * NUM_TX_CORES);
    printf("  Expected RX workers: %d\n", nb_ports * NUM_RX_CORES);
    printf("  PRBS-31 cache: Ready (~%.2f GB total)\n",
           (nb_ports * PRBS_CACHE_SIZE) / (1024.0 * 1024.0 * 1024.0));
    printf("  Payload per packet: %u bytes (SEQ: %u + PRBS: %u)\n",
           PAYLOAD_SIZE, SEQ_BYTES, NUM_PRBS_BYTES);
    printf("  Sequence Validation: ENABLED\n");
#if LATENCY_TEST_ENABLED
    printf("  Latency Test: ENABLED (will run before normal mode)\n");
#endif
    printf("\n");

#if LATENCY_TEST_ENABLED
    // *** LATENCY TEST - RUNS BEFORE NORMAL MODE ***
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║            LATENCY TEST MODE ENABLED                             ║\n");
    printf("║  1 packet will be sent from each VLAN, latency will be measured   ║\n");
    printf("║  Normal mode will resume after test                              ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    int latency_ret = start_latency_test(&ports_config, &force_quit);
    if (latency_ret < 0) {
        printf("Warning: Latency test failed, continuing with normal mode\n");
    }

    // Check if user pressed Ctrl+C during latency test
    if (force_quit) {
        printf("User interrupted during latency test, exiting...\n");
        cleanup_prbs_cache();
        cleanup_ports(&ports_config);
        cleanup_eal();
        return 0;
    }

    printf("\n=== Latency test complete, starting normal TX/RX workers ===\n\n");
#endif

#if PTP_ENABLED
    // Steer PTP to queue 5 BEFORE the RX workers start polling queues 0-3.
    // The rule used to be installed inside ptp_start(), which runs several
    // seconds later - after the raw socket workers, the health monitor and
    // external TX are all up. Every PTP frame arriving in that window had no
    // rule to match, fell back to RSS, landed on a PRBS queue and was counted
    // as test traffic. The rule itself needs nothing from the PTP context,
    // only a port id, so it can be installed this early; ptp_start() sees the
    // handles already in place and leaves them alone.
    if (!ate_mode_enabled()) {
        printf("\n=== Installing PTP flow rules (before RX workers start) ===\n");
        for (uint16_t i = 0; i < (uint16_t)nb_ports; i++) {
            ptp_flow_rule_install(ports_config.ports[i].port_id);
        }
    }
#endif

    int start_ret = start_txrx_workers(&ports_config, &force_quit, &force_quit_rx);
    if (start_ret < 0)
    {
        printf("Failed to start TX/RX workers\n");
        cleanup_prbs_cache();
        cleanup_ports(&ports_config);
        cleanup_eal();
        return -1;
    }

#if ENABLE_RAW_SOCKET_PORTS
    // Start raw socket workers (only if initialization succeeded)
    if (raw_ports_initialized)
    {
        printf("\n=== Starting Raw Socket Workers ===\n");
        start_ret = start_raw_socket_workers(&force_quit, &force_quit_rx);
        if (start_ret < 0)
        {
            printf("Warning: Failed to start raw socket workers\n");
            printf("Continuing with DPDK workers only...\n");
            raw_ports_initialized = false;  // Mark as not running
        }
        else
        {
            printf("Raw socket workers started successfully\n");
        }
    }

    // Runtime flags for ATE mode — used in both init and shutdown sections
    bool health_active = false;
    bool ptp_active = false;

    // Initialize and start Health Monitor (runs on Port 13, independent from PRBS)
    // Disabled in ATE mode via ATE_HEALTH_MONITOR_ENABLED flag
#if HEALTH_MONITOR_ENABLED
    {
        health_active = ate_mode_enabled() ? ATE_HEALTH_MONITOR_ENABLED : HEALTH_MONITOR_ENABLED;
        if (health_active) {
            printf("\n=== Initializing Health Monitor ===\n");
            if (init_health_monitor() == 0) {
                // Pass expected 28V power status to health monitor (if provided by MainSoftware)
                if (g_power_status_check_enabled) {
                    health_monitor_set_expected_power_status(g_expected_power_status);
                    printf("[HEALTH] 28V Power Status check enabled: expected=0x%02X (Primary=%s, Secondary=%s)\n",
                           g_expected_power_status,
                           (g_expected_power_status & 0x01) ? "FAIL" : "SUCCESS",
                           (g_expected_power_status & 0x02) ? "FAIL" : "SUCCESS");
                }
                // Watches force_quit_rx so it keeps polling the DTN through the
                // RX drain window; sets force_quit if it has to abort the test.
                if (start_health_monitor(&force_quit_rx, &force_quit) != 0) {
                    printf("Warning: Failed to start health monitor\n");
                }
            } else {
                printf("Warning: Failed to initialize health monitor\n");
            }
        } else {
            printf("\n[ATE] Health Monitor disabled (ATE_HEALTH_MONITOR_ENABLED=0)\n");
        }
    }
#endif

#endif

    // Start DPDK External TX workers AFTER raw socket workers
    // This ensures Port 12 RX is ready before receiving packets from Port 2,3,4,5
#if DPDK_EXT_TX_ENABLED
    if (!ate_mode_enabled()) {
        printf("\n=== Starting DPDK External TX Workers ===\n");
        printf("(Started after raw socket RX to prevent initial packet loss)\n");
        int ext_ret = dpdk_ext_tx_start_workers(&ports_config, &force_quit);
        if (ext_ret != 0)
        {
            printf("Error starting external TX workers: %d\n", ext_ret);
            // Continue anyway, this is not fatal
        }
    }
#endif

#if PTP_ENABLED
    // *** PTP SLAVE INITIALIZATION AND START ***
    // Disabled in ATE mode via ATE_PTP_ENABLED flag
    {
        ptp_active = ate_mode_enabled() ? ATE_PTP_ENABLED : PTP_ENABLED;
        if (ptp_active) {
            printf("\n=== Initializing PTP Slave (IEEE 1588v2) ===\n");
            printf("Mode: One-step | Transport: Layer 2 | Timestamps: Software (rte_rdtsc)\n");
            printf("Architecture: Split TX/RX Port Support\n\n");

            // Initialize PTP subsystem
            if (ptp_init() != 0) {
                printf("Warning: PTP initialization failed\n");
            } else {
                // Configure PTP sessions with split TX/RX port support
                // Sessions are defined in config.h with separate RX and TX ports
                static struct ptp_session_config ptp_sessions[] = PTP_SESSIONS_CONFIG_INIT;

                if (ptp_configure_split_sessions(ptp_sessions, PTP_SESSION_COUNT) != 0) {
                    printf("Warning: Failed to configure PTP sessions\n");
                } else {
                    // Assign PTP cores to RX ports (where sessions live)
                    // Each unique RX port needs a dedicated lcore
                    for (uint16_t i = 0; i < PTP_SESSION_COUNT; i++) {
                        uint16_t rx_port_id = ptp_sessions[i].rx_port_id;

                        // Find this port in ports_config and assign lcore
                        for (uint16_t j = 0; j < (uint16_t)nb_ports; j++) {
                            if (ports_config.ports[j].port_id == rx_port_id &&
                                ports_config.ports[j].used_ptp_core != 0) {
                                ptp_assign_lcore(rx_port_id, ports_config.ports[j].used_ptp_core);
                                break;
                            }
                        }
                    }

                    // Initialize raw socket PTP ports (Port 12/13)
                    // These ports use AF_PACKET instead of DPDK
                    {
                        ptp_context_t *ptp_ctx = ptp_get_context();
                        // Port 12
                        if (ptp_ctx->ports[12].enabled && ptp_ctx->ports[12].is_raw_socket) {
                            if (ptp_raw_socket_init(&ptp_ctx->ports[12], RAW_SOCKET_PORT_12_IFACE) != 0) {
                                printf("Warning: Failed to init raw socket PTP for Port 12\n");
                            }
                        }
                        // Port 13
                        if (ptp_ctx->ports[13].enabled && ptp_ctx->ports[13].is_raw_socket) {
                            if (ptp_raw_socket_init(&ptp_ctx->ports[13], RAW_SOCKET_PORT_13_IFACE) != 0) {
                                printf("Warning: Failed to init raw socket PTP for Port 13\n");
                            }
                        }
                    }

                    // Start PTP workers
                    printf("\n=== Starting PTP Workers ===\n");
                    if (ptp_start() != 0) {
                        printf("Warning: Failed to start PTP workers\n");
                    } else {
                        printf("PTP workers started (%d sessions with split TX/RX ports)\n",
                               PTP_SESSION_COUNT);
                    }
                }
            }
        } else {
            printf("\n[ATE] PTP disabled (ATE_PTP_ENABLED=0)\n");
        }
    }
#endif

    // Start PSU telemetry listener: receives 1 Hz V/I/W packets from
    // MainSoftware (PC) and makes them available to the health print cycle.
    // Failure here is non-fatal: the test can still run, DPDK just won't
    // show a power-supply row in the health table.
    // Bound to force_quit_rx, not force_quit: the health block the monitor
    // renders carries the PSU row, MainSoftware keeps publishing telemetry for
    // the whole drain, and a receiver that stopped at Ctrl+C would leave that
    // row dead in exactly the snapshot we care about.
    if (psu_telem_init(PSU_TELEM_PORT) == 0) {
        if (psu_telem_start(&force_quit_rx) != 0) {
            printf("Warning: PSU telemetry listener failed to start\n");
        }
    } else {
        printf("Warning: PSU telemetry listener init failed (port %u busy?)\n",
               (unsigned)PSU_TELEM_PORT);
    }

    printf("\n=== Running (Press Ctrl+C to stop) ===\n");
    printf("  No warm-up phase: second 1 of the table is second 1 of the test\n\n");

    // Previous TX/RX bytes for per-second rate calculation
    static uint64_t prev_tx_bytes[MAX_PORTS] = {0};
    static uint64_t prev_rx_bytes[MAX_PORTS] = {0};

    // There is no warm-up phase, so zero every counter right here instead.
    // Workers have been forwarding for a few seconds already (port bring-up,
    // raw sockets, health monitor, external TX, PTP, PSU listener); without
    // this reset that start-up traffic would be counted as test traffic.
    //
    // Resetting counters while traffic flows cannot be exact. They live in 32
    // TX lcores, 32 RX lcores, three pthreads and the NIC; zeroing them takes
    // time, and whatever is in the air during that window is counted on one
    // side only. Measured, that was the whole of the ~213 packet residue the
    // end-of-test totals carried - stable at 212/213/213 across 47, 73 and 98
    // second runs, because it is a fixed window and not a rate.
    //
    // So empty the pipe first. Hold every PRBS sender, wait long enough that
    // nothing can still be in flight (one flight time is tens of
    // microseconds; this waits four orders of magnitude longer), then reset
    // and release. The reset now happens against a stopped world.
    //
    // The quiet window is also the only moment at which this side and the
    // switch can be given a common zero point. The switch keeps its own
    // counters and they cannot be cleared in step with ours - the clear is an
    // SSH round trip, seconds long, and at two million packets a second
    // seconds of skew is millions of packets. Reading them while nothing is
    // moving removes the need for that precision entirely: any snapshot taken
    // between these two markers describes the same instant. The same holds at
    // the other end, after the RX drain.
    printf("\n[SYNC] Quiet window OPEN - all test traffic held, counters "
           "being zeroed.\n"
           "       Switch-side counters read between this line and the next "
           "one\n"
           "       share a zero point with ours.\n");
    fflush(stdout);

    txrx_set_tx_paused(true);
    sleep(TEST_START_QUIET_SECONDS);


    helper_reset_stats(&ports_config, prev_tx_bytes, prev_rx_bytes);

#if PTP_ENABLED
    if (ptp_active)
        ptp_reset_stats();
#endif

#if HEALTH_MONITOR_ENABLED
    // Arms the post-start cycle counter that gates the FW-version and 28V
    // power-status checks (they fire on its 10th cycle). Despite the name,
    // this now simply means "the test has started".
    if (health_active && is_health_monitor_running()) {
        health_monitor_set_warmup_complete();
    }
#endif

    // Counters are zero and nothing was in flight while they were zeroed.
    // Release the senders; from here the numbers are the test's.
    txrx_set_tx_paused(false);
    printf("[SYNC] Quiet window CLOSED - test traffic starts now.\n");

    // Log Test Start Time for PDF report
    {
        time_t now_t = time(NULL);
        struct tm *tm_info = localtime(&now_t);
        char time_buf[64];
        strftime(time_buf, sizeof(time_buf), "%B %d, %Y %H:%M:%S", tm_info);
        printf("Test Start Time : %s\n", time_buf);
    }
    fflush(stdout);

    // Main loop - print stats table every second. It does not end the moment
    // Ctrl+C arrives: force_quit stops the TX workers, and we then keep
    // rendering for RX_DRAIN_SECONDS while the RX workers collect whatever is
    // still in flight. Only after that window is the snapshot frozen and the
    // RX workers released.
    uint32_t test_time = 0;
    bool     draining = false;
    uint32_t drain_until = 0;

    // Diagnostic, off unless DTN_DEBUG_EXTRA_LOCAL_DROPS is set to a count.
    //
    // The end-of-test totals carry a residue of about 213 packets in the
    // direction of more returned than sent - stable at 212/213/213 across 47,
    // 73 and 98 second runs, so not a rate and not a random flush collision.
    // The explanation on offer is the counter reset: when
    // txrx_reset_worker_locals() bumps the generation, workers notice at
    // slightly different moments, so a packet sent just before its TX worker
    // notices has its send discarded while its arrival, a flight time later,
    // is counted by an RX worker already in the new window.
    //
    // If that is right, the residue is per generation bump and nothing else,
    // and repeating the bump must add another one each time. This does exactly
    // that and nothing else - it does not zero any counter, so everything
    // except the packets inside the window cancels: a send dropped from a TX
    // local has its arrival dropped from an RX local too.
    //
    // Set it to N and the final difference should come out near 213 * (1 + N).
    // If it does not move, the reset is not the cause and the explanation
    // above is wrong.
    unsigned extra_local_drops = 0;
    {
        const char *env = getenv("DTN_DEBUG_EXTRA_LOCAL_DROPS");
        if (env != NULL) {
            long v = strtol(env, NULL, 10);
            if (v > 0 && v < 1000) {
                extra_local_drops = (unsigned)v;
                printf("[DIAG] DTN_DEBUG_EXTRA_LOCAL_DROPS=%u - dropping worker\n"
                       "       locals once per second for the first %u seconds.\n"
                       "       Expect the final difference near %u x the usual\n"
                       "       residue if the reset boundary is its source.\n",
                       extra_local_drops, extra_local_drops,
                       extra_local_drops + 1);
            }
        }
    }

    while (1)
    {
        sleep(1);

        if (extra_local_drops > 0) {
            txrx_reset_worker_locals();
            extra_local_drops--;
        }

        // First second in which the stop request is visible: TX (DPDK,
        // external and raw socket) is already unwinding on force_quit, RX is
        // not - it runs off force_quit_rx, which stays false until the drain
        // window closes below.
        if (force_quit && !draining)
        {
            draining = true;
            drain_until = test_time + RX_DRAIN_SECONDS;
            printf("\n");
            printf("═══════════════════════════════════════════════════════════════\n");
            printf("   TX STOPPED - DRAINING RX FOR %u SECONDS\n",
                   (unsigned)RX_DRAIN_SECONDS);
            printf("   (in-flight packets are still being counted;\n");
            printf("    the health monitor keeps querying the DTN)\n");
            printf("═══════════════════════════════════════════════════════════════\n");
            printf("\n");

            // Wait for the TX workers to actually be gone before anything is
            // rendered. sleep() returns early on the signal that set
            // force_quit, so without this the first drain table is taken
            // microseconds after Ctrl+C, while forty senders are still on
            // their way out - their last packets sent but not yet handed over,
            // and the arrivals of those packets already counted on the RX
            // side. The table then shows more validated than sent, which is
            // what the per-second tables did: plus or minus one packet all run,
            // then plus six to eight in this one second, and frozen there for
            // the rest of the drain.
            //
            // They leave on force_quit, which is already set, so this returns
            // as soon as they are done rather than adding a wait of its own.
            txrx_wait_tx_workers();
        }

        test_time++;

        const bool final_second = (draining && test_time >= drain_until);

        // Ask the RX workers to hand over what they are holding before the
        // table is rendered, and wait for them to do it. Their counters are
        // thread-local and go out on a deadline, so without this every table -
        // including the last one - trails them by up to a flush period. Asking
        // makes the numbers current at the moment they are read, and keeps that
        // independent of when the workers happen to stop.
        txrx_flush_now(50);

        // Full table + queue distributions (includes DPDK External TX stats)
        helper_print_stats(&ports_config, prev_tx_bytes, prev_rx_bytes,
                           test_time);

#if ENABLE_RAW_SOCKET_PORTS
        // Print raw socket port stats (only if initialized)
        // In DTN mode raw socket table is not printed separately, DTN table is sufficient
#if !STATS_MODE_DTN
        if (raw_ports_initialized)
        {
            print_raw_socket_stats();
        }
#endif
#endif

#if PTP_ENABLED
        // Print PTP stats every second
        if (ptp_active)
            ptp_print_stats();
#endif

        fflush(stdout);  // Ensure output is visible on remote/main computer

        // Update prev_* for the NEXT second: (cumulative HW byte counters)
        // helper_print_stats calculates per-second rates based on prev_* difference.
        for (uint16_t i = 0; i < (uint16_t)nb_ports; i++)
        {
            uint16_t port_id = ports_config.ports[i].port_id;
            struct rte_eth_stats st;
            if (rte_eth_stats_get(port_id, &st) == 0)
            {
                prev_tx_bytes[port_id] = st.obytes;
                prev_rx_bytes[port_id] = st.ibytes;
            }
        }

        // Drain window is over and the table just rendered holds the settled
        // counters, so it is the one the summary log should keep. Stop the RX
        // workers on the way out - by now their counters are already folded in,
        // so their exit flush has nothing left to contribute.
        if (final_second) {
            force_quit_rx = true;
            txrx_wait_rx_workers();
            break;
        }
    }

    // txrx_wait_rx_workers() above waits on the DPDK RX lcores only. The raw
    // socket ports run their TX and RX on pthreads, and their counters feed
    // the same totals: tx_targets[].stats is the raw leg's send side and
    // dpdk_ext_rx_stats is the external leg's return side. Those threads were
    // signalled to stop but not joined until the teardown block far below, so
    // the totals used to be read while they could still be holding unflushed
    // counts - and a raw RX worker exits on force_quit_rx, set one statement
    // before this point, which made it a race rather than a rare case.
    //
    // Join them here instead. The call is idempotent, so the teardown block
    // keeps its call and simply finds the work already done.
#if ENABLE_RAW_SOCKET_PORTS
    if (raw_ports_initialized) {
        stop_raw_socket_workers();
    }
#endif

    printf("\n[SYNC] Quiet window OPEN - test traffic stopped and the drain is\n"
           "       complete, so nothing is in flight. Switch-side counters read\n"
           "       from here describe the same instant as the totals below;\n"
           "       subtract the reading taken at the start to get this test's\n"
           "       traffic as the switch saw it.\n");

    // Every worker that feeds these counters - DPDK lcores and raw socket
    // pthreads alike - has now exited and run its final flush.
    helper_print_final_totals(&ports_config, test_time);
    fflush(stdout);

    // Everything that belongs in the summary has been stored; stop the
    // per-second producers from overwriting it during teardown.
    shutdown_snapshot_freeze();

    // Dump the DTN table + Health Monitor + PSU block into a single file. The
    // DTN table is the one rendered on the loop's final iteration, after the RX
    // workers exited and flushed, so its counters include every packet that was
    // still in flight when Ctrl+C arrived. Done before any teardown prints so
    // the snapshot holds those settled values, not something re-rendered during
    // shutdown.
    {
        char note[96];
        snprintf(note, sizeof(note),
                 "stop requested at test second %u (after %u s RX drain)",
                 test_time, (unsigned)RX_DRAIN_SECONDS);
        if (shutdown_snapshot_dump(note) == 0) {
            printf("[SUMMARY] Last-second summary written to %s\n",
                   SHUTDOWN_SNAPSHOT_PATH);
        } else {
            printf("[SUMMARY] WARNING: failed to write %s\n",
                   SHUTDOWN_SNAPSHOT_PATH);
        }
    }

    // Log Test Finish Time for PDF report
    {
        time_t now_t = time(NULL);
        struct tm *tm_info = localtime(&now_t);
        char time_buf[64];
        strftime(time_buf, sizeof(time_buf), "%B %d, %Y %H:%M:%S", tm_info);
        printf("Test Finish Time : %s\n", time_buf);
    }

    printf("\n=== Shutting down ===\n");

#if PTP_ENABLED
    if (ptp_active) {
        // Stop PTP workers first
        printf("Stopping PTP workers...\n");
        ptp_print_stats();  // Final stats
        ptp_stop();
    }
#endif

#if HEALTH_MONITOR_ENABLED
    // Stop health monitor
    if (health_active && is_health_monitor_running()) {
        printf("Stopping health monitor...\n");
        print_health_monitor_stats();  // Final stats
        stop_health_monitor();
    }
#endif

    // Stop PSU telemetry listener (idempotent - safe if it never started).
    psu_telem_stop();

#if ENABLE_RAW_SOCKET_PORTS
    // Stop raw socket workers first (only if initialized)
    if (raw_ports_initialized)
    {
        // Already done before the totals were read; idempotent, so this is a
        // no-op unless that path was somehow skipped.
        stop_raw_socket_workers();
#if !STATS_MODE_DTN
        // In DTN mode raw socket table is not printed separately, DTN table is sufficient
        print_raw_socket_stats();  // Final stats
#endif
    }
#endif

    printf("Waiting 5 seconds for RX counters to flush...\n");
    sleep(15);

    // Wait for all DPDK workers to stop
    rte_eal_mp_wait_lcore();

    // Cleanup
#if PTP_ENABLED
    if (ptp_active)
        ptp_cleanup();
#endif

#if HEALTH_MONITOR_ENABLED
    if (health_active)
        cleanup_health_monitor();
#endif

#if ENABLE_RAW_SOCKET_PORTS
    if (raw_ports_initialized)
    {
        cleanup_raw_socket_ports();
    }
#endif
    cleanup_prbs_cache();
    cleanup_ports(&ports_config);
    cleanup_eal();
    shutdown_snapshot_cleanup();

    printf("Application exited cleanly\n");

    printf("\n Total test duration: %u seconds\n", test_time);

    return 0;
}
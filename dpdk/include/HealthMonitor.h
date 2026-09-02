#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>   // FILE, for health_monitor_render_port_tables()
#include <pthread.h>
#include "HealthTypes.h"

// ==========================================
// HEALTH MONITOR CONFIGURATION
// ==========================================

#define HEALTH_MONITOR_INTERFACE "eno12409"  // Port 13 interface
#define HEALTH_MONITOR_QUERY_INTERVAL_MS 1000  // Query interval (1 second)
#define HEALTH_MONITOR_RESPONSE_TIMEOUT_MS 500  // Response timeout (500ms)
#define HEALTH_MONITOR_EXPECTED_RESPONSES HEALTH_TOTAL_EXPECTED_PACKETS  // 6 (2 assistant + 3 manager + 1 MCU)
#define HEALTH_MONITOR_QUERY_SIZE 64  // Query packet size
#define HEALTH_MONITOR_SEQ_INIT 0x2F  // Initial sequence number (47)
#define HEALTH_MONITOR_RX_BUFFER_SIZE 2048  // RX buffer size

// VL_IDX for response filtering (DST MAC offset 4-5)
#define HEALTH_MONITOR_RESPONSE_VL_IDX 0x1188  // 4488 decimal
// VL_IDX carried by our own query packets: their DST MAC is the multicast
// 03:00:00:00:00:00, so bytes 4-5 read back as 0. The switch floods those
// queries to the other ports, where the raw socket RX rings see them, so the
// PRBS filters need to recognise them too. VL-ID 0 is never a valid PRBS
// VL-ID, so matching it costs nothing.
#define HEALTH_MONITOR_QUERY_VL_IDX 0x0000
#define HEALTH_MONITOR_RESPONSE_VL_IDX_HIGH 0x11
#define HEALTH_MONITOR_RESPONSE_VL_IDX_LOW 0x88

// ==========================================
// HEALTH MONITOR STATISTICS
// ==========================================

struct health_monitor_stats {
    uint64_t queries_sent;        // Total queries sent
    uint64_t responses_received;  // Total responses received
    uint64_t timeouts;            // Cycles with incomplete responses
    uint8_t  current_sequence;    // Current sequence number
    uint64_t last_cycle_time_ms;  // Last cycle duration in ms
    uint8_t  last_response_count; // Responses in last cycle
};

// ==========================================
// HEALTH MONITOR STATE
// ==========================================

struct health_monitor_state {
    // Thread
    pthread_t thread;
    volatile bool running;

    // Sockets
    int tx_socket;
    int rx_socket;
    int if_index;

    // Query packet (template)
    uint8_t query_packet[HEALTH_MONITOR_QUERY_SIZE];

    // Sequence tracking
    uint8_t sequence;

    // Statistics
    struct health_monitor_stats stats;
    pthread_spinlock_t stats_lock;

    // Warmup & FW version check
    volatile bool warmup_complete;
    uint32_t post_warmup_cycle_count;
    bool fw_check_done;

    // 28V power status check (from MainSoftware)
    uint8_t  expected_power_status;      // Expected input_power_status value
    bool     power_status_check_enabled; // Check enabled flag
    bool     power_status_check_done;    // Check completed flag
};

// ==========================================
// FUNCTION DECLARATIONS
// ==========================================

/**
 * @brief Initialize health monitor system
 * @return 0 on success, -1 on failure
 */
int init_health_monitor(void);

/**
 * @brief Start health monitor thread
 * @param stop_flag Pointer to global stop flag
 * @return 0 on success, -1 on failure
 */
// stop_flag: what the monitor watches to know when to stop. Pass the RX drain
//   flag rather than force_quit so the monitor keeps running through the
//   post-Ctrl+C drain window and the final health block is not stale.
// abort_flag: what the monitor sets when it finds a condition that must end the
//   test (28V power-status mismatch). Must be the app-wide stop flag.
int start_health_monitor(volatile bool *stop_flag, volatile bool *abort_flag);

/**
 * @brief Record the device's per-port frame counters as the test's zero point.
 *
 * The device never clears its own counters, so comparing them against ours as
 * absolutes only measures how long it had been running before we started.
 * Call this in the quiet window, when no traffic is moving, and the difference
 * from here is what the device carried during the test.
 */
void health_monitor_mark_port_baseline(void);

/**
 * @brief Device-side frames on one of its ports since the baseline.
 * @return false if that port was never seen, or its counter went backwards.
 */
bool health_monitor_get_port_delta(int port, uint64_t *tx, uint64_t *rx);

/**
 * @brief The two readings a port's delta was taken between.
 *
 * These are the device's counters as they stand, not differences: the same
 * numbers the ASSISTANT / MANAGER FPGA port tables print in their TxCnt and
 * RxCnt columns, so a reading here can be matched against a health monitor
 * table directly. `_base` is what was snapshotted in the quiet window, `_now`
 * the last cycle stored.
 *
 * @return false if that port was never seen by the monitor.
 */
bool health_monitor_get_port_readings(int port,
                                      uint64_t *tx_base, uint64_t *rx_base,
                                      uint64_t *tx_now, uint64_t *rx_now);

/**
 * @brief Print the monitor's own ASSISTANT and MANAGER port tables to `out`.
 *
 * Both readings, in the same layout the monitor prints every second: the
 * tables as they stood in the quiet window, then as they stood at the end. The
 * end-of-test reconciliation is one minus the other, so it can be checked here
 * without going back through the log for either.
 */
void health_monitor_render_port_tables(FILE *out);

/**
 * @brief Stop health monitor thread
 */
void stop_health_monitor(void);

/**
 * @brief Cleanup health monitor resources
 */
void cleanup_health_monitor(void);

/**
 * @brief Get health monitor statistics (thread-safe copy)
 * @param stats Output statistics structure
 */
void get_health_monitor_stats(struct health_monitor_stats *stats);

/**
 * @brief Print health monitor statistics
 */
void print_health_monitor_stats(void);

/**
 * @brief Check if health monitor is running
 * @return true if running
 */
bool is_health_monitor_running(void);

/**
 * @brief Notify health monitor that warmup is complete
 *
 * After this call, health monitor will start counting cycles
 * and check FW version match at the 10th cycle.
 */
void health_monitor_set_warmup_complete(void);

/**
 * @brief Set expected 28V power status for MCU health data verification
 *
 * After warmup, at the 10th cycle, the health monitor will compare
 * MCU's input_power_status against this expected value.
 * If mismatch: error printed and test stopped.
 *
 * @param expected_status Expected input_power_status value
 *        bit0: 28V Primary (0=SUCCESS, 1=FAIL)
 *        bit1: 28V Secondary (0=SUCCESS, 1=FAIL)
 */
void health_monitor_set_expected_power_status(uint8_t expected_status);

#endif // HEALTH_MONITOR_H
/**
 * vmc_test - server-side VMC test application (raw socket, no DPDK)
 *
 * Built and run on the ATE server (vmcStandalone step 2).
 * Runs every scenario in SCENARIOS[] in order; waits SCENARIO_GAP_S seconds
 * between two scenarios.
 *
 * Every scenario opens its own raw socket on the interface given in its
 * call below (different scenarios may use different interfaces).
 *
 * Usage: vmc_test [--hexdump]
 */

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "Packet.h"
#include "TestScenarios.h"
#include "Version.h"

/* Wait between two scenarios (seconds). */
#ifndef SCENARIO_GAP_S
#define SCENARIO_GAP_S 30
#endif

/* ====================================================================
 *  Scenarios - run in this order. Every scenario gets its own wrapper
 *  with its parameter values; add new ones to SCENARIOS[] below.
 * ==================================================================== */

/* tryPacketSend(ifname, network, vlanId, vlIdx, payloadLen, count, delayMs, resetSeq)
 *   10 x Net A, VLAN 100, VL-IDX 70, 100 B payload (+1 B seq), 10 ms
 *   10 x Net B, VLAN 100, VL-IDX 71, 100 B payload (+1 B seq), 10 ms
 *   sequence reset at the start of each call */
static bool runTryPacketSend(void)
{
    return tryPacketSend("ens1f0np0", NET_A, 100, 70, 100, 10, 10, true) == 10
        && tryPacketSend("ens1f0np0", NET_B, 100, 71, 100, 10, 10, true) == 10;
}

/* senaryoFlcsEndSystem(ifname, vlanIdA, vlIdxA, vlanIdB, vlIdxB, payloadLen,
 *                      countA, countB, pairCount, delayMs, abDelayUs, resetSeq)
 *   10 x Net A  VLAN 100  VL-IDX 70
 *   10 x Net B  VLAN  99  VL-IDX 71
 *   10 x (A, B) -> A B A B ...
 *   100 B payload (+1 B seq), 10 ms between packets, 10000 us A->B within a pair,
 *   sequence reset (A, B start at 0) */
static bool runSenaryoFlcsEndSystem(void)
{
    return senaryoFlcsEndSystem("ens1f0np0", 100, 70, 99, 71, 100, 10, 10, 10, 10, 10000, true) == 10 + 10 + 2 * 10;
}

/* senaryoVsEndSystem - same flow as FLCS:
 *   10 x Net A  VLAN  98  VL-IDX 60
 *   10 x Net B  VLAN  97  VL-IDX 61
 *   10 x (A, B) -> A B A B ...
 *   100 B payload (+1 B seq), 10 ms between packets, 10000 us A->B within a pair,
 *   sequence reset (A, B start at 0) */
static bool runSenaryoVsEndSystem(void)
{
    return senaryoVsEndSystem("ens1f0np0", 98, 60, 97, 61, 100, 10, 10, 10, 10, 10000, true) == 10 + 10 + 2 * 10;
}

/* senaryoFlcsSwitch(ifname, network, vlanId, vlIdxList, vlIdxCount, payloadLen,
 *                   countPerVl, delayMs, resetSeq)
 *   interface ens2f0np0, Net A, VLAN 125
 *   10 x VL-IDX 90, then 10 x 91, then 10 x 80, then 10 x 81
 *   100 B payload (+1 B seq), 10 ms between packets, sequence reset (A starts at 0) */
static bool runSenaryoFlcsSwitch(void)
{
    static const uint16_t vlIdx[] = { 90, 91, 80, 81 };
    const int n = (int)(sizeof(vlIdx) / sizeof(vlIdx[0]));
    return senaryoFlcsSwitch("ens2f0np0", NET_A, 125, vlIdx, n, 100, 10, 10, true) == n * 10;
}

typedef struct {
    const char *name;
    bool (*run)(void);
} Scenario;

static const Scenario SCENARIOS[] = {
    { "tryPacketSend",        runTryPacketSend        },
    { "senaryoFlcsEndSystem", runSenaryoFlcsEndSystem },
    { "senaryoVsEndSystem",   runSenaryoVsEndSystem   },
    { "senaryoFlcsSwitch",    runSenaryoFlcsSwitch    },
    /* { "senaryoXxx", runSenaryoXxx }, */
};

#define SCENARIO_COUNT ((int)(sizeof(SCENARIOS) / sizeof(SCENARIOS[0])))

/* ==================================================================== */

static void onSigint(int sig)
{
    (void)sig;
    g_stopRequested = 1;
}

/* Sleep `seconds`, waking early on Ctrl+C. */
static void waitSeconds(int seconds)
{
    for (int i = 0; i < seconds && !g_stopRequested; ++i) {
        struct timespec ts = { 1, 0 };
        nanosleep(&ts, NULL);
    }
}

static void usage(const char *prog)
{
    printf("Usage: %s [options]\n"
           "  -x, --hexdump          Print every sent frame as hex\n"
           "  -h, --help             Show this help\n"
           "\n"
           "Runs all scenarios in order, %d s apart:\n",
           prog, SCENARIO_GAP_S);
    for (int i = 0; i < SCENARIO_COUNT; ++i) printf("  %d. %s\n", i + 1, SCENARIOS[i].name);
}

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-x") || !strcmp(argv[i], "--hexdump")) {
            scenarioSetHexDump(true);
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage(argv[0]);
            return 2;
        }
    }

    /* Line-buffered even when stdout is a pipe/log, so lines stay in order. */
    setvbuf(stdout, NULL, _IOLBF, 0);

    printf("vmc_test v%s  (%d scenarios, %d s apart)\n", Version, SCENARIO_COUNT, SCENARIO_GAP_S);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = onSigint;
    sigaction(SIGINT, &sa, NULL);

    const char *result[SCENARIO_COUNT];
    for (int i = 0; i < SCENARIO_COUNT; ++i) result[i] = "NOT RUN";

    bool ok = true;
    for (int i = 0; i < SCENARIO_COUNT && !g_stopRequested; ++i) {
        if (i > 0) {
            printf("\nWaiting %d s before scenario %d/%d (%s)...\n",
                   SCENARIO_GAP_S, i + 1, SCENARIO_COUNT, SCENARIOS[i].name);
            waitSeconds(SCENARIO_GAP_S);
            if (g_stopRequested) break;
        }

        printf("\n==================================================\n");
        printf(" SCENARIO %d/%d: %s\n", i + 1, SCENARIO_COUNT, SCENARIOS[i].name);
        printf("==================================================\n");

        const bool scenario_ok = SCENARIOS[i].run() && !g_stopRequested;
        result[i] = scenario_ok ? "PASS" : "FAIL";
        printf("SCENARIO %d/%d %s: %s\n", i + 1, SCENARIO_COUNT, SCENARIOS[i].name, result[i]);

        if (!scenario_ok) {
            ok = false;
            break;          /* a send failure means the rest would fail too */
        }
    }
    if (g_stopRequested) ok = false;

    printf("\n==================== vmc_test summary ====================\n");
    for (int i = 0; i < SCENARIO_COUNT; ++i) {
        printf("  %d. %-24s %s\n", i + 1, SCENARIOS[i].name, result[i]);
    }
    printf("  Result: %s%s\n", ok ? "PASS" : "FAIL", g_stopRequested ? " (stopped by Ctrl+C)" : "");
    printf("===========================================================\n");
    return ok ? 0 : 1;
}

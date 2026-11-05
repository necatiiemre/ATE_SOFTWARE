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
#define SCENARIO_GAP_S 5
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

/* caseRedundancyFlcsEndSystem(ifname, vlanIdA, vlIdxA, vlanIdB, vlIdxB, payloadLen,
 *                      countA, countB, pairCount, delayMs, abDelayUs, resetSeq)
 *   10 x Net A  VLAN 100  VL-IDX 70
 *   10 x Net B  VLAN  99  VL-IDX 71
 *   10 x (A, B) -> A B A B ...
 *   100 B payload (+1 B seq), 10 ms between packets, 10000 us A->B within a pair,
 *   sequence reset (A, B start at 0) */
static bool runCaseRedundancyFlcsEndSystem(void)
{
    return caseRedundancyFlcsEndSystem("ens1f0np0", 100, 70, 99, 71, 100, 10, 10, 10, 80, 80, true) == 10 + 10 + 10 + 10 + 4 * 10;
}

/* caseRedundancyVsEndSystem - same flow as FLCS:
 *   10 x Net A  VLAN  98  VL-IDX 60
 *   10 x Net B  VLAN  97  VL-IDX 61
 *   10 x (A, B) -> A B A B ...
 *   100 B payload (+1 B seq), 10 ms between packets, 10000 us A->B within a pair,
 *   sequence reset (A, B start at 0) */
static bool runCaseRedundancyVsEndSystem(void)
{
    return caseRedundancyVsEndSystem("ens1f0np0", 98, 60, 97, 61, 100, 10, 10, 10, 80, 80, true) == 10 + 10 + 10 + 10 + 4 * 10;
}

/* caseRedundancyFlcsSwitch(ifname, network, vlanId, vlIdxList, vlIdxCount, payloadLen,
 *                   countPerVl, delayMs, resetSeq)
 *   interface ens2f0np0, Net A, VLAN 125
 *   10 x VL-IDX 90, then 10 x 91, then 10 x 80, then 10 x 81
 *   100 B payload (+1 B seq), 10 ms between packets, sequence reset (A starts at 0) */
static bool runCaseRedundancyFlcsSwitch(void)
{
    static const uint16_t vlIdx[] = { 90, 91, 80, 81 };
    const int n = (int)(sizeof(vlIdx) / sizeof(vlIdx[0]));
    return caseRedundancyFlcsSwitch("ens2f0np0", NET_A, 105, vlIdx, n, 100, 10, 80, true) == n * 10;
}

/* caseRedundancyVsSwitch - same flow as FLCS switch:
 *   interface ens2f0np0, Net B, VLAN 126
 *   10 x VL-IDX 90, then 10 x 91, then 10 x 80, then 10 x 81
 *   100 B payload (+1 B seq), 10 ms between packets, sequence reset (B starts at 0) */
static bool runCaseRedundancyVsSwitch(void)
{
    static const uint16_t vlIdx[] = { 90, 91, 80, 81 };
    const int n = (int)(sizeof(vlIdx) / sizeof(vlIdx[0]));
    return caseRedundancyVsSwitch("ens2f0np0", NET_B, 107, vlIdx, n, 100, 10, 80, true) == n * 10;
}

/* caseRedundancyBothSwitch(ifname, vlanIdA, vlanIdB, vlIdxList, vlIdxCount, payloadLen,
 *                   pairsPerVl, delayMs, abDelayUs, resetSeq)
 *   interface ens2f0np0, A = Net A VLAN 125, B = Net B VLAN 126
 *   10 x (A, B) on VL-IDX 80, then 81, then 90, then 91 (A and B same VL-IDX)
 *   100 B payload (+1 B seq), 10 ms between pairs, 20 us A->B within a pair,
 *   sequence reset (A, B start at 0; each pair carries the same sequence) */
static bool runCaseRedundancyBothSwitch(void)
{
    static const uint16_t vlIdx[] = { 80, 81, 90, 91 };
    const int n = (int)(sizeof(vlIdx) / sizeof(vlIdx[0]));
    return caseRedundancyBothSwitch("ens2f0np0", 105, 107, vlIdx, n, 100, 10, 80, 80, true) == n * 10 * 2;
}

/* caseIntegrityFlcsEndSystem(ifname, vlanId, vlIdx1, vlIdx2, payloadLen, count, delayMs)
 *   20 x Net A  VLAN 100  VL-IDX 72-73
 *   10 ms between packets,
 *   sequence reset (start at 0 then 1,2,3...9,11,12,14,15,17,18,20,21,23,24)*/
static bool runCaseIntegrityFlcsEndSystem(void)
{
    return caseIntegrityFlcsEndSystem("ens1f0np0", 100, 72, 73, 100, 20, 20) == 20 * 2;
}

/* caseIntegrityVsEndSystem - same flow as FLCS:
 *   20 x Net A  VLAN 98  VL-IDX 62-63
 *   10 ms between packets,
 *   sequence reset (start at 0 then 1,2,3...9,11,12,14,15,17,18,20,21,23,24)*/
static bool runCaseIntegrityVsEndSystem(void)
{
    return caseIntegrityVsEndSystem("ens1f0np0", 98, 62, 63, 100, 20, 20) == 20 * 2;
}

/* caseIntegrityFlcsSwitch(ifname, network, vlanId, vlIdx1, vlIdx2, ayloadLen,
 *                   countPerVl, delayMs)
 *   interface ens2f0np0, Net A, VLAN 125
 *   20 x Net A  VLAN 105  VL-IDX 62-63
 *   10 ms between packets,
 *   sequence reset (start at 0 then 1,2,3...9,11,12,14,15,17,18,20,21,23,24)*/
static bool runCaseIntegrityFlcsSwitch(void)
{
    return caseIntegrityFlcsSwitch("ens2f0np0", NET_A, 105, 92, 93, 100, 20, 20) == 20 * 2;
}

/* caseIntegrityVsSwitch - same flow as FLCS switch:
 *   interface ens2f0np0, Net B, VLAN 126
 *   20 x Net B  VLAN 107  VL-IDX 62-63
 *   10 ms between packets,
 *   sequence reset (start at 0 then 1,2,3...9,11,12,14,15,17,18,20,21,23,24)*/
static bool runCaseIntegrityVsSwitch(void)
{
    return caseIntegrityVsSwitch("ens2f0np0", NET_B, 107, 82, 83, 100, 20, 20) == 20 * 2;
}

typedef struct {
    const char *name;
    bool (*run)(void);
} Scenario;

static const Scenario SCENARIOS[] = {
    //{ "tryPacketSend",        runTryPacketSend        },
    { "caseRedundancyFlcsEndSystem", runCaseRedundancyFlcsEndSystem },
    { "caseRedundancyVsEndSystem",   runCaseRedundancyVsEndSystem   },
    { "caseRedundancyFlcsSwitch",    runCaseRedundancyFlcsSwitch    },
    { "caseRedundancyVsSwitch",      runCaseRedundancyVsSwitch      },
    { "caseRedundancyBothSwitch",    runCaseRedundancyBothSwitch    },
    { "caseIntegrityFlcsEndSystem", runCaseIntegrityFlcsEndSystem },
    { "caseIntegrityVsEndSystem",   runCaseIntegrityVsEndSystem   },
    { "caseIntegrityFlcsSwitch",    runCaseIntegrityFlcsSwitch    },
    { "caseIntegrityVsSwitch",      runCaseIntegrityVsSwitch      },
    /* { "caseXxx", runCaseXxx }, */
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

/*
 * Fibre emulator.
 *
 * The unit that will sit on the DTN's fibre ports is not built yet, so this
 * stands in for it long enough to answer one question: did the DTN take its
 * configuration? It injects a handful of packets on every VL a round defines
 * and reports which ones came back.
 *
 * It runs on the server alongside the acceleration test, which drives the DTN
 * from the workstation over copper. The two are independent programs: this one
 * owns the fibre side and the Mellanox switch, that one owns the copper side
 * and the DTN's configuration. A VL routed from fibre out to copper is sent
 * here and confirmed there.
 */

#include "CumulusSetup.h"
#include "FibreMap.h"
#include "PortRunner.h"
#include "Report.h"
#include "Scenario.h"
#include "VlFrame.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_PACKETS_PER_VL  5
#define DEFAULT_DRAIN_MS      500
#define INTERFACES_PATH  "cumulus/interfaces"

static scenario_flow_t g_flows[SCENARIO_MAX_FLOWS];
static report_t        g_report;
static uint8_t         g_frame[VL_FRAME_SIZE];
static uint8_t         g_rx[2048];
static volatile sig_atomic_t g_stop;

static void on_signal(int sig) { (void)sig; g_stop = 1; }

static uint64_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)(ts.tv_nsec / 1000000L);
}

static int read_choice(int min, int max)
{
    char line[64];

    for (;;) {
        printf("Choice [%d-%d]: ", min, max);
        fflush(stdout);
        if (!fgets(line, sizeof line, stdin))
            return min - 1;
        char *end;
        long value = strtol(line, &end, 10);
        if (end != line && value >= min && value <= max)
            return (int)value;
        puts("  Not a valid choice.");
    }
}

static const scenario_t *select_scenario(void)
{
    size_t count;
    const scenario_t *scenarios = scenario_all(&count);

    puts("\nSelect scenario");
    for (size_t i = 0; i < count; i++)
        printf("  %zu) %-8s %s\n", i + 1, scenarios[i].name, scenarios[i].description);
    puts("  0) Quit");

    int choice = read_choice(0, (int)count);
    return choice <= 0 ? NULL : &scenarios[choice - 1];
}

/* Drain both directions for a while, matching returning probes to their flow. */
static void collect(uint16_t rx_mask, unsigned duration_ms)
{
    uint64_t deadline = now_ms() + duration_ms;

    while (now_ms() < deadline && !g_stop) {
        bool idle = true;

        for (uint8_t port = 0; port < FIBRE_SERVER_PORT_COUNT; port++) {
            if (!(rx_mask >> port & 1))
                continue;
            int len = port_runner_receive(port, g_rx, sizeof g_rx);
            if (len <= 0)
                continue;
            idle = false;

            vl_probe_t probe;
            if (vl_frame_parse(g_rx, (size_t)len, &probe))
                report_received(&g_report, probe.vl_id, probe.vlan);
        }
        if (idle) {
            struct timespec ts = {.tv_sec = 0, .tv_nsec = 200000L};
            nanosleep(&ts, NULL);
        }
    }
}

static bool inject(size_t flow_count, unsigned packets_per_vl, uint16_t rx_mask)
{
    for (unsigned round = 0; round < packets_per_vl && !g_stop; round++) {
        for (size_t i = 0; i < flow_count && !g_stop; i++) {
            const scenario_flow_t *flow = &g_flows[i];
            int len = vl_frame_build(g_frame, sizeof g_frame, flow->vl_id,
                                     flow->tx_vlan, flow->src_dtn_port, round);
            if (len < 0)
                return false;
            if (!port_runner_send(flow->tx_server_port, g_frame, (size_t)len))
                return false;
            report_sent(&g_report, i);
        }
        /* Returns start arriving while we are still sending. */
        collect(rx_mask, 20);
        printf("\r  injected round %u/%u", round + 1, packets_per_vl);
        fflush(stdout);
    }
    putchar('\n');
    return true;
}

int main(int argc, char **argv)
{
    unsigned packets_per_vl = DEFAULT_PACKETS_PER_VL;
    bool skip_cumulus = false;
    int consumed = 0;

    puts("======================================");
    puts("  Fibre Emulator");
    puts("======================================");

    if (!port_runner_init(argc, argv, &consumed))
        return 1;
    argc -= consumed;
    argv += consumed;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--skip-cumulus") == 0)
            skip_cumulus = true;
        else if (strcmp(argv[i], "--packets") == 0 && i + 1 < argc)
            packets_per_vl = (unsigned)strtoul(argv[++i], NULL, 10);
        else
            printf("Ignoring unknown argument '%s'\n", argv[i]);
    }

    signal(SIGINT, on_signal);

    const scenario_t *scenario = select_scenario();
    if (!scenario)
        return 0;

    int flow_count = scenario_expand(scenario, g_flows, SCENARIO_MAX_FLOWS);
    if (flow_count < 0) {
        puts("Scenario does not expand - a port is out of range.");
        return 1;
    }

    uint16_t tx_mask, rx_mask;
    scenario_port_masks(g_flows, (size_t)flow_count, &tx_mask, &rx_mask);

    printf("\n  scenario     : %s - %s\n", scenario->name, scenario->description);
    printf("  VLs          : %d  (%u packets each)\n", flow_count, packets_per_vl);
    printf("  server ports : transmit");
    for (uint8_t p = 0; p < FIBRE_SERVER_PORT_COUNT; p++)
        if (tx_mask >> p & 1)
            printf(" %u (%s)", p, fibre_server_pci(p));
    printf("\n                 receive ");
    for (uint8_t p = 0; p < FIBRE_SERVER_PORT_COUNT; p++)
        if (rx_mask >> p & 1)
            printf(" %u (%s)", p, fibre_server_pci(p));
    putchar('\n');

    fflush(stdout);
    if (!skip_cumulus) {
        puts("\n  configuring the Mellanox switch");
        if (!cumulus_setup(INTERFACES_PATH)) {
            puts("  switch setup failed - nothing sent from here can reach the DTN");
            return 1;
        }
    } else {
        puts("\n  skipping switch setup at your request");
    }

    if (!port_runner_open((uint16_t)(tx_mask | rx_mask)))
        return 1;

    report_init(&g_report, scenario, g_flows, (size_t)flow_count);

    puts("");
    if (!inject((size_t)flow_count, packets_per_vl, rx_mask))
        puts("  injection stopped early");

    printf("  waiting %d ms for the last returns\n", DEFAULT_DRAIN_MS);
    collect(rx_mask, DEFAULT_DRAIN_MS);

    report_render(&g_report);
    bool all_up = report_all_links_up(&g_report);
    printf("\n  %s\n", all_up
           ? "every fibre link returned traffic - the DTN is forwarding as configured"
           : "some links returned nothing - see the rows marked NOTHING BACK");

    port_runner_close();
    return all_up ? 0 : 1;
}

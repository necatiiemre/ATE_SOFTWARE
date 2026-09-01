/*
 * Everything that does not need DPDK: the fibre map, the scenarios and the
 * probe frames. Building and running these on any machine keeps the parts that
 * can be got wrong quietly - a VLAN off by one, a link mapped to the wrong
 * server port - out of the lab.
 */

#include "FibreMap.h"
#include "Report.h"
#include "Scenario.h"
#include "VlFrame.h"

#include <stdio.h>
#include <string.h>

static int failures;

static void check(bool condition, const char *what)
{
    if (!condition) {
        printf("[FAIL] %s\n", what);
        failures++;
    }
}

static scenario_flow_t g_flows[SCENARIO_MAX_FLOWS];

static void test_map(void)
{
    check(fibre_tx_vlan(0) == 97 && fibre_rx_vlan(0) == 225, "DTN port 0 VLANs");
    check(fibre_tx_vlan(31) == 128 && fibre_rx_vlan(31) == 256, "DTN port 31 VLANs");
    check(fibre_server_port(0) == 2 && fibre_server_port(3) == 2, "DTN 0-3 on server port 2");
    check(fibre_server_port(8) == 0 && fibre_server_port(15) == 1, "DTN 8-15 on server ports 0,1");
    check(fibre_server_port(16) == 4 && fibre_server_port(31) == 7, "DTN 16-31 on server ports 4-7");
    check(fibre_server_port(32) < 0, "port 32 is not a fibre port");
    check(fibre_dtn_port_from_rx_vlan(241) == 16, "RX VLAN 241 is DTN port 16");
    check(fibre_dtn_port_from_rx_vlan(97) < 0, "a TX VLAN is not an RX VLAN");

    for (uint8_t p = 0; p < FIBRE_DTN_PORT_COUNT; p++)
        check(fibre_dtn_port_from_rx_vlan(fibre_rx_vlan(p)) == p, "RX VLAN round trip");
    printf("[ OK ] fibre map\n");
}

static void test_scenarios(void)
{
    size_t count;
    const scenario_t *scenarios = scenario_all(&count);

    check(count == 3, "three scenarios");
    for (size_t s = 0; s < count; s++) {
        int flows = scenario_expand(&scenarios[s], g_flows, SCENARIO_MAX_FLOWS);
        check(flows == 122, "12 links of 10 VLs plus 2 taps");
        if (flows < 0)
            continue;

        uint16_t tx_mask, rx_mask;
        scenario_port_masks(g_flows, (size_t)flows, &tx_mask, &rx_mask);

        int tx_ports = __builtin_popcount(tx_mask);
        int rx_ports = __builtin_popcount(rx_mask);
        check(tx_ports >= 4 && rx_ports >= 4, "at least four server ports each way");

        /* Every VL must be unique, and every link's VLANs must follow the map. */
        for (int i = 0; i < flows; i++) {
            const scenario_flow_t *f = &g_flows[i];
            check(f->tx_vlan == fibre_tx_vlan(f->src_dtn_port), "TX VLAN follows the map");
            check(f->tx_server_port == fibre_server_port(f->src_dtn_port),
                  "TX server port follows the map");
            if (f->expect_return) {
                check(f->rx_vlan == fibre_rx_vlan(f->dst_dtn_port), "RX VLAN follows the map");
                check(f->rx_server_port == fibre_server_port(f->dst_dtn_port),
                      "RX server port follows the map");
            }
            for (int j = i + 1; j < flows; j++)
                if (g_flows[i].vl_id == g_flows[j].vl_id) {
                    printf("[FAIL] %s: VL %u appears twice\n", scenarios[s].name, g_flows[i].vl_id);
                    failures++;
                    break;
                }
        }
        printf("[ OK ] %-8s %d flows, TX on %d server ports, RX on %d\n",
               scenarios[s].name, flows, tx_ports, rx_ports);
    }
}

static void test_frames(void)
{
    uint8_t frame[VL_FRAME_SIZE];
    vl_probe_t probe;

    int len = vl_frame_build(frame, sizeof frame, 1024, 97, 0, 7);
    check(len == VL_FRAME_SIZE, "frame is built");
    check(frame[0] == 0x03 && frame[4] == 0x04 && frame[5] == 0x00,
          "VL id sits in the destination MAC");
    check(frame[16] == 0x08 && frame[17] == 0x00, "VLAN tag precedes IPv4");
    check(frame[14] == 0x00 && frame[15] == 97, "VLAN 97 in the tag");

    check(vl_frame_parse(frame, (size_t)len, &probe), "frame is recognised");
    check(probe.vl_id == 1024 && probe.vlan == 97 && probe.sequence == 7 &&
          probe.src_dtn_port == 0, "probe fields survive the round trip");

    /* The switch retags on the way back; the payload must still identify it. */
    frame[14] = 0x00; frame[15] = (uint8_t)225;
    check(vl_frame_parse(frame, (size_t)len, &probe) && probe.vlan == 225,
          "a retagged frame is still ours");

    uint8_t alien[VL_FRAME_SIZE];
    memset(alien, 0xAA, sizeof alien);
    check(!vl_frame_parse(alien, sizeof alien, &probe), "a foreign frame is rejected");
    printf("[ OK ] probe frames\n");
}

static void test_report(void)
{
    size_t count;
    const scenario_t *scenarios = scenario_all(&count);
    int flows = scenario_expand(&scenarios[0], g_flows, SCENARIO_MAX_FLOWS);
    report_t report;

    report_init(&report, &scenarios[0], g_flows, (size_t)flows);
    for (int i = 0; i < flows; i++)
        report_sent(&report, (size_t)i);

    check(!report_all_links_up(&report), "nothing returned yet");
    for (int i = 0; i < flows; i++)
        if (g_flows[i].expect_return)
            check(report_received(&report, g_flows[i].vl_id, g_flows[i].rx_vlan),
                  "a returning VL is matched");
    check(report_all_links_up(&report), "every link reported up");

    check(!report_received(&report, 60000, 999), "an unknown VL is not matched");
    printf("[ OK ] report\n");
}

int main(void)
{
    test_map();
    test_scenarios();
    test_frames();
    test_report();

    if (failures) {
        printf("FAILED: %d check(s)\n", failures);
        return 1;
    }
    puts("PASS: fibre map, scenarios, frames and report are consistent");
    return 0;
}

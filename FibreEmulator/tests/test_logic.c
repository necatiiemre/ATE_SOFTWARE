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
#include <stdlib.h>
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

/* ------------------------------------------------------------------ */
/* The switch configuration is the authority for both port maps, and it is in
 * the repository, so the maps can be re-derived from it rather than trusted.
 * Assuming transmit and receive shared one map cost a whole round of testing. */

#define INTERFACES "cumulus/interfaces"

typedef struct {
    char     name[64];
    uint16_t vids[64];
    size_t   vid_count;
    int      pvid;
} iface_t;

static iface_t g_ifaces[128];
static size_t  g_iface_count;

static iface_t *iface_named(const char *name)
{
    for (size_t i = 0; i < g_iface_count; i++)
        if (strcmp(g_ifaces[i].name, name) == 0)
            return &g_ifaces[i];
    return NULL;
}

/* A server-facing trunk is swpN; a DTN-facing breakout is swpNsM. */
static bool is_trunk(const char *name)
{
    if (strncmp(name, "swp", 3) != 0 || name[3] == '\0')
        return false;
    for (const char *p = name + 3; *p; p++)
        if (*p < '0' || *p > '9')
            return false;
    return true;
}

/* The trunk a VLAN leaves the switch through. */
static const iface_t *trunk_with_vid(uint16_t vid)
{
    for (size_t i = 0; i < g_iface_count; i++) {
        const iface_t *f = &g_ifaces[i];
        if (!is_trunk(f->name))
            continue;
        for (size_t v = 0; v < f->vid_count; v++)
            if (f->vids[v] == vid)
                return f;
    }
    return NULL;
}

static int read_interfaces(void)
{
    FILE *f = fopen(INTERFACES, "r");
    char line[512];
    iface_t *cur = NULL;

    if (!f) {
        printf("[FAIL] cannot open %s - run from the FibreEmulator directory\n",
               INTERFACES);
        return -1;
    }
    while (fgets(line, sizeof line, f)) {
        char name[64];

        if (sscanf(line, " iface %63s", name) == 1) {
            if (g_iface_count == sizeof g_ifaces / sizeof g_ifaces[0])
                break;
            cur = &g_ifaces[g_iface_count++];
            memset(cur, 0, sizeof *cur);
            snprintf(cur->name, sizeof cur->name, "%s", name);
            cur->pvid = -1;
            continue;
        }
        if (!cur)
            continue;

        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (strncmp(p, "bridge-vids", 11) == 0) {
            for (p += 11; *p; ) {
                char *end;
                long v = strtol(p, &end, 10);
                if (end == p)
                    break;
                if (cur->vid_count < sizeof cur->vids / sizeof cur->vids[0])
                    cur->vids[cur->vid_count++] = (uint16_t)v;
                p = end;
            }
        } else if (strncmp(p, "bridge-pvid", 11) == 0) {
            cur->pvid = (int)strtol(p + 11, NULL, 10);
        }
    }
    fclose(f);
    return g_iface_count ? 0 : -1;
}

static void test_against_switch_config(void)
{
    /* swpN <-> server port, taken from the transmit map: the trunk carrying
     * DTN port p's transmit VLAN belongs to the server port that sends to p. */
    const iface_t *trunk_of_server[FIBRE_SERVER_PORT_COUNT] = {NULL};

    if (read_interfaces() != 0) {
        failures++;
        return;
    }

    for (uint8_t p = 0; p < FIBRE_DTN_PORT_COUNT; p++) {
        char breakout[16];
        snprintf(breakout, sizeof breakout, "swp%us%u", 25u + p / 4u, p % 4u);

        const iface_t *f = iface_named(breakout);
        if (!f) {
            printf("[FAIL] %s is not in %s (DTN port %u)\n", breakout, INTERFACES, p);
            failures++;
            continue;
        }
        check(f->vid_count == 1 && f->vids[0] == fibre_tx_vlan(p),
              "the breakout carries only its transmit VLAN");
        check(f->pvid == fibre_rx_vlan(p),
              "the breakout tags what the DTN sends with its receive VLAN");

        const iface_t *tx_trunk = trunk_with_vid(fibre_tx_vlan(p));
        if (!tx_trunk) {
            printf("[FAIL] no trunk carries VLAN %u (DTN port %u transmit)\n",
                   fibre_tx_vlan(p), p);
            failures++;
            continue;
        }
        int server = fibre_server_port(p);
        if (trunk_of_server[server] && trunk_of_server[server] != tx_trunk) {
            printf("[FAIL] server port %d maps to both %s and %s\n",
                   server, trunk_of_server[server]->name, tx_trunk->name);
            failures++;
        }
        trunk_of_server[server] = tx_trunk;
    }

    /* Now the claim worth testing: where each DTN port's traffic comes back. */
    for (uint8_t p = 0; p < FIBRE_DTN_PORT_COUNT; p++) {
        const iface_t *rx_trunk = trunk_with_vid(fibre_rx_vlan(p));
        int server = fibre_rx_server_port(p);

        if (!rx_trunk) {
            printf("[FAIL] no trunk carries VLAN %u (DTN port %u receive)\n",
                   fibre_rx_vlan(p), p);
            failures++;
            continue;
        }
        if (server < 0 || trunk_of_server[server] != rx_trunk) {
            printf("[FAIL] DTN port %u comes back on %s, but the map says server "
                   "port %d (%s)\n", p, rx_trunk->name, server,
                   server >= 0 && trunk_of_server[server]
                       ? trunk_of_server[server]->name : "?");
            failures++;
        }
    }

    /* And that the two maps really are different, so a future simplification
     * that merges them fails here rather than in the lab. */
    bool differs = false;
    for (uint8_t p = 0; p < FIBRE_DTN_PORT_COUNT; p++)
        if (fibre_server_port(p) != fibre_rx_server_port(p))
            differs = true;
    check(differs, "transmit and receive server ports are not the same map");

    if (!failures)
        printf("[ OK ] both port maps re-derived from %s\n", INTERFACES);
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
                check(f->rx_server_port == fibre_rx_server_port(f->dst_dtn_port),
                      "RX server port follows the receive map, not the transmit one");
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

    /* The AFDX sequence byte sits outside the IP total_length, so the frame is
     * one byte longer than IP declares - exactly as the reference frames are. */
    uint16_t ip_total = (uint16_t)((frame[18 + 2] << 8) | frame[18 + 3]);
    check(ip_total == VL_FRAME_SIZE - 14 - 4 - 1, "IP total_length excludes the AFDX byte");
    uint16_t udp_len = (uint16_t)((frame[18 + 20 + 4] << 8) | frame[18 + 20 + 5]);
    check(udp_len == ip_total - 20, "UDP length agrees with IP");
    check(frame[VL_FRAME_SIZE - 1] == vl_frame_afdx_seq(7), "AFDX sequence byte is last");
    check(vl_frame_afdx_seq(0) == 0 && vl_frame_afdx_seq(1) == 1 &&
          vl_frame_afdx_seq(255) == 255 && vl_frame_afdx_seq(256) == 1,
          "AFDX sequence wraps 1..255 after a single 0");
    check(frame[18 + 12] == 10 && frame[18 + 13] == 1 && frame[18 + 14] == 0 &&
          frame[18 + 15] == 1, "source IP follows 10.1.<port>.1");

    check(vl_frame_parse(frame, (size_t)len, &probe), "frame is recognised");
    check(probe.vl_id == 1024 && probe.vlan == 97 && probe.sequence == 7 &&
          probe.src_dtn_port == 0, "probe fields survive the round trip");
    check(probe.afdx_seq == vl_frame_afdx_seq(7), "AFDX sequence survives the round trip");

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
    test_against_switch_config();
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

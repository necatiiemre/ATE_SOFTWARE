/**
 * vmc_test - server-side VMC test application (raw socket, no DPDK)
 *
 * Built and run on the ATE server (vmcStandalone step 2).
 *
 * Usage: vmc_test [--interface <dev>] [--hexdump]
 */

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "Packet.h"
#include "RawSocket.h"
#include "TestScenarios.h"
#include "Version.h"

#define DEFAULT_INTERFACE "ens1f0np0"

static void onSigint(int sig)
{
    (void)sig;
    g_stopRequested = 1;
}

static void usage(const char *prog)
{
    printf("Usage: %s [options]\n"
           "  -i, --interface <dev>  Server interface to send on (default: %s)\n"
           "  -x, --hexdump          Print every sent frame as hex\n"
           "  -h, --help             Show this help\n",
           prog, DEFAULT_INTERFACE);
}

int main(int argc, char *argv[])
{
    const char *ifname = DEFAULT_INTERFACE;

    for (int i = 1; i < argc; ++i) {
        if ((!strcmp(argv[i], "-i") || !strcmp(argv[i], "--interface")) && i + 1 < argc) {
            ifname = argv[++i];
        } else if (!strncmp(argv[i], "--interface=", 12)) {
            ifname = argv[i] + 12;
        } else if (!strcmp(argv[i], "-x") || !strcmp(argv[i], "--hexdump")) {
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

    printf("vmc_test v%s\n", Version);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = onSigint;
    sigaction(SIGINT, &sa, NULL);

    RawSocket sock;
    if (rawSocketOpen(&sock, ifname) != 0) {
        return 1;
    }

    bool ok = true;

    /* ================= VMC trial packets =================
     * tryPacketSend(sock, network, vlanId, vlIdx, payloadLen, count, delayMs)
     *   10 x Net A, VLAN 100, VL-IDX 70, 100 B payload (+1 B seq)
     *   10 x Net B, VLAN 100, VL-IDX 71, 100 B payload (+1 B seq)
     */
    ok &= tryPacketSend(&sock, NET_A, 100, 70, 100, 10, 10) == 10;
    ok &= tryPacketSend(&sock, NET_B, 100, 71, 100, 10, 10) == 10;

    rawSocketClose(&sock);

    printf("vmc_test: %s\n", ok ? "all packets sent" : "FAILED");
    return ok ? 0 : 1;
}

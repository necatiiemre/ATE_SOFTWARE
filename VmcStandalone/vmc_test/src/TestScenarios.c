#include "TestScenarios.h"
#include "PacketDefs.h"

#include <stdio.h>
#include <time.h>

volatile int g_stopRequested = 0;
static bool  g_hexDump = false;

void scenarioSetHexDump(bool enabled) { g_hexDump = enabled; }

static void sleepMs(unsigned ms)
{
    struct timespec ts = { (time_t)(ms / 1000), (long)(ms % 1000) * 1000000L };
    while (nanosleep(&ts, &ts) != 0 && !g_stopRequested) { /* EINTR: continue */ }
}

int tryPacketSend(RawSocket *sock, Network net, uint16_t vlanId, uint16_t vlIdx,
                  uint16_t payloadLen, int count, unsigned delayMs)
{
    if (!sock || net < 0 || net >= NET_COUNT || count < 0) {
        fprintf(stderr, "[tryPacketSend] invalid arguments\n");
        return -1;
    }
    if (vlanId > MAX_VLAN_ID) {
        fprintf(stderr, "[tryPacketSend] VLAN ID %u out of range (0..%d)\n", vlanId, MAX_VLAN_ID);
        return -1;
    }
    if (payloadLen < MIN_DATA_LEN || payloadLen > MAX_DATA_LEN) {
        fprintf(stderr, "[tryPacketSend] payload length %u out of range (%d..%d)\n",
                payloadLen, MIN_DATA_LEN, MAX_DATA_LEN);
        return -1;
    }

    printf("[tryPacketSend] %s VLAN %u VL-IDX %u payload %u+1 B, %d packets, %u ms interval\n",
           networkName(net), vlanId, vlIdx, payloadLen, count, delayMs);

    uint8_t frame[MAX_FRAME_LEN];
    int sent = 0;

    for (int i = 0; i < count && !g_stopRequested; ++i) {
        const uint8_t seq = nextSequence(net);
        const size_t len = buildPacket(frame, sizeof(frame), net, vlanId, vlIdx, payloadLen, seq);
        if (len == 0) {
            fprintf(stderr, "[tryPacketSend] buildPacket failed\n");
            break;
        }

        if (rawSocketSend(sock, frame, len) != 0) {
            fprintf(stderr, "[tryPacketSend] %s packet %d/%d (seq %u) NOT sent\n",
                    networkName(net), i + 1, count, seq);
            break;
        }
        ++sent;

        printf("[TX] %s VLAN %u VL-IDX %u seq %3u  frame %zu B  (%d/%d)\n",
               networkName(net), vlanId, vlIdx, seq, len, i + 1, count);
        if (g_hexDump) hexDump(frame, len);

        if (delayMs > 0 && i + 1 < count) sleepMs(delayMs);
    }

    printf("[tryPacketSend] %s: %d/%d packets sent%s\n", networkName(net), sent, count,
           g_stopRequested ? " (stopped by Ctrl+C)" : "");
    return sent;
}

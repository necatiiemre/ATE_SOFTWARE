#include "TestScenarios.h"
#include "PacketDefs.h"

#include <stdio.h>
#include <time.h>

volatile int g_stopRequested = 0;
static bool  g_hexDump = false;

void scenarioSetHexDump(bool enabled) { g_hexDump = enabled; }

/* ====================================================================
 *  Shared helpers - every scenario sends through sendOne()
 * ==================================================================== */

static void sleepMs(unsigned ms)
{
    struct timespec ts = { (time_t)(ms / 1000), (long)(ms % 1000) * 1000000L };
    while (nanosleep(&ts, &ts) != 0 && !g_stopRequested) { /* EINTR: continue */ }
}

static bool validFlow(const char *who, Network net, uint16_t vlanId, uint16_t payloadLen)
{
    if (net < 0 || net >= NET_COUNT) {
        fprintf(stderr, "[%s] invalid network\n", who);
        return false;
    }
    if (vlanId > MAX_VLAN_ID) {
        fprintf(stderr, "[%s] VLAN ID %u out of range (0..%d)\n", who, vlanId, MAX_VLAN_ID);
        return false;
    }
    if (payloadLen < MIN_DATA_LEN || payloadLen > MAX_DATA_LEN) {
        fprintf(stderr, "[%s] payload length %u out of range (%d..%d)\n",
                who, payloadLen, MIN_DATA_LEN, MAX_DATA_LEN);
        return false;
    }
    return true;
}

/* Build + send one packet with the network's next sequence number.
 * `progress` is printed at the end of the [TX] line, e.g. "(3/10)". */
static bool sendOne(RawSocket *sock, Network net, uint16_t vlanId, uint16_t vlIdx,
                    uint16_t payloadLen, const char *progress)
{
    uint8_t frame[MAX_FRAME_LEN];
    const uint8_t seq = nextSequence(net);
    const size_t len = buildPacket(frame, sizeof(frame), net, vlanId, vlIdx, payloadLen, seq);
    if (len == 0) {
        fprintf(stderr, "[TX] buildPacket failed\n");
        return false;
    }
    if (rawSocketSend(sock, frame, len) != 0) {
        fprintf(stderr, "[TX] %s VLAN %u VL-IDX %u seq %u NOT sent %s\n",
                networkName(net), vlanId, vlIdx, seq, progress);
        return false;
    }
    printf("[TX] %s VLAN %3u VL-IDX %u seq %3u  frame %zu B  %s\n",
           networkName(net), vlanId, vlIdx, seq, len, progress);
    if (g_hexDump) hexDump(frame, len);
    return true;
}

/* Send `count` packets of one flow. Delay goes BETWEEN packets; pass
 * delayBefore=true when this burst follows other packets (keeps the same
 * spacing across phases). Returns packets sent. */
static int sendBurst(RawSocket *sock, Network net, uint16_t vlanId, uint16_t vlIdx,
                     uint16_t payloadLen, int count, unsigned delayMs, bool delayBefore)
{
    int sent = 0;
    char progress[32];
    for (int i = 0; i < count && !g_stopRequested; ++i) {
        if (delayMs > 0 && (i > 0 || delayBefore)) sleepMs(delayMs);
        if (g_stopRequested) break;
        snprintf(progress, sizeof(progress), "(%d/%d)", i + 1, count);
        if (!sendOne(sock, net, vlanId, vlIdx, payloadLen, progress)) break;
        ++sent;
    }
    return sent;
}

/* resetSeq=true -> counters back to 0; false -> continue. Logs what happens. */
static void applySeqReset(const char *who, const Network *nets, int n, bool resetSeq)
{
    printf("[%s] sequence:", who);
    for (int i = 0; i < n; ++i) {
        if (resetSeq) resetSequence(nets[i]);
        printf(" %s starts at %u", networkName(nets[i]), peekSequence(nets[i]));
        if (i + 1 < n) printf(",");
    }
    printf(" (%s)\n", resetSeq ? "reset" : "continuing");
}

/* ====================================================================
 *  Scenarios
 * ==================================================================== */

int tryPacketSend(const char *ifname, Network net, uint16_t vlanId, uint16_t vlIdx,
                  uint16_t payloadLen, int count, unsigned delayMs, bool resetSeq)
{
    if (count < 0 || !validFlow("tryPacketSend", net, vlanId, payloadLen)) return -1;

    printf("[tryPacketSend] %s: %s VLAN %u VL-IDX %u payload %u+1 B, %d packets, %u ms interval\n",
           ifname ? ifname : "(null)", networkName(net), vlanId, vlIdx, payloadLen, count, delayMs);

    RawSocket sock;
    if (rawSocketOpen(&sock, ifname) != 0) return -1;

    applySeqReset("tryPacketSend", &net, 1, resetSeq);

    const int sent = sendBurst(&sock, net, vlanId, vlIdx, payloadLen, count, delayMs, false);
    rawSocketClose(&sock);

    printf("[tryPacketSend] %s: %d/%d packets sent%s\n", networkName(net), sent, count,
           g_stopRequested ? " (stopped by Ctrl+C)" : "");
    return sent;
}

/* Shared end-system flow: countA x A, countB x B, pairCount x (A, B).
 * `who` is the scenario name used in every log line. */
static int endSystemFlow(const char *who, const char *ifname,
                         uint16_t vlanIdA, uint16_t vlIdxA,
                         uint16_t vlanIdB, uint16_t vlIdxB,
                         uint16_t payloadLen,
                         int countA, int countB, int pairCount,
                         unsigned delayMs, bool resetSeq)
{
    if (countA < 0 || countB < 0 || pairCount < 0 ||
        !validFlow(who, NET_A, vlanIdA, payloadLen) ||
        !validFlow(who, NET_B, vlanIdB, payloadLen)) {
        return -1;
    }

    const int expected = countA + countB + 2 * pairCount;
    printf("[%s] interface %s\n", who, ifname ? ifname : "(null)");
    printf("[%s] A: VLAN %u VL-IDX %u | B: VLAN %u VL-IDX %u | payload %u+1 B | %u ms interval\n",
           who, vlanIdA, vlIdxA, vlanIdB, vlIdxB, payloadLen, delayMs);
    printf("[%s] plan: %d x A, then %d x B, then %d x (A,B)  = %d packets\n",
           who, countA, countB, pairCount, expected);

    RawSocket sockStorage;
    if (rawSocketOpen(&sockStorage, ifname) != 0) return -1;
    RawSocket *sock = &sockStorage;

    static const Network bothNets[2] = { NET_A, NET_B };
    applySeqReset(who, bothNets, 2, resetSeq);

    int sent = 0;
    int ok;

    /* Phase 1: countA x Net A */
    printf("[%s] --- Phase 1/3: %d x NetA ---\n", who, countA);
    ok = sendBurst(sock, NET_A, vlanIdA, vlIdxA, payloadLen, countA, delayMs, false);
    sent += ok;
    if (ok != countA) goto done;

    /* Phase 2: countB x Net B */
    printf("[%s] --- Phase 2/3: %d x NetB ---\n", who, countB);
    ok = sendBurst(sock, NET_B, vlanIdB, vlIdxB, payloadLen, countB, delayMs, sent > 0);
    sent += ok;
    if (ok != countB) goto done;

    /* Phase 3: pairCount x (A, B) -> A B A B ...
     * A and B of a pair carry the SAME sequence number. */
    printf("[%s] --- Phase 3/3: %d x (NetA, NetB) ---\n", who, pairCount);
    {
        const uint8_t seqA = peekSequence(NET_A), seqB = peekSequence(NET_B);
        alignSequences(NET_A, NET_B);
        const uint8_t start = peekSequence(NET_A);
        if (seqA != seqB) {
            printf("[%s] WARNING: A/B sequences differ (A %u, B %u) - %s jumps to %u so "
                   "each A/B pair carries the same sequence\n",
                   who, seqA, seqB, (start == seqA) ? "NetB" : "NetA", start);
        }
        printf("[%s] A/B pairs share the sequence, starting at %u\n", who, start);
    }
    for (int i = 0; i < pairCount && !g_stopRequested; ++i) {
        char progress[32];
        snprintf(progress, sizeof(progress), "(pair %d/%d)", i + 1, pairCount);

        if (delayMs > 0 && sent > 0) sleepMs(delayMs);
        if (g_stopRequested || !sendOne(sock, NET_A, vlanIdA, vlIdxA, payloadLen, progress)) break;
        ++sent;

        if (delayMs > 0) sleepMs(delayMs);
        if (g_stopRequested || !sendOne(sock, NET_B, vlanIdB, vlIdxB, payloadLen, progress)) break;
        ++sent;
    }

done:
    rawSocketClose(sock);
    printf("[%s] %d/%d packets sent%s\n", who, sent, expected,
           g_stopRequested ? " (stopped by Ctrl+C)" : "");
    return sent;
}

int senaryoFlcsEndSystem(const char *ifname,
                         uint16_t vlanIdA, uint16_t vlIdxA,
                         uint16_t vlanIdB, uint16_t vlIdxB,
                         uint16_t payloadLen,
                         int countA, int countB, int pairCount,
                         unsigned delayMs, bool resetSeq)
{
    return endSystemFlow("senaryoFlcsEndSystem", ifname, vlanIdA, vlIdxA, vlanIdB, vlIdxB,
                         payloadLen, countA, countB, pairCount, delayMs, resetSeq);
}

int senaryoVsEndSystem(const char *ifname,
                       uint16_t vlanIdA, uint16_t vlIdxA,
                       uint16_t vlanIdB, uint16_t vlIdxB,
                       uint16_t payloadLen,
                       int countA, int countB, int pairCount,
                       unsigned delayMs, bool resetSeq)
{
    return endSystemFlow("senaryoVsEndSystem", ifname, vlanIdA, vlIdxA, vlanIdB, vlIdxB,
                         payloadLen, countA, countB, pairCount, delayMs, resetSeq);
}

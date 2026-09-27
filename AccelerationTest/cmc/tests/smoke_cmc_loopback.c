/**
 * @file smoke_cmc_loopback.c
 * @brief The whole CMC loop on a real socket, with a stand-in for the unit.
 *
 * Every other CMC test hands frames to a function. This one puts them on a
 * wire: it runs the data plane against the loopback interface, with a thread
 * standing in for the CMC - reading what the sender emits, applying the
 * transform the unit applies, moving the VL id into the return range and sending
 * it back - and then checks the sender's own receiver verified it.
 *
 * What that proves is the plumbing, which is the part no amount of unit testing
 * reaches: the sockets open, the sender paces and actually transmits, the
 * receivers actually receive, the classification runs on frames that came off a
 * socket rather than out of a buffer, and the threads start and join without
 * leaving anything behind. What it cannot prove is anything about the CMC: the
 * stand-in does what we believe the unit does, so agreeing with it says the two
 * ends of *this* program agree.
 *
 * Not part of `make test`: it needs a raw socket, so it needs root. `make smoke`.
 *
 * One network, not two. Both would have to share the loopback interface, and
 * then each receiver would see the other's frames - the interface is what tells
 * the networks apart, so aliasing them defeats the thing being tested. The twin
 * arrangement is covered in test_cmc_dataplane.c, which can hand a frame to one
 * network without the other seeing it.
 */

#include "AppConfig.h"
#include "CmcDataPlane.h"
#include "CmcPacket.h"
#include "CmcPayloadVerify.h"
#include "CmcStats.h"
#include "CmcVerify.h"
#include "Log.h"
#include "RawSocket.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int failures;

static void check(bool ok, const char *what)
{
    printf("%s %s\n", ok ? "[ OK ]" : "[FAIL]", what);
    if (!ok)
        failures++;
}

/* Small and quick: four VLs so each one's sequence advances fast enough to see,
 * and a rate that fills a couple of seconds with a few thousand frames rather
 * than a few hundred thousand. */
#define SMOKE_IFACE    "lo"
#define SMOKE_VL_COUNT 4
#define SMOKE_GBPS     0.050
#define SMOKE_SECONDS  2

static cmc_config_t smoke_config(void)
{
    cmc_config_t c = {0};

    c.nets[0].iface        = SMOKE_IFACE;
    c.nets[0].label        = "NET-A";
    c.nets[0].unit_label   = "DSMA";
    c.nets[0].src_mac_tail = 0x20;
    c.net_count            = 1;
    c.pmm_count            = 0;

    c.vlan_tagged  = false;
    c.tx_vl_start  = 10001;
    c.rx_vl_start  = 10521;
    c.vl_count     = SMOKE_VL_COUNT;
    c.target_gbps  = SMOKE_GBPS;
    c.warmup_s     = 0;
    c.stats_interval_s = 1;
    return c;
}

/* ------------------------------------------------------------------ */
/* The stand-in for the unit                                          */
/* ------------------------------------------------------------------ */

struct unit_ctx {
    const cmc_config_t     *config;
    raw_socket_t            sock;
    volatile bool          *stop;
    /* Its own copy of the stream: the unit regenerates it too, and sharing the
     * caller's would hide a disagreement about the offset. Generated before the
     * thread starts, because generating it inside the thread would have it still
     * counting bits while the sender was already on its last frame - which is
     * exactly what happened the first time this test ran. */
    const prbs31_cache_t *prbs;
    uint64_t                echoed;
    uint64_t                seen_own_echo;
};

/* What the CMC does, written out from the spec in CmcVerify.h rather than
 * called from it - the XOR chain is applied one constant at a time, so the
 * folded mask the verifier uses is exercised rather than assumed. */
static void apply_transform(uint8_t *payload, const uint8_t *prbs_exp, uint64_t seq)
{
    uint8_t sm[SPLITMIX_XOR_BYTES];

    build_expected_splitmix(sm, seq, prbs_exp);
    memcpy(payload + CMC_SEQ_BYTES, sm, SPLITMIX_XOR_BYTES);

    const uint32_t crc = sw_crc32c(payload, CMC_SEQ_BYTES + SPLITMIX_XOR_BYTES);
    const uint32_t be  = __builtin_bswap32(crc);

    memcpy(payload + CMC_SEQ_BYTES + SPLITMIX_XOR_BYTES, &be, sizeof be);

    static const uint8_t chain[] = {6, 7, 8, 13, 15};
    uint8_t *xor_byte = payload + CMC_SEQ_BYTES + SPLITMIX_XOR_BYTES + SPLITMIX_CRC_BYTES;

    for (size_t i = 0; i < sizeof chain; i++)
        *xor_byte ^= chain[i];
}

static void *unit_thread_fn(void *arg)
{
    struct unit_ctx *u = arg;
    const cmc_config_t *c = u->config;
    const size_t frame_len = CMC_FRAME_LEN(c->vlan_tagged);
    const size_t payload_off = CMC_PAYLOAD_OFF(c->vlan_tagged);
    uint8_t *buf = malloc(CMC_FRAME_LEN_MAX + 64);

    if (!buf)
        return NULL;

    while (!*u->stop) {
        int n = raw_socket_recv(&u->sock, buf, CMC_FRAME_LEN_MAX + 64, 200);

        while (n > 0) {
            const uint16_t vl = cmc_vl_id_of(buf);

            if ((size_t)n == frame_len &&
                vl >= c->tx_vl_start && vl < c->tx_vl_start + c->vl_count) {
                const uint16_t offset = (uint16_t)(vl - c->tx_vl_start);
                const uint16_t back   = (uint16_t)(c->rx_vl_start + offset);
                uint8_t *payload = buf + payload_off;
                const uint64_t seq = cmc_payload_seq(payload);

                /* The VL id, in both the places it is carried. */
                buf[4] = (uint8_t)(back >> 8);
                buf[5] = (uint8_t)back;

                uint8_t *ip = buf + CMC_L2_LEN(c->vlan_tagged);
                ip[18] = (uint8_t)(back >> 8);
                ip[19] = (uint8_t)back;
                ip[10] = ip[11] = 0;
                const uint16_t csum = cmc_ip_checksum(ip);
                ip[10] = (uint8_t)(csum >> 8);
                ip[11] = (uint8_t)csum;

                apply_transform(payload, prbs31_at(u->prbs, seq), seq);

                if (raw_socket_send(&u->sock, buf, frame_len))
                    u->echoed++;
            } else if (vl >= c->rx_vl_start) {
                u->seen_own_echo++;
            }
            n = raw_socket_recv_nowait(&u->sock, buf, CMC_FRAME_LEN_MAX + 64);
        }
    }

    free(buf);
    return NULL;
}

/* ------------------------------------------------------------------ */

int main(void)
{
    const cmc_config_t c = smoke_config();
    prbs31_cache_t prbs = {0};
    prbs31_cache_t unit_prbs = {0};
    volatile bool stop = false;
    struct unit_ctx unit = {0};
    pthread_t unit_thread;
    cmc_data_plane_t *dp = NULL;
    int rc = 1;

    printf("CMC loopback smoke test: %s, %u VL(s), %.3f Gbps, %d s\n",
           SMOKE_IFACE, SMOKE_VL_COUNT, SMOKE_GBPS, SMOKE_SECONDS);

    printf("Generating the PRBS-31 stream twice (%zu MB each - one for each end, "
           "so an offset disagreement would show)...\n",
           CMC_PRBS_CACHE_SIZE / (1024 * 1024));
    if (!cmc_prbs_cache_init(&prbs, CMC_PRBS_INITIAL_STATE, NULL) ||
        !cmc_prbs_cache_init(&unit_prbs, CMC_PRBS_INITIAL_STATE, NULL)) {
        puts("[FAIL] the PRBS stream would not allocate");
        prbs31_cache_free(&prbs);
        prbs31_cache_free(&unit_prbs);
        return 1;
    }

    /* The stand-in comes up first, so nothing the sender emits is missed. */
    unit.config = &c;
    unit.stop = &stop;
    unit.prbs = &unit_prbs;
    if (!raw_socket_open(&unit.sock, SMOKE_IFACE, true)) {
        puts("[FAIL] could not open a raw socket on " SMOKE_IFACE
             " - this test needs root");
        goto out;
    }
    raw_socket_set_buffers(&unit.sock, 8 * 1024 * 1024, 4 * 1024 * 1024, NULL, NULL);
    if (pthread_create(&unit_thread, NULL, unit_thread_fn, &unit) != 0) {
        puts("[FAIL] could not start the stand-in");
        raw_socket_close(&unit.sock);
        goto out;
    }

    cmc_sink_t sink = {0};
    dp = cmc_data_plane_create(&c, &prbs, &sink, &stop);
    check(dp != NULL, "the data plane allocates");
    if (!dp)
        goto join;

    check(cmc_data_plane_open(dp), "the link opens");
    check(cmc_data_plane_start(dp), "the sender and receiver start");

    sleep(SMOKE_SECONDS);

    /* Stop sending, then give the last frames time to go round before stopping
     * the receivers - otherwise the frames in flight at that instant are
     * counted as sent and echoed but never verified, and the comparison below
     * fails for a reason that has nothing to do with the code. */
    cmc_data_plane_pause_tx(dp, true);
    usleep(200000);

    stop = true;
    cmc_data_plane_stop(dp);
    pthread_join(unit_thread, NULL);

    const cmc_net_stats_t *st = cmc_data_plane_stats(dp, 0);

    printf("\n  sent %" PRIu64 ", echoed by the stand-in %" PRIu64
           ", verified %" PRIu64 " good / %" PRIu64 " bad, lost %" PRIu64 "\n",
           st->tx_pkts, unit.echoed, st->good, st->bad, st->lost);
    /* The loopback device really does loop a transmitted frame back as an
     * incoming one, so on `lo` this end sees its own frames once however the
     * socket is set up. They carry the outbound VL range, so they land in the
     * unexpected-VL counter, and that number should be the number sent. On a
     * real link there is no loop and the counter stays at zero - which is why
     * this is reported rather than asserted either way. */
    printf("  frames on the link %" PRIu64 ", of which %" PRIu64 " are this end's "
           "own, looped back by %s and counted as unexpected VLs\n\n",
           st->frames, st->other_vl, SMOKE_IFACE);

    check(st->tx_pkts > 0, "the sender put frames on the wire");
    check(unit.echoed > 0, "the stand-in received them and sent them back");
    check(st->good > 0, "and the receiver verified what came back");
    check(st->bad == 0, "with nothing failing verification");
    check(st->lost == 0, "and no gaps in any VL's sequence");
    check(st->tx_refused == 0, "the link took every frame");
    check(st->short_pkts == 0, "nothing arrived too short to be a frame");

    /* The rate. Loopback is not a wire and the point is not the number, only
     * that pacing produced roughly what it was asked for rather than a burst or
     * a trickle: within a factor of two either way. */
    const double want = cmc_data_plane_net_gbps(dp) * 1e9 / 8.0 /
                        (double)CMC_FRAME_LEN(c.vlan_tagged) * SMOKE_SECONDS;
    printf("  paced %" PRIu64 " frames in %d s, asked for about %.0f\n",
           st->tx_pkts, SMOKE_SECONDS, want);
    check((double)st->tx_pkts > want / 2.0 && (double)st->tx_pkts < want * 2.0,
          "the pacing is in the region it was asked for");

    /* The strong one. Every frame the stand-in sent back was verified, once:
     * fewer means the receiver dropped some, more means it counted some twice -
     * which is what a socket handed its own transmissions does, and how the
     * option that stops that got here. */
    printf("  echoed %" PRIu64 ", verified %" PRIu64 "\n",
           unit.echoed, st->total_rx_pkts);
    /* Exact equality would be a race with whatever was in flight when the
     * threads stopped, so a few frames of slack - but only a few. The failure
     * this is here for doubles the count, which is nowhere near the band. */
    check(st->good <= unit.echoed && st->good + 4 >= unit.echoed,
          "every frame that came back was verified exactly once");
    check(st->other_vl == st->tx_pkts,
          "and this end's own frames, looped back by " SMOKE_IFACE
          ", are all accounted for");

    rc = failures ? 1 : 0;

join:
    stop = true;
    raw_socket_close(&unit.sock);
out:
    cmc_data_plane_destroy(dp);
    prbs31_cache_free(&prbs);
    prbs31_cache_free(&unit_prbs);

    if (rc == 0)
        puts("\nPASS: the CMC loop runs on a real socket, end to end");
    else
        printf("\nFAILED: %d check(s)\n", failures ? failures : 1);
    return rc;
}

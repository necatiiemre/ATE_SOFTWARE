#include "CmcPmm.h"

#include "CmcPayloadVerify.h"   /* sw_crc32c - the unit's table, not the standard one */
#include "Log.h"
#include "RawSocket.h"

#include <arpa/inet.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PMM_RCVBUF (8 * 1024 * 1024)
#define PMM_FRAME_MAX 2048

/* ------------------------------------------------------------------ */
/* CRC-32 (Ethernet/zlib, reflected polynomial 0xEDB88320)             */
/* CRC32C is in CmcPayloadVerify.h; both are tried when working out     */
/* which variant the unit uses.                                        */
/* ------------------------------------------------------------------ */

static uint32_t crc32_table[256];
static bool     crc32_table_ready;

static void crc32_table_init(void)
{
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;

        for (int k = 0; k < 8; k++)
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        crc32_table[i] = c;
    }
    crc32_table_ready = true;
}

static uint32_t sw_crc32(const void *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    const uint8_t *p = data;

    for (uint32_t i = 0; i < len; i++)
        crc = (crc >> 8) ^ crc32_table[(crc ^ p[i]) & 0xFF];
    return crc ^ 0xFFFFFFFFu;
}

/* Three unknowns, eight combinations: which algorithm, what it covers, and the
 * byte order of the field. The first one that matches is locked onto and named.
 * The verified one is first in the list, so on a healthy line the lock happens
 * on the first packet. */
struct pmm_crc_variant {
    const char *name;
    uint32_t  (*fn)(const void *, uint32_t);
    bool       cover_seq;    /* true: sequence and data; false: data alone */
    bool       big_endian;   /* the field's byte order on the wire */
};

static const struct pmm_crc_variant g_variants[] = {
    /* Verified: in a real captured packet (sequence bytes
     * 00 00 00 00 00 01 63 01, data 1024 x 0x01) the field held C0 76 40 C6,
     * and sw_crc32c over sequence+data produces exactly that. */
    {"CRC32C  seq+data    BE", sw_crc32c, true,  true },
    {"CRC32C  data        LE", sw_crc32c, false, false},
    {"CRC32C  data        BE", sw_crc32c, false, true },
    {"CRC32C  seq+data    LE", sw_crc32c, true,  false},
    {"CRC32   data        LE", sw_crc32,  false, false},
    {"CRC32   data        BE", sw_crc32,  false, true },
    {"CRC32   seq+data    LE", sw_crc32,  true,  false},
    {"CRC32   seq+data    BE", sw_crc32,  true,  true },
};
#define PMM_VARIANT_COUNT ((int)(sizeof g_variants / sizeof g_variants[0]))

struct cmc_pmm {
    const cmc_config_t *config;
    volatile bool      *stop;
    volatile bool       own_stop;

    raw_socket_t    sock[APP_MAX_CMC_PMMS];
    bool            opened[APP_MAX_CMC_PMMS];
    cmc_pmm_stats_t stats[APP_MAX_CMC_PMMS];
    uint32_t        unit_ip_be[APP_MAX_CMC_PMMS];
    uint32_t        local_ip_be[APP_MAX_CMC_PMMS];

    pthread_t thread[APP_MAX_CMC_PMMS];
    bool      thread_running[APP_MAX_CMC_PMMS];
    bool      running;

    int      crc_variant;      /**< -1 until one matches */
    uint64_t crc_probe_pkts;
    bool     crc_probe_warned;
    uint64_t unmatched;        /**< frames that were not MSG-shaped at all */
};

static uint64_t now_mono_ns(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static uint16_t be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }

static bool crc_variant_matches(int v, const smmm_msg_t *msg)
{
    const struct pmm_crc_variant *var = &g_variants[v];
    const void *start = var->cover_seq ? (const void *)msg : (const void *)msg->data;
    const uint32_t len = var->cover_seq ? (SMMM_SEQ_NUM_BYTE + SMMM_DATA_BYTE)
                                        : SMMM_DATA_BYTE;
    uint32_t recv = msg->data_crc;

    if (var->big_endian)
        recv = __builtin_bswap32(recv);
    return var->fn(start, len) == recv;
}

/* ------------------------------------------------------------------ */
/* Lifecycle                                                          */
/* ------------------------------------------------------------------ */

cmc_pmm_t *cmc_pmm_create(const cmc_config_t *config, volatile bool *stop)
{
    if (!config || config->pmm_count > APP_MAX_CMC_PMMS)
        return NULL;

    cmc_pmm_t *pmm = calloc(1, sizeof *pmm);
    if (!pmm)
        return NULL;

    if (!crc32_table_ready)
        crc32_table_init();

    pmm->config = config;
    pmm->stop = stop ? stop : &pmm->own_stop;
    pmm->crc_variant = -1;

    for (uint8_t i = 0; i < config->pmm_count; i++) {
        struct in_addr a;

        if (inet_pton(AF_INET, config->pmms[i].unit_ip, &a) == 1)
            pmm->unit_ip_be[i] = a.s_addr;
        if (inet_pton(AF_INET, config->pmms[i].local_ip, &a) == 1)
            pmm->local_ip_be[i] = a.s_addr;
    }
    return pmm;
}

void cmc_pmm_destroy(cmc_pmm_t *pmm)
{
    if (!pmm)
        return;
    cmc_pmm_stop(pmm);
    for (uint8_t i = 0; i < APP_MAX_CMC_PMMS; i++)
        if (pmm->opened[i])
            raw_socket_close(&pmm->sock[i]);
    free(pmm);
}

bool cmc_pmm_open(cmc_pmm_t *pmm)
{
    for (uint8_t i = 0; i < pmm->config->pmm_count; i++) {
        const cmc_pmm_link_t *link = &pmm->config->pmms[i];
        bool carrier = false;

        if (!raw_socket_link_up(link->iface, &carrier)) {
            log_line("[pmm] %s (%s) is not up", link->iface, link->label);
            return false;
        }
        if (!carrier)
            log_line("[pmm] %s (%s) is up but has no carrier - is the cable in?",
                     link->iface, link->label);

        if (!raw_socket_open(&pmm->sock[i], link->iface, true)) {
            log_line("[pmm] cannot open %s (%s)", link->iface, link->label);
            return false;
        }
        pmm->opened[i] = true;

        int rcv = 0;
        raw_socket_set_buffers(&pmm->sock[i], PMM_RCVBUF, 0, &rcv, NULL);
        log_line("[pmm] %-8s %-5s listening for %s:%u -> %s:%u  rcvbuf %d KB",
                 link->iface, link->label, link->unit_ip, link->unit_port,
                 link->local_ip, link->local_port, rcv / 1024);
    }
    return true;
}

void cmc_pmm_reset(cmc_pmm_t *pmm)
{
    for (uint8_t i = 0; i < pmm->config->pmm_count; i++)
        memset(&pmm->stats[i], 0, sizeof pmm->stats[i]);
    pmm->unmatched = 0;
    /* The CRC variant is not reset: it is a property of the unit, not of the
     * test window, and re-detecting it would cost the first packets again. */
}

const cmc_pmm_stats_t *cmc_pmm_stats(const cmc_pmm_t *pmm, uint8_t link)
{
    if (!pmm || link >= pmm->config->pmm_count)
        return NULL;
    return &pmm->stats[link];
}

const char *cmc_pmm_crc_variant(const cmc_pmm_t *pmm)
{
    if (!pmm || pmm->crc_variant < 0)
        return NULL;
    return g_variants[pmm->crc_variant].name;
}

bool cmc_pmm_running(const cmc_pmm_t *pmm) { return pmm && pmm->running; }

/* ------------------------------------------------------------------ */
/* Receive                                                            */
/* ------------------------------------------------------------------ */

/* Sequence continuity: loss, out of order, duplicate, and the jump that is
 * none of those. Lifted from the reference unchanged, including the two resync
 * cases - a stream that restarts must not put a billion packets in the loss
 * column. */
static void track_sequence(cmc_pmm_stats_t *st, uint64_t seq)
{
    st->last_seq = seq;

    if (!st->seq_initialized) {
        st->seq_initialized = true;
        st->expected_seq = seq + 1;
        return;
    }

    if (seq == st->expected_seq) {
        st->expected_seq = seq + 1;
    } else if (seq > st->expected_seq) {
        const uint64_t gap = seq - st->expected_seq;

        if (gap > SMMM_UDP_MAX_SEQ_GAP)
            st->resync_events++;
        else
            st->lost_pkts += gap;
        st->expected_seq = seq + 1;
    } else if (seq == st->expected_seq - 1) {
        st->duplicate_pkts++;
    } else if (st->expected_seq - seq > SMMM_UDP_MAX_SEQ_GAP) {
        st->resync_events++;
        st->expected_seq = seq + 1;
    } else {
        st->out_of_order_pkts++;
    }
}

static void check_crc(cmc_pmm_t *pmm, uint8_t link, const smmm_msg_t *msg, uint64_t seq)
{
    cmc_pmm_stats_t *st = &pmm->stats[link];

    if (pmm->crc_variant < 0) {
        for (int v = 0; v < PMM_VARIANT_COUNT; v++) {
            if (crc_variant_matches(v, msg)) {
                pmm->crc_variant = v;
                log_line("[pmm] CRC variant found: %s (after %" PRIu64 " packet(s))",
                         g_variants[v].name, pmm->crc_probe_pkts);
                break;
            }
        }
    }

    if (pmm->crc_variant < 0) {
        st->crc_unknown++;
        pmm->crc_probe_pkts++;
        if (!pmm->crc_probe_warned && pmm->crc_probe_pkts >= 64) {
            pmm->crc_probe_warned = true;
            log_line("[pmm] none of the eight known CRC variants matched the first "
                     "%" PRIu64 " packets (CRC32/CRC32C x data|seq+data x LE|BE) - "
                     "the definition may be something else; still trying",
                     pmm->crc_probe_pkts);
        }
        return;
    }

    if (crc_variant_matches(pmm->crc_variant, msg)) {
        st->crc_ok++;
        return;
    }

    st->crc_fail++;
    if (!st->first_bad_logged) {
        st->first_bad_logged = true;
        log_line("[pmm] %s first CRC failure: seq=%" PRIu64 " (variant %s)",
                 pmm->config->pmms[link].label, seq,
                 g_variants[pmm->crc_variant].name);
    }
}

void cmc_pmm_ingest(cmc_pmm_t *pmm, uint8_t link, const uint8_t *frame, size_t len)
{
    if (link >= pmm->config->pmm_count)
        return;

    const cmc_pmm_link_t *cfg = &pmm->config->pmms[link];
    cmc_pmm_stats_t *st = &pmm->stats[link];

    /* Which line a frame belongs to is the interface it arrived on, so there is
     * no VLAN to match - but a tag is still accepted, in case a switch is in
     * the path, and then it has to be this line's. */
    size_t l2 = SMMM_UDP_ETH_HDR_LEN;

    if (len < SMMM_UDP_ETH_HDR_LEN) {
        pmm->unmatched++;
        return;
    }
    if (be16(frame + 12) == 0x8100) {
        if (len < SMMM_UDP_ETH_HDR_LEN + SMMM_UDP_VLAN_HDR_LEN ||
            be16(frame + 16) != 0x0800) {
            pmm->unmatched++;
            return;
        }
        if (cfg->rx_vlan && (be16(frame + 14) & 0x0FFF) != cfg->rx_vlan) {
            pmm->unmatched++;
            return;
        }
        l2 = SMMM_UDP_ETH_HDR_LEN + SMMM_UDP_VLAN_HDR_LEN;
    } else if (be16(frame + 12) != 0x0800) {
        pmm->unmatched++;
        return;
    }

    st->rx_pkts++;
    st->rx_bytes += len;
    st->last_rx_ns = now_mono_ns();

    if (len < l2 + SMMM_UDP_IP_HDR_LEN + SMMM_UDP_L4_HDR_LEN) {
        st->bad_len_pkts++;
        return;
    }

    const uint8_t *ip = frame + l2;
    if (ip[9] != 17) {                          /* UDP */
        st->bad_len_pkts++;
        return;
    }

    const size_t ip_hdr_len = (size_t)(ip[0] & 0x0F) * 4;
    if (ip_hdr_len < SMMM_UDP_IP_HDR_LEN ||
        len < l2 + ip_hdr_len + SMMM_UDP_L4_HDR_LEN) {
        st->bad_len_pkts++;
        return;
    }

    const uint8_t *udp = frame + l2 + ip_hdr_len;

    /* Address consistency. A mismatch is counted, not dropped: a wrong
     * assumption here should show up in the table rather than silently throw
     * the line's traffic away. */
    uint32_t src_be, dst_be;
    memcpy(&src_be, ip + 12, sizeof src_be);
    memcpy(&dst_be, ip + 16, sizeof dst_be);
    if (src_be != pmm->unit_ip_be[link] || dst_be != pmm->local_ip_be[link] ||
        be16(udp + 0) != cfg->unit_port || be16(udp + 2) != cfg->local_port)
        st->addr_mismatch++;

    const uint16_t udp_len = be16(udp + 4);
    if (udp_len < SMMM_UDP_L4_HDR_LEN) {
        st->bad_len_pkts++;
        return;
    }

    size_t payload_len = (size_t)(udp_len - SMMM_UDP_L4_HDR_LEN);
    const uint8_t *payload = udp + SMMM_UDP_L4_HDR_LEN;
    const size_t avail = len - (size_t)(payload - frame);

    if (payload_len > avail)
        payload_len = avail;                    /* a truncated frame */

    size_t msg_off;
    if (payload_len == SMMM_UDP_PAYLOAD_NO_HDR) {
        msg_off = 0;
    } else if (payload_len == SMMM_UDP_PAYLOAD_WITH_HDR) {
        msg_off = SMMM_UDP_SMMM_HDR_LEN;
        st->hdr_present_pkts++;
    } else {
        st->bad_len_pkts++;
        return;
    }

    /* Nothing guarantees alignment in a frame off the wire, so it is copied and
     * read from the copy. The CRC is over the raw bytes either way; only the
     * sequence is byte-swapped, and only for the counters. */
    smmm_msg_t msg;
    memcpy(&msg, payload + msg_off, sizeof msg);

    const uint64_t seq = __builtin_bswap64(msg.seq_num);

    track_sequence(st, seq);
    check_crc(pmm, link, &msg, seq);
}

struct pmm_arg {
    cmc_pmm_t *pmm;
    uint8_t    link;
};

static void *pmm_thread_fn(void *arg)
{
    struct pmm_arg *a = arg;
    cmc_pmm_t *pmm = a->pmm;
    const uint8_t link = a->link;
    uint8_t *buf = malloc(PMM_FRAME_MAX);

    free(a);
    if (!buf)
        return NULL;

    while (!*pmm->stop) {
        const int n = raw_socket_recv(&pmm->sock[link], buf, PMM_FRAME_MAX, 200);

        if (n <= 0)
            continue;
        cmc_pmm_ingest(pmm, link, buf, (size_t)n);
    }
    free(buf);
    return NULL;
}

bool cmc_pmm_start(cmc_pmm_t *pmm)
{
    for (uint8_t i = 0; i < pmm->config->pmm_count; i++) {
        struct pmm_arg *a = malloc(sizeof *a);

        if (!a)
            return false;
        a->pmm = pmm;
        a->link = i;
        if (pthread_create(&pmm->thread[i], NULL, pmm_thread_fn, a) != 0) {
            log_line("[pmm] cannot start the listener for %s",
                     pmm->config->pmms[i].iface);
            free(a);
            return false;
        }
        pmm->thread_running[i] = true;
    }
    pmm->running = true;
    return true;
}

void cmc_pmm_stop(cmc_pmm_t *pmm)
{
    for (uint8_t i = 0; i < APP_MAX_CMC_PMMS; i++) {
        if (pmm->thread_running[i]) {
            pthread_join(pmm->thread[i], NULL);
            pmm->thread_running[i] = false;
        }
    }
    pmm->running = false;
}

/* ------------------------------------------------------------------ */
/* The table                                                          */
/* ------------------------------------------------------------------ */

void cmc_pmm_print_table(cmc_pmm_t *pmm)
{
    const uint64_t now = now_mono_ns();

    printf("\n  === SMMM UDP RX (%s", pmm->config->pmms[0].iface);
    for (uint8_t i = 1; i < pmm->config->pmm_count; i++)
        printf("/%s", pmm->config->pmms[i].iface);
    printf(" | listen only");
    if (pmm->crc_variant >= 0)
        printf(" | CRC: %s) ===\n", g_variants[pmm->crc_variant].name);
    else
        printf(" | looking for the CRC variant) ===\n");

    /* The reference stops here when its listener is not running. This also
     * prints the table when something has arrived on a line, because a listener
     * that has stopped after collecting a run's traffic still has the run's
     * numbers, and losing them to a one-line message would be a poor trade. */
    bool any = false;
    for (uint8_t i = 0; i < pmm->config->pmm_count; i++)
        any = any || pmm->stats[i].rx_pkts > 0;

    if (!pmm->running && !any) {
        printf("      The listener is not running\n");
        return;
    }

    printf("  ┌─────────┬────────┬────────────────────────────────────────────────────────────────────────────────────────────┬──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐\n");
    printf("  │ Server  │  SMMM  │                                 SMMM UDP RX (SMMM->Server)                                 │                                                                                                 Payload Verification                                                                                                 │\n");
    printf("  │  Port   │  Line  ├──────────────────────────────┬──────────────────────────────┬──────────────────────────────┼──────────────────────────────┬──────────────────────────────┬──────────────────────────────┬──────────────────────────────┬──────────────────────────────┬──────────────────────────────┬────────────────────────────┤\n");
    printf("  │         │        │           Packets            │            Bytes             │             Gbps             │            CRC OK            │           CRC Fail           │             Loss             │         Out-of-Order         │          Duplicate           │          Bad Length          │          Last Seq          │\n");
    printf("  ├─────────┼────────┼──────────────────────────────┼──────────────────────────────┼──────────────────────────────┼──────────────────────────────┼──────────────────────────────┼──────────────────────────────┼──────────────────────────────┼──────────────────────────────┼──────────────────────────────┼────────────────────────────┤\n");

    for (uint8_t i = 0; i < pmm->config->pmm_count; i++) {
        cmc_pmm_stats_t *st = &pmm->stats[i];

        double elapsed_s = (double)pmm->config->stats_interval_s;
        if (st->prev_print_ns != 0 && now > st->prev_print_ns)
            elapsed_s = (double)(now - st->prev_print_ns) / 1e9;
        if (elapsed_s <= 0.0)
            elapsed_s = 1.0;

        const double gbps =
            ((double)(st->rx_bytes - st->prev_rx_bytes) * 8.0) / (elapsed_s * 1e9);

        st->prev_rx_bytes = st->rx_bytes;
        st->prev_print_ns = now;

        printf("  │ %-7s │ %-6s │ %28" PRIu64 " │ %28" PRIu64 " │ %28.4f │ %28" PRIu64
               " │ %28" PRIu64 " │ %28" PRIu64 " │ %28" PRIu64 " │ %28" PRIu64
               " │ %28" PRIu64 " │ %26" PRIu64 " │\n",
               pmm->config->pmms[i].iface, pmm->config->pmms[i].label,
               st->rx_pkts, st->rx_bytes, gbps,
               st->crc_ok, st->crc_fail, st->lost_pkts,
               st->out_of_order_pkts, st->duplicate_pkts, st->bad_len_pkts,
               st->last_seq);
    }

    printf("  └─────────┴────────┴──────────────────────────────┴──────────────────────────────┴──────────────────────────────┴──────────────────────────────┴──────────────────────────────┴──────────────────────────────┴──────────────────────────────┴──────────────────────────────┴──────────────────────────────┴────────────────────────────┘\n");

    /* Line state */
    printf("     ");
    for (uint8_t i = 0; i < pmm->config->pmm_count; i++) {
        const cmc_pmm_stats_t *st = &pmm->stats[i];
        const char *status;

        if (st->last_rx_ns == 0)
            status = "NO DATA";
        else if (now - st->last_rx_ns > (uint64_t)SMMM_UDP_STALE_S * 1000000000ull)
            status = "STALE";
        else
            status = "LIVE";
        printf(" %s: %-8s", pmm->config->pmms[i].label, status);
    }
    printf("\n");

    /* Warnings */
    bool warned = false;
    for (uint8_t i = 0; i < pmm->config->pmm_count; i++) {
        const cmc_pmm_stats_t *st = &pmm->stats[i];

        if (st->crc_fail == 0 && st->lost_pkts == 0 && st->bad_len_pkts == 0)
            continue;
        if (!warned) {
            printf("\n  SMMM UDP WARNINGS:\n");
            warned = true;
        }
        if (st->crc_fail > 0)
            printf("      %s: %" PRIu64 " CRC failure(s)!\n",
                   pmm->config->pmms[i].label, st->crc_fail);
        if (st->lost_pkts > 0)
            printf("      %s: %" PRIu64 " lost packet(s)!\n",
                   pmm->config->pmms[i].label, st->lost_pkts);
        if (st->bad_len_pkts > 0)
            printf("      %s: %" PRIu64 " packet(s) of an invalid length!\n",
                   pmm->config->pmms[i].label, st->bad_len_pkts);
    }

    printf("      Diag:");
    for (uint8_t i = 0; i < pmm->config->pmm_count; i++) {
        const cmc_pmm_stats_t *st = &pmm->stats[i];

        printf("  %s[CRCpending:%" PRIu64 " AddrMismatch:%" PRIu64
               " SMMMhdr:%" PRIu64 " Resync:%" PRIu64 "]",
               pmm->config->pmms[i].label, st->crc_unknown, st->addr_mismatch,
               st->hdr_present_pkts, st->resync_events);
    }
    printf("  Unmatched RX: %" PRIu64 "\n", pmm->unmatched);
}

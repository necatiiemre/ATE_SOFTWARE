#include "Ptp.h"
#include "PacketDefs.h"
#include "RawSocket.h"
#include "TestScenarios.h"   /* g_stopRequested */

#include <stdio.h>
#include <string.h>
#include <time.h>

/* ====================================================================
 *  Wire format - identical to dpdk_vmc include/ptp/ptp_wire.h
 * ==================================================================== */

#define PTP_ETHERTYPE        0x88F7
#define PTP_MSG_SYNC         0x00
#define PTP_MSG_DELAY_REQ    0x01
#define PTP_MSG_DELAY_RESP   0x09

#define PTP_BODY_LEN         106
#define PTP_FRAME_NOVLAN_LEN 120     /* eth 14 + body 106 */
#define PTP_FRAME_VLAN_LEN   124     /* + 802.1Q */

#define PTP_DATA_LEN         35
#define PTP_BODY_MSG_OFF     0
#define PTP_BODY_DATA_OFF    1
#define PTP_BODY_TIME1_OFF   (PTP_BODY_DATA_OFF + PTP_DATA_LEN)   /* 36 */
#define PTP_BODY_FILLER_OFF  (PTP_BODY_TIME1_OFF + 8)             /* 44 */
#define PTP_BODY_TIME2_OFF   (PTP_BODY_FILLER_OFF + 1)            /* 45 */

static const uint8_t PTP_DATA_MASTER_BLOB[PTP_DATA_LEN] = {
    0x02, 0x00, 0x6a, 0x0b, 0x00, 0x01, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x6d, 0x61, 0x73, 0x74, 0x65,
    0x72, 0x64, 0x65, 0x76, 0x2e, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x00
};
static const uint8_t PTP_DATA_SLAVE_BLOB[PTP_DATA_LEN] = {
    0x02, 0x00, 0x6a, 0x0a, 0x00, 0x01, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x73, 0x6c, 0x61, 0x76, 0x65,
    0x5f, 0x64, 0x65, 0x76, 0x2e, 0x00, 0x01, 0x01,
    0xff, 0x12, 0x13
};

static void putTime(uint8_t *p, uint64_t ns)
{
    const uint32_t sec = (uint32_t)(ns / 1000000000ULL);
    const uint32_t rem = (uint32_t)(ns % 1000000000ULL);
    p[0] = (uint8_t)(sec >> 24); p[1] = (uint8_t)(sec >> 16); p[2] = (uint8_t)(sec >> 8); p[3] = (uint8_t)sec;
    p[4] = (uint8_t)(rem >> 24); p[5] = (uint8_t)(rem >> 16); p[6] = (uint8_t)(rem >> 8); p[7] = (uint8_t)rem;
}

static uint64_t getTime(const uint8_t *p)
{
    const uint32_t sec = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
    const uint32_t ns  = ((uint32_t)p[4] << 24) | ((uint32_t)p[5] << 16) | ((uint32_t)p[6] << 8) | p[7];
    return (uint64_t)sec * 1000000000ULL + ns;
}

static uint8_t neByte(Network net)
{
    return (net == NET_A) ? NET_A_SRC_MAC_TAIL : NET_B_SRC_MAC_TAIL;
}

/* eth + 802.1Q + proprietary body (dpdk_vmc build_proprietary_frame). */
static void buildPtpFrame(uint8_t *f, Network net, uint16_t vlan, uint16_t vl,
                          uint8_t msg, const uint8_t *blob, uint64_t time1Ns)
{
    memset(f, 0, PTP_FRAME_VLAN_LEN);
    f[0] = 0x03; f[4] = (uint8_t)(vl >> 8); f[5] = (uint8_t)vl;      /* DST 03:00:00:00:VV:VV */
    f[6] = 0x02; f[11] = neByte(net);                                 /* SRC 02:00:00:00:00:NE */
    f[12] = 0x81; f[13] = 0x00;
    f[14] = (uint8_t)((vlan >> 8) & 0x0F); f[15] = (uint8_t)vlan;     /* priority 0 */
    f[16] = (uint8_t)(PTP_ETHERTYPE >> 8); f[17] = (uint8_t)PTP_ETHERTYPE;

    uint8_t *body = f + 18;
    body[PTP_BODY_MSG_OFF] = msg;
    memcpy(body + PTP_BODY_DATA_OFF, blob, PTP_DATA_LEN);
    putTime(body + PTP_BODY_TIME1_OFF, time1Ns);
    /* filler, TIME_2 and padding stay 0 */
}

/* ====================================================================
 *  Time / formatting helpers
 * ==================================================================== */

static uint64_t realNs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint64_t monoNs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static const char *msgName(uint8_t m)
{
    switch (m) {
    case PTP_MSG_SYNC:       return "Sync";
    case PTP_MSG_DELAY_REQ:  return "Req ";
    case PTP_MSG_DELAY_RESP: return "Resp";
    default:                 return "?   ";
    }
}

/* Signed nanosecond delta in a readable unit (clocks of the two sides may be
 * hours/days apart, same as dpdk_vmc's fmt_dur_us). */
static void fmtDelta(int64_t ns, char *out, size_t n)
{
    const double us = (double)ns / 1000.0;
    const double a = us < 0 ? -us : us;
    const char sign = us < 0 ? '-' : '+';
    if (a >= 86400.0 * 1e6)      snprintf(out, n, "%c%.3f d",  sign, a / (86400.0 * 1e6));
    else if (a >= 3600.0 * 1e6)  snprintf(out, n, "%c%.3f h",  sign, a / (3600.0 * 1e6));
    else if (a >= 1e6)           snprintf(out, n, "%c%.3f s",  sign, a / 1e6);
    else if (a >= 1e3)           snprintf(out, n, "%c%.3f ms", sign, a / 1e3);
    else                         snprintf(out, n, "%c%.3f us", sign, a);
}

static void fmtTime(uint64_t ns, char *out, size_t n)
{
    snprintf(out, n, "%llu.%09llu", (unsigned long long)(ns / 1000000000ULL),
             (unsigned long long)(ns % 1000000000ULL));
}

/* Sleep `seconds`, waking early on Ctrl+C. */
static void waitSeconds(unsigned seconds)
{
    for (unsigned i = 0; i < seconds && !g_stopRequested; ++i) {
        struct timespec ts = { 1, 0 };
        nanosleep(&ts, NULL);
    }
}

/* ====================================================================
 *  Per-leg, per-phase state
 * ==================================================================== */

typedef struct {
    /* VMC master side: VMC Sync -> our Req -> VMC Resp */
    uint32_t syncRx, reqTx, respRx, respTimeout, lateResp;
    bool     reqOutstanding;
    uint64_t reqTxMono;
    uint64_t t1, t2, t3, t4;          /* last T1 (VMC) T2 (us) T3 (us) T4 (VMC) */

    /* Our master side: our Sync -> VMC Req -> our Resp */
    uint32_t syncTx, reqRx, respTx;
    uint64_t nextSyncMono;
    uint64_t mT1, mT3, mT4;           /* last T1 (us) T3 (VMC) T4 (us) */

    /* Errors */
    uint32_t txFail, badNe, badSize, unexpected;
    uint32_t kickReq;                 /* phase 1: VMC Req answering the kick Sync (ignored) */
} LegState;

typedef enum { PH_GRANDMASTER = 0, PH_MASTER = 1, PH_SLAVE = 2 } PhaseId;

static const char *phaseName(PhaseId p)
{
    switch (p) {
    case PH_GRANDMASTER: return "VMC GRANDMASTER";
    case PH_MASTER:      return "VMC MASTER";
    default:             return "VMC SLAVE";
    }
}

typedef struct {
    const PtpConfig *cfg;
    PhaseId  phase;
    bool     vmcMaster;      /* phases 1, 2: VMC sends Sync, we answer with Req */
    bool     weMaster;       /* phases 2, 3: we send Sync, VMC answers with Req */
    bool     started;
    bool     ending;         /* duration over: no new Sync/Req, collect late answers */
    uint32_t unmatched;      /* 0x88F7 frames on no known VL-IDX */
    RawSocket sock;
    LegState legs[PTP_MAX_LEGS];
} PhaseCtx;

static bool sendFrame(PhaseCtx *c, LegState *st, const PtpLeg *leg, uint8_t msg,
                      uint16_t vl, const uint8_t *blob, uint64_t time1)
{
    uint8_t f[PTP_FRAME_VLAN_LEN];
    buildPtpFrame(f, leg->net, leg->txVlan, vl, msg, blob, time1);
    if (rawSocketSend(&c->sock, f, sizeof(f)) != 0) {
        st->txFail++;
        return false;
    }
    if (c->cfg->trace) {
        char t[32];
        fmtTime(time1, t, sizeof(t));
        printf("[PTP] TX %s %-10s VLAN %4u VL %5u  TIME_1 %s\n",
               msgName(msg), leg->label, leg->txVlan, vl, t);
    }
    return true;
}

/* ---- incoming frame dispatch -------------------------------------- */

static void handleFrame(PhaseCtx *c, const uint8_t *f, const RawRxInfo *rx, uint64_t mono)
{
    const PtpConfig *cfg = c->cfg;
    const uint16_t vl  = (uint16_t)((f[4] << 8) | f[5]);
    const uint8_t  ne  = f[11];
    const uint8_t *body = f + 14;                 /* VLAN tag already removed */
    const uint8_t  msg = body[PTP_BODY_MSG_OFF];

    for (int i = 0; i < cfg->legCount; ++i) {
        const PtpLeg *leg = &cfg->legs[i];
        LegState *st = &c->legs[i];

        int kind = 0;                             /* 1 Sync, 2 Resp, 3 Req */
        if (vl == leg->vmcSyncVl) kind = 1;
        else if (vl == leg->vmcRespVl) kind = 2;
        else if (vl == leg->vmcReqVl) kind = 3;
        else continue;

        if (rx->len < PTP_FRAME_NOVLAN_LEN) { st->badSize++; return; }

        const uint8_t wantMsg = (kind == 1) ? PTP_MSG_SYNC
                              : (kind == 2) ? PTP_MSG_DELAY_RESP : PTP_MSG_DELAY_REQ;
        if (msg != wantMsg) { st->unexpected++; return; }

        if (cfg->trace) {
            char t[32];
            fmtTime(getTime(body + PTP_BODY_TIME1_OFF), t, sizeof(t));
            printf("[PTP] RX %s %-10s VLAN %4u VL %5u  TIME_1 %s\n", msgName(msg), leg->label,
                   rx->hasVlan ? rx->vlanId : 0, vl, t);
        }

        if (kind == 1) {                          /* VMC Sync */
            if (!c->vmcMaster) { st->unexpected++; return; }
            if (ne != neByte(leg->net)) { st->badNe++; return; }
            if (!c->started) {
                c->started = true;
                printf("[PTP] first VMC Sync on %s - %s phase starts (%u s)\n",
                       leg->label, phaseName(c->phase), cfg->grandmasterS);
            }
            if (c->ending) return;                /* phase over: no new Req */

            if (st->reqOutstanding) {             /* previous Req never answered */
                st->respTimeout++;
                st->reqOutstanding = false;
            }
            st->syncRx++;
            st->t1 = getTime(body + PTP_BODY_TIME1_OFF);
            st->t2 = rx->rxTimeNs;

            const uint64_t t3 = realNs();
            if (sendFrame(c, st, leg, PTP_MSG_DELAY_REQ, leg->ourReqVl, PTP_DATA_SLAVE_BLOB, t3)) {
                st->reqTx++;
                st->t3 = t3;
                st->reqOutstanding = true;
                st->reqTxMono = mono;
            }
        } else if (kind == 2) {                   /* VMC Delay_Resp */
            if (!c->vmcMaster) { st->unexpected++; return; }
            if (!st->reqOutstanding) { st->lateResp++; return; }
            st->respRx++;
            st->t4 = getTime(body + PTP_BODY_TIME1_OFF);
            st->reqOutstanding = false;
        } else {                                  /* VMC Delay_Req */
            if (c->phase == PH_GRANDMASTER) { st->kickReq++; return; }   /* answer to the kick Sync */
            if (!c->weMaster || !c->started) { st->unexpected++; return; }
            st->reqRx++;
            st->mT3 = getTime(body + PTP_BODY_TIME1_OFF);
            st->mT4 = rx->rxTimeNs;
            /* Our RX time goes back in TIME_1 (dpdk_vmc build_delay_resp). */
            if (sendFrame(c, st, leg, PTP_MSG_DELAY_RESP, leg->ourRespVl, PTP_DATA_MASTER_BLOB, st->mT4))
                st->respTx++;
        }
        return;
    }
    c->unmatched++;
}

/* ---- status / result ----------------------------------------------- */

static void printLegLine(const PhaseCtx *c, int i)
{
    const PtpLeg *leg = &c->cfg->legs[i];
    const LegState *s = &c->legs[i];
    char a[24], b[24], d[24];

    printf("[PTP]   %-10s", leg->label);
    if (c->vmcMaster) {
        printf(" | VMC-master: Sync %3u Req %3u Resp %3u TO %u", s->syncRx, s->reqTx, s->respRx, s->respTimeout);
        if (s->t1 && s->t2 && s->t3 && s->t4) {
            const int64_t dSync = (int64_t)(s->t2 - s->t1);
            const int64_t dReq  = (int64_t)(s->t4 - s->t3);
            fmtDelta((dSync - dReq) / 2, a, sizeof(a));
            fmtDelta((dSync + dReq) / 2, b, sizeof(b));
            printf(" offset %s delay %s", a, b);
        }
    }
    if (c->weMaster) {
        printf(" | we-master: Sync %3u Req %3u Resp %3u", s->syncTx, s->reqRx, s->respTx);
        if (s->mT3 && s->mT4) {
            fmtDelta((int64_t)(s->mT4 - s->mT3), d, sizeof(d));
            printf(" T4-T3 %s", d);
        }
    }
    if (s->txFail || s->badNe || s->badSize || s->unexpected || s->lateResp)
        printf(" | err: txFail %u badNe %u badSize %u unexpected %u lateResp %u",
               s->txFail, s->badNe, s->badSize, s->unexpected, s->lateResp);
    printf("\n");
}

static bool legPassed(const PhaseCtx *c, int i, char *why, size_t n)
{
    const LegState *s = &c->legs[i];
    why[0] = 0;
    if (c->vmcMaster) {
        if (s->syncRx == 0)                   { snprintf(why, n, "no VMC Sync"); return false; }
        if (s->respTimeout > 0)               { snprintf(why, n, "%u Delay_Resp timeout(s)", s->respTimeout); return false; }
        if (s->respRx != s->reqTx)            { snprintf(why, n, "Resp %u != Req %u", s->respRx, s->reqTx); return false; }
    }
    if (c->weMaster) {
        if (s->syncTx == 0)                   { snprintf(why, n, "no Sync sent"); return false; }
        if (s->reqRx == 0)                    { snprintf(why, n, "no VMC Delay_Req"); return false; }
        if (s->respTx != s->reqRx)            { snprintf(why, n, "Resp sent %u != Req %u", s->respTx, s->reqRx); return false; }
    }
    if (s->txFail > 0)                        { snprintf(why, n, "%u send failure(s)", s->txFail); return false; }
    return true;
}

/* ---- one phase ----------------------------------------------------- */

static bool runPhase(const PtpConfig *cfg, PhaseId phase, unsigned durationS, int num)
{
    static PhaseCtx c;                            /* large-ish, keep it off the stack */
    memset(&c, 0, sizeof(c));
    c.cfg       = cfg;
    c.phase     = phase;
    c.vmcMaster = (phase == PH_GRANDMASTER || phase == PH_MASTER);
    c.weMaster  = (phase == PH_MASTER || phase == PH_SLAVE);

    printf("\n[PTP] ===== Phase %d/3: %s (%u s) =====\n", num, phaseName(phase), durationS);
    if (c.vmcMaster)
        printf("[PTP]   VMC -> us Sync, us -> VMC Delay_Req, VMC -> us Delay_Resp\n");
    if (c.weMaster)
        printf("[PTP]   us -> VMC Sync every %u ms, VMC -> us Delay_Req, us -> VMC Delay_Resp\n",
               cfg->syncIntervalMs);
    for (int i = 0; i < cfg->legCount; ++i) {
        const PtpLeg *l = &cfg->legs[i];
        printf("[PTP]   %-10s %s  TX VLAN %u", l->label, networkName(l->net), l->txVlan);
        if (c.vmcMaster) printf(" | VMC-master VL %u/%u/%u", l->vmcSyncVl, l->ourReqVl, l->vmcRespVl);
        if (c.weMaster)  printf(" | we-master VL %u/%u/%u", l->ourSyncVl, l->vmcReqVl, l->ourRespVl);
        printf("\n");
    }

    /* Fresh socket per phase: nothing queued during the gap is processed. */
    if (rawSocketOpenRx(&c.sock, cfg->ifname) != 0) return false;

    const uint64_t ms = 1000000ULL;
    const uint64_t t0 = monoNs();
    uint64_t endMono = 0, graceEnd = 0;
    uint64_t nextDash = t0 + (uint64_t)cfg->dashboardS * 1000 * ms;
    bool noSync = false;

    if (phase == PH_GRANDMASTER) {
        if (cfg->kickSync) {
            /* The VMC starts its own Sync only after it has seen one from us. */
            for (int i = 0; i < cfg->legCount; ++i) {
                const PtpLeg *l = &cfg->legs[i];
                if (sendFrame(&c, &c.legs[i], l, PTP_MSG_SYNC, l->ourSyncVl, PTP_DATA_MASTER_BLOB, realNs()))
                    printf("[PTP] kick: 1 Sync sent on %s (VL %u) to start the VMC Sync\n",
                           l->label, l->ourSyncVl);
                else
                    printf("[PTP] WARNING: kick Sync on %s could not be sent\n", l->label);
            }
        }
        printf("[PTP] waiting up to %u s for the first VMC Sync...\n", cfg->firstSyncWaitS);
    } else {
        c.started = true;                        /* starts with our first Sync */
        endMono = t0 + (uint64_t)durationS * 1000 * ms;
        for (int i = 0; i < cfg->legCount; ++i) c.legs[i].nextSyncMono = t0;
    }

    uint8_t buf[2048];
    while (!g_stopRequested) {
        const uint64_t now = monoNs();

        if (!c.started) {
            if (now - t0 >= (uint64_t)cfg->firstSyncWaitS * 1000 * ms) { noSync = true; break; }
        } else if (endMono == 0) {
            endMono = now + (uint64_t)durationS * 1000 * ms;   /* GM: from the first Sync */
        }

        if (c.started && !c.ending && now >= endMono) {
            c.ending = true;
            graceEnd = now + (uint64_t)cfg->respTimeoutMs * ms;
            printf("[PTP] %s duration over%s - collecting answers for %u ms\n", phaseName(phase),
                   c.weMaster ? ", our Sync stopped" : "", cfg->respTimeoutMs);
        }
        if (c.ending && now >= graceEnd) break;

        /* Our Sync */
        if (c.weMaster && c.started && !c.ending) {
            for (int i = 0; i < cfg->legCount; ++i) {
                LegState *st = &c.legs[i];
                if (now < st->nextSyncMono) continue;
                st->nextSyncMono += (uint64_t)cfg->syncIntervalMs * ms;
                if (st->nextSyncMono <= now) st->nextSyncMono = now + (uint64_t)cfg->syncIntervalMs * ms;
                const uint64_t t1 = realNs();
                if (sendFrame(&c, st, &cfg->legs[i], PTP_MSG_SYNC, cfg->legs[i].ourSyncVl,
                              PTP_DATA_MASTER_BLOB, t1)) {
                    st->syncTx++;
                    st->mT1 = t1;
                }
            }
        }

        /* Delay_Req watchdog */
        for (int i = 0; i < cfg->legCount; ++i) {
            LegState *st = &c.legs[i];
            if (st->reqOutstanding && now - st->reqTxMono > (uint64_t)cfg->respTimeoutMs * ms) {
                st->respTimeout++;
                st->reqOutstanding = false;
                printf("[PTP] WARNING: %s Delay_Req got no Delay_Resp within %u ms\n",
                       cfg->legs[i].label, cfg->respTimeoutMs);
            }
        }

        if (cfg->dashboardS && c.started && now >= nextDash) {
            nextDash = now + (uint64_t)cfg->dashboardS * 1000 * ms;
            printf("[PTP] --- %s status, %.0f s ---\n", phaseName(phase), (double)(now - t0) / 1e9);
            for (int i = 0; i < cfg->legCount; ++i) printLegLine(&c, i);
        }

        RawRxInfo info;
        const int r = rawSocketRecv(&c.sock, buf, sizeof(buf), &info, 5);
        if (r < 0) break;
        if (r == 1 && info.etherType == PTP_ETHERTYPE) handleFrame(&c, buf, &info, monoNs());
    }

    rawSocketClose(&c.sock);

    /* ---- phase result ---- */
    bool ok = !g_stopRequested;
    printf("[PTP] ----- Phase %d/3 %s result -----\n", num, phaseName(phase));
    if (noSync) {
        printf("[PTP]   no VMC Sync within %u s - phase stopped\n", cfg->firstSyncWaitS);
        ok = false;
    }
    for (int i = 0; i < cfg->legCount; ++i) {
        char why[64] = "no VMC Sync";
        const bool legOk = !noSync && legPassed(&c, i, why, sizeof(why));
        printLegLine(&c, i);
        printf("[PTP]   %-10s %s%s%s\n", cfg->legs[i].label, legOk ? "PASS" : "FAIL",
               (!legOk && why[0]) ? " - " : "", (!legOk && why[0]) ? why : "");
        ok &= legOk;
    }
    for (int i = 0; i < cfg->legCount; ++i)
        if (c.legs[i].kickReq)
            printf("[PTP]   %s: %u VMC Delay_Req answering the kick Sync (ignored)\n",
                   cfg->legs[i].label, c.legs[i].kickReq);
    if (c.unmatched) printf("[PTP]   %u PTP frame(s) on unknown VL-IDX ignored\n", c.unmatched);
    printf("[PTP] Phase %d/3 %s: %s\n", num, phaseName(phase), ok ? "PASS" : "FAIL");
    return ok;
}

/* ==================================================================== */

bool casePtp(const PtpConfig *cfg)
{
    if (!cfg || !cfg->ifname || !cfg->legs || cfg->legCount <= 0 || cfg->legCount > PTP_MAX_LEGS ||
        cfg->syncIntervalMs == 0) {
        fprintf(stderr, "[PTP] invalid configuration\n");
        return false;
    }

    printf("[PTP] interface %s, %d leg(s), phases %u/%u/%u s, gap %u s\n", cfg->ifname,
           cfg->legCount, cfg->grandmasterS, cfg->masterS, cfg->slaveS, cfg->gapS);

    const PhaseId  order[3] = { PH_GRANDMASTER, PH_MASTER, PH_SLAVE };
    const unsigned dur[3]   = { cfg->grandmasterS, cfg->masterS, cfg->slaveS };
    bool result[3] = { false, false, false };
    bool ran[3]    = { false, false, false };

    for (int p = 0; p < 3 && !g_stopRequested; ++p) {
        if (p > 0) {
            printf("\n[PTP] waiting %u s before phase %d/3 (%s)...\n", cfg->gapS, p + 1, phaseName(order[p]));
            waitSeconds(cfg->gapS);
            if (g_stopRequested) break;
        }
        result[p] = runPhase(cfg, order[p], dur[p], p + 1);
        ran[p] = true;
    }

    printf("\n[PTP] ===== summary =====\n");
    bool ok = !g_stopRequested;
    for (int p = 0; p < 3; ++p) {
        printf("[PTP]   %d. %-16s %s\n", p + 1, phaseName(order[p]),
               !ran[p] ? "NOT RUN" : result[p] ? "PASS" : "FAIL");
        ok &= ran[p] && result[p];
    }
    return ok;
}

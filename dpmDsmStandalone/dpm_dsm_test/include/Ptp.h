#ifndef VMC_TEST_PTP_H_
#define VMC_TEST_PTP_H_

/*
 * PTP scenario (raw socket port of dpdk_vmc's src/ptp), dpmDsmStandalone copy.
 * The comments below say "VMC" for the unit under test; at run time every
 * message carries the selected unit's name (unitName), and only the enabled
 * phases run (DSM: all three, exactly as VMC; DPM: slave only).
 *
 * Wire format: the proprietary 124-byte frame of dpdk_vmc (ptp_wire.h),
 * NOT IEEE 1588:
 *
 *   DST MAC 03:00:00:00:VV:VV | SRC MAC 02:00:00:00:00:NE | 8100 VLAN | 88F7
 *   body[0]      msg type  0x00 Sync / 0x01 Delay_Req / 0x09 Delay_Resp
 *   body[1..35]  fixed blob (master blob in Sync/Resp, slave blob in Req)
 *   body[36..43] TIME_1 = 4 B seconds + 4 B nanoseconds, big endian
 *   body[44]     0x00, body[45..52] TIME_2 (0), rest zero padding
 *
 * Phases (run in this order, gapS seconds apart):
 *
 *   1. VMC GRANDMASTER - VMC only sends Sync. With kickSync, we first sync
 *      the VMC: ONE Sync per leg (ourSyncVl 1500/1520) -> VMC Delay_Req
 *      (vmcReqVl 1501/1521) -> our Delay_Resp (ourRespVl 1502/1522). Only
 *      then does the VMC start its own Sync. The phase starts with the first
 *      VMC Sync (none within firstSyncWaitS -> phase FAIL), runs grandmasterS:
 *        VMC -> us   Sync      vmcSyncVl  (1600 / 1620)
 *        us  -> VMC  Delay_Req ourReqVl   (1601 / 1621)   one per Sync
 *        VMC -> us   Delay_Resp vmcRespVl (1602 / 1622)
 *
 *   2. VMC MASTER - VMC is master AND slave at the same time. Starts with our
 *      first Sync, runs masterS; stopping our Sync tells the VMC it is over:
 *        the grandmaster exchange above, plus
 *        us  -> VMC  Sync       ourSyncVl  (1500 / 1520)  every syncIntervalMs
 *        VMC -> us   Delay_Req  vmcReqVl   (1501 / 1521)
 *        us  -> VMC  Delay_Resp ourRespVl  (1502 / 1522)  TIME_1 = our RX time
 *
 *   3. VMC SLAVE - only the "us -> VMC Sync" exchange of phase 2, slaveS.
 *
 * Received frames are matched on VL-IDX + message type (VL-IDX values are
 * unique); the VLAN is only logged. Every frame we send carries the leg's
 * txVlan.
 */

#include <stdbool.h>
#include <stdint.h>

#include "Packet.h"

typedef struct {
    const char *label;      /* e.g. "FLCS J6-1" */
    Network     net;        /* NE in SRC MAC: NET_A = 0x20 (FLCS), NET_B = 0x40 (VS) */
    uint16_t    txVlan;     /* 802.1Q VLAN of every frame we send on this leg */

    /* VMC is master (phases 1 and 2) */
    uint16_t    vmcSyncVl;  /* Sync       VMC -> us   */
    uint16_t    ourReqVl;   /* Delay_Req  us  -> VMC  */
    uint16_t    vmcRespVl;  /* Delay_Resp VMC -> us   */

    /* We are master (phases 2 and 3) */
    uint16_t    ourSyncVl;  /* Sync       us  -> VMC  */
    uint16_t    vmcReqVl;   /* Delay_Req  VMC -> us   */
    uint16_t    ourRespVl;  /* Delay_Resp us  -> VMC  */
} PtpLeg;

#define PTP_MAX_LEGS 8

typedef struct {
    const char   *unitName;        /* "DPM" / "DSM" - shown in every PTP message */
    bool          runGrandmaster;  /* phases to run (DSM: all three, DPM: slave only) */
    bool          runMaster;
    bool          runSlave;
    const char   *ifname;          /* server interface (raw socket TX + RX) */
    const PtpLeg *legs;
    int           legCount;

    unsigned grandmasterS;         /* phase 1 duration (from the first VMC Sync) */
    unsigned masterS;              /* phase 2 duration (from our first Sync)     */
    unsigned slaveS;               /* phase 3 duration (from our first Sync)     */
    unsigned gapS;                 /* wait between two phases                    */
    unsigned firstSyncWaitS;       /* phase 1: give up if no VMC Sync by then    */
    unsigned syncIntervalMs;       /* period of our Sync                         */
    unsigned respTimeoutMs;        /* Delay_Req -> Delay_Resp timeout            */
    unsigned dashboardS;           /* status table period, 0 = off               */
    bool     kickSync;             /* phase 1: sync the VMC first (one Sync per leg,
                                      answer its Req), so it starts its own Sync */
    bool     trace;                /* print every PTP message sent / received    */
} PtpConfig;

/* Run the three phases. true if every phase PASSed. */
bool casePtp(const PtpConfig *cfg);

#endif /* VMC_TEST_PTP_H_ */

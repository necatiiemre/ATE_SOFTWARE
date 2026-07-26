// test_starter (VMC) — health-monitor handshake gate before VMC tests run.
//
// Unlike the CMC starter (which passively waits for a single "start-test"
// trigger packet), the VMC starter is an ACTIVE participant in the VMC
// health-monitor (HM) protocol. It runs on the ATE server, on a still
// kernel-owned NIC (default: ens1f0np0), using a raw AF_PACKET socket for
// both RX and TX. The sequence is:
//
//   1. COLLECT_CBIT : listen for CBIT reports from the DUT. Count packets
//                     per (side × report-type). Both sides (FLCS + VS) and
//                     all four CBIT report types must each reach
//                     CBIT_REQUIRED_PER_TYPE.
//   2. SEND_REQUEST : once the CBIT quota is met, transmit a PBIT result
//                     request to both sides (FLCS + VS).
//   3. WAIT_PBIT    : wait for the matching PBIT responses from both sides
//                     (re-sending the request periodically until they
//                     arrive or the deadline passes).
//   4. CHECK        : validate the collected CBIT data in checks_pass().
//                     #1: FLCS and VS switch firmware versions (A664_SW_FW_VER)
//                     must match. #2: every switch port (A664_SW_PORT_i_STATUS)
//                     must be linked (== 0) on both sides. #3: FLCS and VS
//                     DTN_ES firmware versions (A664_ES_FW_VER) must match.
//                     These are HARD checks (failure => TIMEOUT, shut down).
//                     A separate SOFT check then scans the BM FLAG CBIT report
//                     for faults (any nonzero fault word or event bitmap, both
//                     sides). Faults do not fail the gate here; they are
//                     surfaced so the operator can decide whether to continue.
//
// Wire encoding (matches dpdk_vmc, see TxRxManager.c / health_monitor.h):
//   Ethernet (dst MAC bytes [4..5] = VL-IDX) + 802.1Q VLAN + IPv4 + UDP
//   - dst MAC : 03:00:00:00:<VL-IDX hi>:<VL-IDX lo>
//   - src MAC : the listening interface's own MAC
//   - IPv4    : src 10.0.0.0, dst 224.224.<VL hi>.<VL lo>, TTL 1, proto UDP
//   - UDP     : sport 100, dport 100
//   - HM VL-IDs (health_monitor.h):
//        FLCS CBIT 0x0b / VS CBIT 0x0e   (CBIT report, payload[0] = msg-id)
//        FLCS PBIT-req 0x0c / VS PBIT-req 0x0f
//        FLCS PBIT-resp 0x0a / VS PBIT-resp 0x0d (payload[0] = 100)
//   - CBIT report msg-ids (payload[0]): DTN_ES=2, DTN_SW=3,
//        BM_ENGINEERING=5, BM_FLAG=6
//
// Exit contract for the MainSoftware orchestrator (same as the CMC starter):
//   stdout "TEST_STARTER_RESULT=OK"           handshake done + all checks passed
//   stdout "TEST_STARTER_RESULT=BM_FLAG_FAULT" hard checks passed, but the BM
//        flag reports one or more faults. A companion line
//        "TEST_STARTER_BM_FLAG=<summary>" lists them. MainSoftware asks the
//        operator whether to continue (faults are usually non-blocking).
//   stdout "TEST_STARTER_RESULT=TIMEOUT" handshake not completed / hard check failed
//   stdout "TEST_STARTER_RESULT=ERROR"   socket / interface failure
// Process always exits 0 so the SSH wrapper can read the result line.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <poll.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

// ---------------------------------------------------------------------------
// Configuration — edit these to match the deployment / firmware spec.
// ---------------------------------------------------------------------------
#define DEFAULT_IFACE        "ens1f0np0"
#define DEFAULT_TIMEOUT_S    600

// HM VL-ID range (health_monitor.h: HM_VL_ID_MIN..HM_VL_ID_MAX)
#define HM_VLID_MIN          0x0009
#define HM_VLID_MAX          0x0010

// HM VL-IDs (health_monitor.h)
#define VLID_FLCS_CBIT       0x000b
#define VLID_VS_CBIT         0x000e
#define VLID_FLCS_PBIT_REQ   0x000c
#define VLID_VS_PBIT_REQ     0x000f
#define VLID_FLCS_PBIT_RESP  0x000a
#define VLID_VS_PBIT_RESP    0x000d

// CBIT report message identifiers (payload[0])
#define MSG_ID_DTN_ES        2
#define MSG_ID_DTN_SW        3
#define MSG_ID_BM_ENG        5
#define MSG_ID_BM_FLAG       6

// PBIT response message identifier (payload[0])
#define PBIT_RESPONSE_MSG_ID 100

// On success the raw (big-endian) PBIT response payloads are written here so
// dpdk_vmc can preload them into its health-monitor dashboard (Seçenek A).
#define PBIT_SNAPSHOT_FLCS "/tmp/vmc_pbit_flcs.bin"
#define PBIT_SNAPSHOT_VS   "/tmp/vmc_pbit_vs.bin"

// How many of EACH CBIT report type (per side) must be seen before we move on.
#define CBIT_REQUIRED_PER_TYPE 3

// Wire sizes (vmc_message_types.h)
#define VMP_CMSW_HEADER_LEN  11      // message_identifier(1)+message_len(2)+timestamp(8)
#define CBIT_MIN_LEN         13      // header(11)+lru_id(1)+comm_status(1)
#define PBIT_DATA_LEN        454     // sizeof(vmc_pbit_data_t)

// PBIT request payload: 11 bytes, same layout/content as the reference cmsw
// payload header — message_identifier(1)=50, message_len(2)=11, timestamp(8).
// timestamp is left zero. The sequence number is the LAST byte (offset 10,
// overwriting timestamp's last byte), using the reference dtn_seq pattern.
#define PBIT_REQ_PAYLOAD_LEN 11

// Ethernet+VLAN+IP+UDP header bytes, and the frame padded up to the 64-byte
// Ethernet minimum (matches the reference PACKET_SIZE clamp).
#define PBIT_HDRS_LEN  (14 + 4 + 20 + 8)
#define PBIT_FRAME_RAW (PBIT_HDRS_LEN + PBIT_REQ_PAYLOAD_LEN)
#define PBIT_FRAME_LEN (PBIT_FRAME_RAW < 64 ? 64 : PBIT_FRAME_RAW)

// Byte offsets into the DTN_SW CBIT report (dtn_sw_cbit_report_t). The report
// has a 15-byte preamble (header 11 + lru_id + side_type + network_type +
// comm_status), then dtn_sw_status_mon_t (57 B), then port[8] (124 B each).
#define DTN_SW_CBIT_LEN          1064  // sizeof(dtn_sw_cbit_report_t)
#define DTN_SW_CBIT_OFF_FW_VER   46    // A664_SW_FW_VER  (uint64, 8 bytes)
#define SW_FW_VER_LEN            8
#define DTN_SW_CBIT_OFF_PORT_COUNT 41  // A664_SW_PORT_COUNT (uint8)
#define DTN_SW_CBIT_OFF_TX_TOTAL   15  // A664_SW_TX_TOTAL_COUNT (uint64, BE)
#define DTN_SW_CBIT_OFF_RX_TOTAL   23  // A664_SW_RX_TOTAL_COUNT (uint64, BE)
#define DTN_SW_CBIT_OFF_PORTS    72    // start of port[8]
#define DTN_SW_PORT_STRIDE       124   // sizeof(dtn_sw_port_mon_t)
#define DTN_SW_PORT_OFF_ID       0     // A664_SW_PORT_ID       (uint16, BE)
#define DTN_SW_PORT_OFF_STATUS   3     // A664_SW_PORT_i_STATUS (uint8; 0 = link OK)
#define DTN_SW_PORT_MAX          8

// DTN_ES CBIT report (dtn_es_cbit_report_t). A664_ES_FW_VER is the first field
// of the dtn_es_monitoring block (absolute offset 15). Its meaningful version
// triplet is major/minor/bugfix at byte offsets 5/6/7 inside that 8-byte field
// (absolute 20/21/22); the leading 5 bytes are reserved.
#define DTN_ES_CBIT_OFF_FW_VER   15    // A664_ES_FW_VER (8 bytes)
#define DTN_ES_FW_VER_LEN        8
#define DTN_ES_VER_MAJOR_IDX     5     // index of major within the 8-byte FW_VER
#define DTN_ES_VER_MINOR_IDX     6
#define DTN_ES_VER_BUGFIX_IDX    7
#define DTN_ES_VER_MIN_LEN       23    // payload must reach offset 22 (bugfix) inclusive

// BM FLAG CBIT report (bm_flag_cbit_report_t == 105 bytes). Field-test
// convention (matches dpdk_vmc): a healthy unit reports every fault/event word
// as 0x0000, so any nonzero FAULT-polarity word — or any nonzero red/orange/
// yellow event bitmap — is a fault. psm_oring_ch (offset 33) is informational
// (BM_POL_VALUE in dpdk_vmc) and is intentionally excluded.
#define BM_FLAG_LEN              105
#define BM_FLAG_OFF_POWER_STATUS 13
#define BM_FLAG_OFF_VCPU_PG      15
#define BM_FLAG_OFF_FCPU_PG      17
#define BM_FLAG_OFF_MMP_ES_PG    19
#define BM_FLAG_OFF_VSW_B_PG     21
#define BM_FLAG_OFF_VSW_A_PG     23
#define BM_FLAG_OFF_ICS1         25
#define BM_FLAG_OFF_ICS2         27
#define BM_FLAG_OFF_PSM_PRI_FLT  29
#define BM_FLAG_OFF_PSM_SEC_FLT  31
#define BM_FLAG_OFF_PSM_HOLDUP   35
#define BM_FLAG_OFF_EVENT_RED    37    // 10 x u16 = 20 bytes (critical)
#define BM_FLAG_LEN_EVENT_RED    20
#define BM_FLAG_OFF_EVENT_ORANGE 57    // 12 x u16 = 24 bytes
#define BM_FLAG_LEN_EVENT_ORANGE 24
#define BM_FLAG_OFF_EVENT_YELLOW 81    // 12 x u16 = 24 bytes
#define BM_FLAG_LEN_EVENT_YELLOW 24

// PBIT request packet parameters.
//   REQUEST_VLAN          : VLAN the server→DUT request is tagged with.
//   PBIT_REQUEST_MSG_ID   : message_identifier byte placed in the 11-byte
//                           request header (pbit_result_req_mes_t).
// PBIT request VLANs — per the VMC spec the two sides go out on different
// VLANs: VS on 97, FLCS on 99. (Not derivable from the dpdk_vmc source, whose
// PBIT-request path is never exercised.) The message identifier (50) is per
// the VMC spec.
#define REQUEST_VLAN_VS      97
#define REQUEST_VLAN_FLCS    99
#define PBIT_REQUEST_MSG_ID  50

// How often to re-send the PBIT request while waiting for responses.
#define PBIT_RESEND_INTERVAL_S 2

// How often to print the periodic status summary (what arrived / what's still
// missing / where we are).
#define STATUS_INTERVAL_S 5

// CBIT side/type bookkeeping ------------------------------------------------
enum { SIDE_FLCS = 0, SIDE_VS = 1, SIDE_COUNT = 2 };
enum { TYPE_DTN_ES = 0, TYPE_DTN_SW = 1, TYPE_BM_ENG = 2, TYPE_BM_FLAG = 3,
       TYPE_COUNT = 4 };

static int cbit_type_index(uint8_t msg_id)
{
    switch (msg_id) {
    case MSG_ID_DTN_ES:  return TYPE_DTN_ES;
    case MSG_ID_DTN_SW:  return TYPE_DTN_SW;
    case MSG_ID_BM_ENG:  return TYPE_BM_ENG;
    case MSG_ID_BM_FLAG: return TYPE_BM_FLAG;
    default:             return -1;
    }
}

static const char *side_name(int side) { return side == SIDE_VS ? "VS" : "FLCS"; }
static const char *type_name(int t)
{
    switch (t) {
    case TYPE_DTN_ES:  return "DTN_ES";
    case TYPE_DTN_SW:  return "DTN_SW";
    case TYPE_BM_ENG:  return "BM_ENG";
    case TYPE_BM_FLAG: return "BM_FLAG";
    default:           return "?";
    }
}

struct vlan_hdr_be {
    uint16_t tci;
    uint16_t inner_proto;
} __attribute__((packed));

// pbit_result_req_mes_t == vmp_cmsw_header_t (11 bytes), all multi-byte BE.
struct vmp_cmsw_header_be {
    uint8_t  message_identifier;
    uint16_t message_len;   // big-endian on wire
    uint64_t timestamp;     // big-endian on wire
} __attribute__((packed));

// ---------------------------------------------------------------------------
// Socket setup
// ---------------------------------------------------------------------------
static int open_listener(const char *ifname, int *ifindex_out, uint8_t src_mac[6])
{
    int s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (s < 0) {
        perror("socket(AF_PACKET)");
        return -1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        close(s);
        return -1;
    }
    int ifindex = ifr.ifr_ifindex;

    // Source MAC for the request. dpdk_vmc's normal TX uses the locally-
    // administered pattern 02:00:00:00:00:20 (TxRxManager.c src_mac), so match
    // it (rather than the NIC's burned-in MAC) so the PBIT request looks
    // exactly like the normal data-plane packets on the wire.
    const uint8_t fixed_src[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x20};
    memcpy(src_mac, fixed_src, 6);

    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family   = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex  = ifindex;
    if (bind(s, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
        perror("bind");
        close(s);
        return -1;
    }

    struct packet_mreq mreq;
    memset(&mreq, 0, sizeof(mreq));
    mreq.mr_ifindex = ifindex;
    mreq.mr_type    = PACKET_MR_PROMISC;
    if (setsockopt(s, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
                   &mreq, sizeof(mreq)) < 0) {
        perror("PACKET_ADD_MEMBERSHIP (non-fatal)");
    }

    int aux_on = 1;
    if (setsockopt(s, SOL_PACKET, PACKET_AUXDATA,
                   &aux_on, sizeof(aux_on)) < 0) {
        perror("PACKET_AUXDATA (non-fatal)");
    }

    *ifindex_out = ifindex;
    return s;
}

// ---------------------------------------------------------------------------
// RX parsing — pull VL-IDX / VLAN / UDP payload out of a captured frame.
// Returns true for a well-formed IPv4/UDP frame addressed by VL-IDX (dst MAC
// bytes [4..5]); the caller decides whether the VL-IDX is interesting.
// ---------------------------------------------------------------------------
static bool parse_frame(const uint8_t *frame, size_t len,
                        uint16_t aux_vlan, bool aux_vlan_valid,
                        uint16_t *vl_id_out, uint16_t *vlan_out,
                        const uint8_t **payload_out, size_t *payload_len_out)
{
    if (len < sizeof(struct ether_header)) return false;
    const struct ether_header *eh = (const struct ether_header *)frame;

    uint16_t vl_id = ((uint16_t)eh->ether_dhost[4] << 8) | eh->ether_dhost[5];

    uint16_t vlan_id;
    const uint8_t *l3;
    size_t        l3_len;

    uint16_t ether_type = ntohs(eh->ether_type);
    if (ether_type == ETH_P_8021Q) {
        if (len < sizeof(struct ether_header) + sizeof(struct vlan_hdr_be)) return false;
        const struct vlan_hdr_be *vh = (const struct vlan_hdr_be *)
            (frame + sizeof(struct ether_header));
        vlan_id = ntohs(vh->tci) & 0x0FFF;
        if (ntohs(vh->inner_proto) != ETH_P_IP) return false;
        l3     = (const uint8_t *)vh + sizeof(struct vlan_hdr_be);
        l3_len = len - sizeof(struct ether_header) - sizeof(struct vlan_hdr_be);
    } else if (ether_type == ETH_P_IP) {
        // Untagged on the wire, or the VLAN was hardware-stripped into aux data.
        vlan_id = aux_vlan_valid ? (aux_vlan & 0x0FFF) : 0;
        l3     = frame + sizeof(struct ether_header);
        l3_len = len - sizeof(struct ether_header);
    } else {
        return false;
    }

    if (l3_len < 20 + 8) return false;
    const uint8_t *ip  = l3;
    int            ihl = (ip[0] & 0x0F) * 4;
    if (ihl < 20 || (size_t)ihl + 8 > l3_len) return false;
    if (ip[9] != IPPROTO_UDP) return false;

    *vl_id_out       = vl_id;
    *vlan_out        = vlan_id;
    *payload_out     = ip + ihl + 8;
    *payload_len_out = l3_len - (size_t)ihl - 8;
    return true;
}

// ---------------------------------------------------------------------------
// TX — build and send a PBIT result request for one side.
// ---------------------------------------------------------------------------
// Reference DTN sequence pattern: 0, then 1..255 cycling.
static uint8_t calc_dtn_seq(uint64_t seq)
{
    if (seq == 0) return 0;
    return (uint8_t)(((seq - 1) % 255) + 1);
}

static int send_pbit_request(int s, int ifindex, const uint8_t src_mac[6],
                             uint16_t vlan_id, uint16_t vl_id, uint8_t msg_id,
                             uint64_t seq)
{
    uint8_t buf[PBIT_FRAME_LEN];
    memset(buf, 0, sizeof(buf));

    struct ether_header *eh = (struct ether_header *)buf;
    eh->ether_dhost[0] = 0x03;
    eh->ether_dhost[1] = 0x00;
    eh->ether_dhost[2] = 0x00;
    eh->ether_dhost[3] = 0x00;
    eh->ether_dhost[4] = (uint8_t)(vl_id >> 8);
    eh->ether_dhost[5] = (uint8_t)(vl_id & 0xFF);
    memcpy(eh->ether_shost, src_mac, 6);
    eh->ether_type = htons(ETH_P_8021Q);

    struct vlan_hdr_be *vh = (struct vlan_hdr_be *)(buf + sizeof(struct ether_header));
    vh->tci         = htons(vlan_id & 0x0FFF);
    vh->inner_proto = htons(ETH_P_IP);

    uint8_t *ip = buf + sizeof(struct ether_header) + sizeof(struct vlan_hdr_be);
    const uint16_t udp_len = 8 + PBIT_REQ_PAYLOAD_LEN;
    const uint16_t ip_len  = 20 + udp_len;
    ip[0] = 0x45;                       // version 4, IHL 5
    ip[1] = 0x00;                       // TOS
    ip[2] = (uint8_t)(ip_len >> 8);     // total length
    ip[3] = (uint8_t)(ip_len & 0xFF);
    ip[4] = 0; ip[5] = 0;               // id
    ip[6] = 0; ip[7] = 0;               // flags/frag
    ip[8] = 1;                          // TTL
    ip[9] = IPPROTO_UDP;                // protocol
    ip[10] = 0; ip[11] = 0;             // checksum (filled below)
    ip[12] = 10; ip[13] = 0; ip[14] = 0; ip[15] = 0;            // src 10.0.0.0
    ip[16] = 224; ip[17] = 224;                                 // dst 224.224.x.y
    ip[18] = (uint8_t)(vl_id >> 8);
    ip[19] = (uint8_t)(vl_id & 0xFF);
    // IPv4 header checksum
    {
        uint32_t sum = 0;
        for (int i = 0; i < 20; i += 2)
            sum += ((uint32_t)ip[i] << 8) | ip[i + 1];
        while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
        uint16_t cksum = (uint16_t)~sum;
        ip[10] = (uint8_t)(cksum >> 8);
        ip[11] = (uint8_t)(cksum & 0xFF);
    }

    uint8_t *udp = ip + 20;
    udp[0] = 0; udp[1] = 100;           // src port 100
    udp[2] = 0; udp[3] = 100;           // dst port 100
    udp[4] = (uint8_t)(udp_len >> 8);   // length
    udp[5] = (uint8_t)(udp_len & 0xFF);
    udp[6] = 0; udp[7] = 0;             // checksum (optional for IPv4/UDP)

    struct vmp_cmsw_header_be *hdr = (struct vmp_cmsw_header_be *)(udp + 8);
    hdr->message_identifier = msg_id;                       // 50
    hdr->message_len        = htons(VMP_CMSW_HEADER_LEN);   // wire 00 0b = 11
    hdr->timestamp          = 0;                            // timestamp left zero
    // Sequence number is the LAST payload byte (offset 10, overwriting the
    // last timestamp byte), using the reference dtn_seq pattern 0,1,..,255,1,..
    uint8_t dtn = calc_dtn_seq(seq);
    *((uint8_t *)hdr + PBIT_REQ_PAYLOAD_LEN - 1) = dtn;

    struct sockaddr_ll dst;
    memset(&dst, 0, sizeof(dst));
    dst.sll_family   = AF_PACKET;
    dst.sll_protocol = htons(ETH_P_8021Q);
    dst.sll_ifindex  = ifindex;
    dst.sll_halen    = 6;
    memcpy(dst.sll_addr, eh->ether_dhost, 6);

    // Full dump of the frame we are about to send, so a wrong request is
    // immediately visible.
    printf("test_starter(VMC): >>> PBIT request TX  VL-IDX=0x%02x/%u  VLAN=%u  "
           "dtn_seq=%u  msg_id=%u  msg_len=%u  frame_len=%zu\n",
           vl_id, vl_id, vlan_id, dtn, msg_id, VMP_CMSW_HEADER_LEN, sizeof(buf));
    printf("test_starter(VMC):     dstMAC=%02x:%02x:%02x:%02x:%02x:%02x  "
           "srcMAC=%02x:%02x:%02x:%02x:%02x:%02x  ethertype=8100\n",
           eh->ether_dhost[0], eh->ether_dhost[1], eh->ether_dhost[2],
           eh->ether_dhost[3], eh->ether_dhost[4], eh->ether_dhost[5],
           src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4], src_mac[5]);
    printf("test_starter(VMC):     IP 10.0.0.0 -> 224.224.%u.%u  TTL=1  UDP 100->100\n",
           (unsigned)(vl_id >> 8), (unsigned)(vl_id & 0xFF));
    printf("test_starter(VMC):     payload(%dB): msg_id=%u msg_len=%u timestamp=0 "
           "dtn_seq=%u (last byte); frame padded to %d\n",
           PBIT_REQ_PAYLOAD_LEN, msg_id, VMP_CMSW_HEADER_LEN, dtn, PBIT_FRAME_LEN);
    printf("test_starter(VMC):     frame hex:");
    for (size_t i = 0; i < sizeof(buf); i++) printf(" %02x", buf[i]);
    printf("\n");
    fflush(stdout);

    ssize_t n = sendto(s, buf, sizeof(buf), 0,
                       (struct sockaddr *)&dst, sizeof(dst));
    if (n < 0) {
        perror("sendto(pbit_request)");
        return -1;
    }
    return 0;
}

static uint64_t rd_be64(const uint8_t *p)
{
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v = (v << 8) | p[i];
    return v;
}

// A DTN_SW CBIT report is "empty" (init/heartbeat) when both switch traffic
// totals are zero. dpdk_vmc skips these; we must too, otherwise the snapshot
// used by the checks can be an all-zero packet (FW/port_count = 0).
static bool dtn_sw_cbit_empty(const uint8_t *payload)
{
    return rd_be64(payload + DTN_SW_CBIT_OFF_TX_TOTAL) == 0 &&
           rd_be64(payload + DTN_SW_CBIT_OFF_RX_TOTAL) == 0;
}

// Collected HM data needed by the acceptance checks. Populated as packets
// arrive; consumed by checks_pass() once the PBIT handshake completes.
typedef struct {
    uint8_t flcs_dtn_sw[DTN_SW_CBIT_LEN]; // raw (big-endian) FLCS DTN_SW CBIT
    uint8_t vs_dtn_sw[DTN_SW_CBIT_LEN];   // raw (big-endian) VS   DTN_SW CBIT
    bool    got_flcs_dtn_sw;
    bool    got_vs_dtn_sw;
    uint8_t flcs_pbit[PBIT_DATA_LEN];     // raw (big-endian) FLCS PBIT response
    uint8_t vs_pbit[PBIT_DATA_LEN];       // raw (big-endian) VS   PBIT response
    uint8_t flcs_dtn_es_ver[DTN_ES_FW_VER_LEN]; // FLCS A664_ES_FW_VER (8 bytes)
    uint8_t vs_dtn_es_ver[DTN_ES_FW_VER_LEN];   // VS   A664_ES_FW_VER (8 bytes)
    bool    got_flcs_dtn_es;
    bool    got_vs_dtn_es;
    uint8_t flcs_bm_flag[BM_FLAG_LEN];    // raw FLCS BM FLAG CBIT report
    uint8_t vs_bm_flag[BM_FLAG_LEN];      // raw VS   BM FLAG CBIT report
    bool    got_flcs_bm_flag;
    bool    got_vs_bm_flag;
} hm_data_t;

// All ports of one switch must report A664_SW_PORT_i_STATUS == 0 (link up).
// Iterates A664_SW_PORT_COUNT ports (clamped to the 8-slot array).
static bool switch_ports_linked(const uint8_t *dtn_sw, const char *side_label)
{
    uint8_t port_count = dtn_sw[DTN_SW_CBIT_OFF_PORT_COUNT];
    if (port_count == 0 || port_count > DTN_SW_PORT_MAX) {
        printf("test_starter(VMC): %s switch reports invalid port_count=%u\n",
               side_label, port_count);
        fflush(stdout);
        return false;
    }

    bool all_up = true;
    for (int i = 0; i < port_count; i++) {
        const uint8_t *p = dtn_sw + DTN_SW_CBIT_OFF_PORTS + (size_t)i * DTN_SW_PORT_STRIDE;
        uint16_t port_id = ((uint16_t)p[DTN_SW_PORT_OFF_ID] << 8) | p[DTN_SW_PORT_OFF_ID + 1];
        uint8_t  status  = p[DTN_SW_PORT_OFF_STATUS];
        printf("test_starter(VMC): %s port %u status=%u (%s)\n",
               side_label, port_id, status, status == 0 ? "LINK OK" : "NO LINK");
        if (status != 0) all_up = false;
    }
    fflush(stdout);
    return all_up;
}

// Read a big-endian uint16 word from a BM FLAG report at byte offset `off`.
// (Byte order is irrelevant for the nonzero fault test, but a consistent BE
// read gives a stable hex value to print.)
static uint16_t bm_word(const uint8_t *bm, size_t off)
{
    return (uint16_t)((bm[off] << 8) | bm[off + 1]);
}

// Scan one side's BM FLAG report for faults. Per the field-test convention a
// healthy unit reports 0x0000 for every fault/event word, so any nonzero
// FAULT-polarity word — or any nonzero red/orange/yellow event bitmap — is a
// fault. Each fault is logged, and a compact summary is appended to `out`
// (best-effort; *off tracks the write cursor). Returns the fault count.
static int bm_flag_scan(const uint8_t *bm, const char *side,
                        char *out, size_t outsz, size_t *off)
{
    static const struct { uint16_t offset; const char *name; } words[] = {
        { BM_FLAG_OFF_POWER_STATUS, "bm_power_status" },
        { BM_FLAG_OFF_VCPU_PG,      "vscpu_power_goods" },
        { BM_FLAG_OFF_FCPU_PG,      "fccpu_power_goods" },
        { BM_FLAG_OFF_MMP_ES_PG,    "mmp_dtn_es_fpga_power_goods" },
        { BM_FLAG_OFF_VSW_B_PG,     "vmp_dtn_sw_b_fpga_power_goods" },
        { BM_FLAG_OFF_VSW_A_PG,     "vmp_dtn_sw_a_fpga_power_goods" },
        { BM_FLAG_OFF_ICS1,         "ics_status_1" },
        { BM_FLAG_OFF_ICS2,         "ics_status_2" },
        { BM_FLAG_OFF_PSM_PRI_FLT,  "psm_pwr_primary_fault" },
        { BM_FLAG_OFF_PSM_SEC_FLT,  "psm_pwr_secondary_fault" },
        { BM_FLAG_OFF_PSM_HOLDUP,   "psm_hold_up_not_ok" },
    };
    static const struct { uint16_t offset; uint16_t len; const char *name; } events[] = {
        { BM_FLAG_OFF_EVENT_RED,    BM_FLAG_LEN_EVENT_RED,    "event_red" },
        { BM_FLAG_OFF_EVENT_ORANGE, BM_FLAG_LEN_EVENT_ORANGE, "event_orange" },
        { BM_FLAG_OFF_EVENT_YELLOW, BM_FLAG_LEN_EVENT_YELLOW, "event_yellow" },
    };

    int faults = 0;
    for (size_t i = 0; i < sizeof(words) / sizeof(words[0]); i++) {
        uint16_t v = bm_word(bm, words[i].offset);
        if (v != 0) {
            faults++;
            printf("test_starter(VMC):   BM_FLAG FAULT %s/%s = 0x%04X\n",
                   side, words[i].name, v);
            if (*off < outsz)
                *off += snprintf(out + *off, outsz - *off, "%s%s/%s=0x%04X",
                                 *off ? ", " : "", side, words[i].name, v);
        }
    }
    for (size_t i = 0; i < sizeof(events) / sizeof(events[0]); i++) {
        const uint8_t *r = bm + events[i].offset;
        bool any = false;
        for (uint16_t b = 0; b < events[i].len; b++)
            if (r[b]) { any = true; break; }
        if (any) {
            faults++;
            printf("test_starter(VMC):   BM_FLAG EVENT %s/%s set\n", side, events[i].name);
            if (*off < outsz)
                *off += snprintf(out + *off, outsz - *off, "%s%s/%s",
                                 *off ? ", " : "", side, events[i].name);
        }
    }
    fflush(stdout);
    return faults;
}

// ---------------------------------------------------------------------------
// Acceptance checks over the collected HM data (DTN_SW CBIT reports).
//
// Check #1: switch firmware version (A664_SW_FW_VER) reported by FLCS must
//           match the one reported by VS.
// Check #2: every switch port (A664_SW_PORT_i_STATUS) must be linked (== 0),
//           on both the FLCS and VS sides.
// Further checks will be appended here as they are specified.
// ---------------------------------------------------------------------------
static bool checks_pass(const hm_data_t *d)
{
    if (!d->got_flcs_dtn_sw || !d->got_vs_dtn_sw) {
        printf("test_starter(VMC): CHECK FAILED — missing DTN_SW CBIT "
               "(FLCS=%d VS=%d)\n", d->got_flcs_dtn_sw, d->got_vs_dtn_sw);
        fflush(stdout);
        return false;
    }

    const uint8_t *f_fw = d->flcs_dtn_sw + DTN_SW_CBIT_OFF_FW_VER;
    const uint8_t *v_fw = d->vs_dtn_sw  + DTN_SW_CBIT_OFF_FW_VER;

    // Check #1 — FLCS vs VS switch firmware version.
    printf("test_starter(VMC): switch FW (A664_SW_FW_VER)  "
           "FLCS=%02x%02x%02x%02x%02x%02x%02x%02x  "
           "VS=%02x%02x%02x%02x%02x%02x%02x%02x\n",
           f_fw[0], f_fw[1], f_fw[2], f_fw[3], f_fw[4], f_fw[5], f_fw[6], f_fw[7],
           v_fw[0], v_fw[1], v_fw[2], v_fw[3], v_fw[4], v_fw[5], v_fw[6], v_fw[7]);
    fflush(stdout);
    if (memcmp(f_fw, v_fw, SW_FW_VER_LEN) != 0) {
        printf("test_starter(VMC): CHECK FAILED — FLCS and VS switch firmware "
               "versions differ\n");
        fflush(stdout);
        return false;
    }

    // Check #2 — all switch ports linked, both sides.
    bool flcs_links = switch_ports_linked(d->flcs_dtn_sw, "FLCS");
    bool vs_links   = switch_ports_linked(d->vs_dtn_sw,   "VS");
    if (!flcs_links || !vs_links) {
        printf("test_starter(VMC): CHECK FAILED — not all switch ports linked "
               "(FLCS=%s VS=%s)\n", flcs_links ? "ok" : "fail",
               vs_links ? "ok" : "fail");
        fflush(stdout);
        return false;
    }

    // Check #3 — DTN_ES firmware version (A664_ES_FW_VER) reported by FLCS must
    // match the one reported by VS (same version-consistency rule as #1, for
    // the other CBIT report that carries a firmware version).
    if (!d->got_flcs_dtn_es || !d->got_vs_dtn_es) {
        printf("test_starter(VMC): CHECK FAILED — missing DTN_ES CBIT "
               "(FLCS=%d VS=%d)\n", d->got_flcs_dtn_es, d->got_vs_dtn_es);
        fflush(stdout);
        return false;
    }
    {
        const uint8_t *f = d->flcs_dtn_es_ver;
        const uint8_t *v = d->vs_dtn_es_ver;
        printf("test_starter(VMC): ES FW (A664_ES_FW_VER)  FLCS=%u.%u.%u  VS=%u.%u.%u\n",
               f[DTN_ES_VER_MAJOR_IDX], f[DTN_ES_VER_MINOR_IDX], f[DTN_ES_VER_BUGFIX_IDX],
               v[DTN_ES_VER_MAJOR_IDX], v[DTN_ES_VER_MINOR_IDX], v[DTN_ES_VER_BUGFIX_IDX]);
        fflush(stdout);
        if (f[DTN_ES_VER_MAJOR_IDX]  != v[DTN_ES_VER_MAJOR_IDX] ||
            f[DTN_ES_VER_MINOR_IDX]  != v[DTN_ES_VER_MINOR_IDX] ||
            f[DTN_ES_VER_BUGFIX_IDX] != v[DTN_ES_VER_BUGFIX_IDX]) {
            printf("test_starter(VMC): CHECK FAILED — FLCS and VS DTN_ES firmware "
                   "versions differ\n");
            fflush(stdout);
            return false;
        }
    }

    return true;
}

typedef enum { PHASE_CBIT = 0, PHASE_PBIT = 1 } phase_t;

// Periodic, human-readable snapshot of where the handshake is: which CBIT
// reports have arrived per side/type, what is still missing, and (in the PBIT
// phase) which responses are still outstanding.
static void print_status(phase_t phase,
                         uint32_t cbit_count[SIDE_COUNT][TYPE_COUNT],
                         bool got_flcs_pbit, bool got_vs_pbit,
                         unsigned pbit_sent, int elapsed_s, int timeout_s)
{
    const char *phase_str = (phase == PHASE_CBIT) ? "1/3 COLLECT_CBIT"
                                                  : "2-3 PBIT_HANDSHAKE";
    printf("test_starter(VMC): ===== STATUS  t=%ds/%ds  phase=%s =====\n",
           elapsed_s, timeout_s, phase_str);

    for (int sd = 0; sd < SIDE_COUNT; sd++) {
        printf("test_starter(VMC):   %-4s CBIT:", side_name(sd));
        for (int ty = 0; ty < TYPE_COUNT; ty++) {
            bool ok = cbit_count[sd][ty] >= CBIT_REQUIRED_PER_TYPE;
            printf("  %-7s %u/%u %s", type_name(ty), cbit_count[sd][ty],
                   CBIT_REQUIRED_PER_TYPE, ok ? "[OK]" : "[--]");
        }
        printf("\n");
    }

    if (phase == PHASE_CBIT) {
        char missing[256];
        size_t off = 0;
        missing[0] = '\0';
        for (int sd = 0; sd < SIDE_COUNT; sd++)
            for (int ty = 0; ty < TYPE_COUNT; ty++)
                if (cbit_count[sd][ty] < CBIT_REQUIRED_PER_TYPE && off < sizeof(missing))
                    off += snprintf(missing + off, sizeof(missing) - off, "%s%s/%s",
                                    off ? ", " : "", side_name(sd), type_name(ty));
        printf("test_starter(VMC):   still waiting for CBIT: %s\n",
               missing[0] ? missing : "(none — quota met)");
    } else {
        printf("test_starter(VMC):   PBIT requests sent=%u  responses: FLCS=%s  VS=%s\n",
               pbit_sent,
               got_flcs_pbit ? "received" : "WAITING",
               got_vs_pbit   ? "received" : "WAITING");
    }
    fflush(stdout);
}

// Persist a raw PBIT response payload so dpdk_vmc can preload it into the
// health-monitor dashboard. Best-effort: a write failure only loses the
// dashboard preload, it does not fail the gate.
static void write_pbit_snapshot(const char *path, const uint8_t *pbit)
{
    FILE *f = fopen(path, "wb");
    if (!f) {
        printf("test_starter(VMC): WARN could not write %s: %s\n",
               path, strerror(errno));
        return;
    }
    size_t n = fwrite(pbit, 1, PBIT_DATA_LEN, f);
    fclose(f);
    printf("test_starter(VMC): wrote PBIT snapshot %s (%zu bytes)\n", path, n);
}

int main(int argc, char **argv)
{
    const char *iface     = DEFAULT_IFACE;
    int         timeout_s = DEFAULT_TIMEOUT_S;

    static struct option opts[] = {
        {"interface", required_argument, 0, 'i'},
        {"timeout",   required_argument, 0, 't'},
        {0, 0, 0, 0}
    };
    int c;
    while ((c = getopt_long(argc, argv, "i:t:", opts, NULL)) != -1) {
        switch (c) {
        case 'i': iface = optarg; break;
        case 't': { int v = atoi(optarg); if (v > 0) timeout_s = v; break; }
        default:
            fprintf(stderr, "usage: %s [--interface=<dev>] [--timeout=<sec>]\n", argv[0]);
            return 1;
        }
    }

    printf("test_starter(VMC): listening on %s (timeout %ds)\n", iface, timeout_s);
    printf("test_starter(VMC): [STEP 1/3] collecting CBIT reports — need %d of "
           "each type (DTN_ES/DTN_SW/BM_ENG/BM_FLAG) on both FLCS and VS\n",
           CBIT_REQUIRED_PER_TYPE);
    fflush(stdout);

    int     ifindex = -1;
    uint8_t src_mac[6];
    int s = open_listener(iface, &ifindex, src_mac);
    if (s < 0) {
        printf("TEST_STARTER_RESULT=ERROR\n");
        fflush(stdout);
        return 0;
    }

    uint32_t cbit_count[SIDE_COUNT][TYPE_COUNT] = {{0}};
    hm_data_t hm;
    memset(&hm, 0, sizeof(hm));
    bool     got_flcs_pbit = false;
    bool     got_vs_pbit   = false;
    uint64_t pbit_seq      = 0;   // increments on every request packet sent

    // Diagnostics — so the status line shows whether anything is arriving and
    // on which VL-IDs (histogram over the HM range 9..16).
    uint64_t rx_total  = 0;       // frames returned by recvmsg
    uint64_t rx_parsed = 0;       // frames that passed parse_frame (IPv4/UDP)
    uint64_t rx_hm     = 0;       // parsed frames with VL-ID in 9..16
    uint64_t rx_other  = 0;       // parsed frames with VL-ID outside 9..16
    uint64_t vlid_hist[HM_VLID_MAX - HM_VLID_MIN + 1] = {0};

    phase_t phase       = PHASE_CBIT;
    time_t  start_time  = time(NULL);
    time_t  deadline    = start_time + timeout_s;
    time_t  next_resend = 0;
    time_t  next_status = start_time;  // print a status snapshot right away

    struct pollfd pfd = { .fd = s, .events = POLLIN };

    while (1) {
        time_t now = time(NULL);
        if (now >= deadline) {
            close(s);
            printf("TEST_STARTER_RESULT=TIMEOUT\n");
            fflush(stdout);
            return 0;
        }

        // While waiting for PBIT responses, re-send the request periodically.
        if (phase == PHASE_PBIT && now >= next_resend) {
            if (!got_flcs_pbit)
                send_pbit_request(s, ifindex, src_mac, REQUEST_VLAN_FLCS,
                                  VLID_FLCS_PBIT_REQ, PBIT_REQUEST_MSG_ID, pbit_seq++);
            if (!got_vs_pbit)
                send_pbit_request(s, ifindex, src_mac, REQUEST_VLAN_VS,
                                  VLID_VS_PBIT_REQ, PBIT_REQUEST_MSG_ID, pbit_seq++);
            next_resend = now + PBIT_RESEND_INTERVAL_S;
        }

        // Periodic status snapshot.
        if (now >= next_status) {
            print_status(phase, cbit_count, got_flcs_pbit, got_vs_pbit,
                         pbit_seq, (int)(now - start_time), timeout_s);
            printf("test_starter(VMC):   rx: total=%llu parsed=%llu hm=%llu other=%llu | "
                   "VL[9..16]=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu\n",
                   (unsigned long long)rx_total, (unsigned long long)rx_parsed,
                   (unsigned long long)rx_hm, (unsigned long long)rx_other,
                   (unsigned long long)vlid_hist[0], (unsigned long long)vlid_hist[1],
                   (unsigned long long)vlid_hist[2], (unsigned long long)vlid_hist[3],
                   (unsigned long long)vlid_hist[4], (unsigned long long)vlid_hist[5],
                   (unsigned long long)vlid_hist[6], (unsigned long long)vlid_hist[7]);
            fflush(stdout);
            next_status = now + STATUS_INTERVAL_S;
        }

        int wait_ms = (int)((deadline - now) * 1000);
        {
            int status_ms = (int)((next_status - now) * 1000);
            if (status_ms >= 0 && status_ms < wait_ms) wait_ms = status_ms;
        }
        if (phase == PHASE_PBIT) {
            int resend_ms = (int)((next_resend - now) * 1000);
            if (resend_ms >= 0 && resend_ms < wait_ms) wait_ms = resend_ms;
        }
        if (wait_ms < 0) wait_ms = 0;

        int pr = poll(&pfd, 1, wait_ms);
        if (pr < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            close(s);
            printf("TEST_STARTER_RESULT=ERROR\n");
            fflush(stdout);
            return 0;
        }
        if (pr == 0) continue;

        uint8_t      buf[2048];
        struct iovec iov = { .iov_base = buf, .iov_len = sizeof(buf) };
        char         ctl[CMSG_SPACE(sizeof(struct tpacket_auxdata))];
        struct msghdr mh = {
            .msg_name = NULL, .msg_namelen = 0,
            .msg_iov = &iov, .msg_iovlen = 1,
            .msg_control = ctl, .msg_controllen = sizeof(ctl), .msg_flags = 0,
        };
        ssize_t n = recvmsg(s, &mh, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("recvmsg");
            continue;
        }
        rx_total++;

        bool     aux_valid = false;
        uint16_t aux_vlan  = 0;
        for (struct cmsghdr *cm = CMSG_FIRSTHDR(&mh); cm != NULL;
             cm = CMSG_NXTHDR(&mh, cm)) {
            if (cm->cmsg_level == SOL_PACKET && cm->cmsg_type == PACKET_AUXDATA) {
                struct tpacket_auxdata aux;
                memcpy(&aux, CMSG_DATA(cm), sizeof(aux));
                if (aux.tp_status & TP_STATUS_VLAN_VALID) {
                    aux_valid = true;
                    aux_vlan  = aux.tp_vlan_tci;
                }
            }
        }

        uint16_t       vl_id = 0, vlan = 0;
        const uint8_t *payload = NULL;
        size_t         payload_len = 0;
        if (!parse_frame(buf, (size_t)n, aux_vlan, aux_valid,
                         &vl_id, &vlan, &payload, &payload_len))
            continue;
        rx_parsed++;
        if (vl_id >= HM_VLID_MIN && vl_id <= HM_VLID_MAX) {
            rx_hm++;
            vlid_hist[vl_id - HM_VLID_MIN]++;
        } else {
            rx_other++;
        }

        if (phase == PHASE_CBIT) {
            int side = -1;
            if (vl_id == VLID_FLCS_CBIT) side = SIDE_FLCS;
            else if (vl_id == VLID_VS_CBIT) side = SIDE_VS;
            if (side < 0) continue;
            if (payload_len < CBIT_MIN_LEN) continue;

            int t = cbit_type_index(payload[0]);
            if (t < 0) continue;

            // DTN_SW: ignore short/empty (init/heartbeat) reports entirely —
            // they carry no switch FW/port data, so they must neither count
            // toward the quota nor overwrite the snapshot the checks read.
            if (t == TYPE_DTN_SW &&
                (payload_len < DTN_SW_CBIT_LEN || dtn_sw_cbit_empty(payload)))
                continue;

            if (cbit_count[side][t] < CBIT_REQUIRED_PER_TYPE) {
                cbit_count[side][t]++;
                printf("test_starter(VMC): CBIT %s/%s %u/%u (VLAN %u)\n",
                       side_name(side), type_name(t),
                       cbit_count[side][t], CBIT_REQUIRED_PER_TYPE, vlan);
                fflush(stdout);
            }

            // Capture the latest populated DTN_SW report (now guaranteed
            // non-empty and full length) for the acceptance checks.
            if (t == TYPE_DTN_SW) {
                if (side == SIDE_FLCS) {
                    memcpy(hm.flcs_dtn_sw, payload, DTN_SW_CBIT_LEN);
                    hm.got_flcs_dtn_sw = true;
                } else {
                    memcpy(hm.vs_dtn_sw, payload, DTN_SW_CBIT_LEN);
                    hm.got_vs_dtn_sw = true;
                }
            }

            // Capture the DTN_ES firmware version (for check #3) — only the
            // 8-byte A664_ES_FW_VER field is needed.
            if (t == TYPE_DTN_ES && payload_len >= DTN_ES_VER_MIN_LEN) {
                if (side == SIDE_FLCS) {
                    memcpy(hm.flcs_dtn_es_ver, payload + DTN_ES_CBIT_OFF_FW_VER,
                           DTN_ES_FW_VER_LEN);
                    hm.got_flcs_dtn_es = true;
                } else {
                    memcpy(hm.vs_dtn_es_ver, payload + DTN_ES_CBIT_OFF_FW_VER,
                           DTN_ES_FW_VER_LEN);
                    hm.got_vs_dtn_es = true;
                }
            }

            // Capture the latest full BM FLAG report for the soft fault scan.
            if (t == TYPE_BM_FLAG && payload_len >= BM_FLAG_LEN) {
                if (side == SIDE_FLCS) {
                    memcpy(hm.flcs_bm_flag, payload, BM_FLAG_LEN);
                    hm.got_flcs_bm_flag = true;
                } else {
                    memcpy(hm.vs_bm_flag, payload, BM_FLAG_LEN);
                    hm.got_vs_bm_flag = true;
                }
            }

            bool all_met = true;
            for (int sd = 0; sd < SIDE_COUNT && all_met; sd++)
                for (int ty = 0; ty < TYPE_COUNT; ty++)
                    if (cbit_count[sd][ty] < CBIT_REQUIRED_PER_TYPE) { all_met = false; break; }

            if (all_met) {
                printf("test_starter(VMC): [STEP 2/3] CBIT quota met — sending PBIT "
                       "requests (FLCS VL=0x%02x/%u on VLAN %u, VS VL=0x%02x/%u on "
                       "VLAN %u), waiting for responses\n",
                       VLID_FLCS_PBIT_REQ, VLID_FLCS_PBIT_REQ, REQUEST_VLAN_FLCS,
                       VLID_VS_PBIT_REQ, VLID_VS_PBIT_REQ, REQUEST_VLAN_VS);
                fflush(stdout);
                phase       = PHASE_PBIT;
                next_resend = 0;  // triggers an immediate send on next loop
            }
            continue;
        }

        // PHASE_PBIT — collect both PBIT responses.
        if (vl_id == VLID_FLCS_PBIT_RESP || vl_id == VLID_VS_PBIT_RESP) {
            if (payload_len < PBIT_DATA_LEN) continue;
            if (payload[0] != PBIT_RESPONSE_MSG_ID) continue;

            if (vl_id == VLID_FLCS_PBIT_RESP && !got_flcs_pbit) {
                memcpy(hm.flcs_pbit, payload, PBIT_DATA_LEN);
                got_flcs_pbit = true;
                printf("test_starter(VMC): FLCS PBIT response received\n");
                fflush(stdout);
            } else if (vl_id == VLID_VS_PBIT_RESP && !got_vs_pbit) {
                memcpy(hm.vs_pbit, payload, PBIT_DATA_LEN);
                got_vs_pbit = true;
                printf("test_starter(VMC): VS PBIT response received\n");
                fflush(stdout);
            }

            if (got_flcs_pbit && got_vs_pbit) {
                printf("test_starter(VMC): [STEP 3/3] both PBIT responses received "
                       "— running acceptance checks\n");
                fflush(stdout);
                bool ok = checks_pass(&hm);
                close(s);
                if (!ok) {
                    printf("test_starter(VMC): PBIT data rejected by check\n");
                    printf("TEST_STARTER_RESULT=TIMEOUT\n");
                    fflush(stdout);
                    return 0;
                }

                // Hard checks passed → persist the PBIT responses for
                // dpdk_vmc's dashboard regardless of the BM flag outcome
                // (the operator may still choose to proceed).
                write_pbit_snapshot(PBIT_SNAPSHOT_FLCS, hm.flcs_pbit);
                write_pbit_snapshot(PBIT_SNAPSHOT_VS,   hm.vs_pbit);

                // Soft check: scan BM FLAG for faults on both sides. A healthy
                // unit yields zero faults → OK. Any fault is reported and the
                // decision (continue or abort) is deferred to the operator via
                // MainSoftware, which prompts on TEST_STARTER_RESULT=BM_FLAG_FAULT.
                if (!hm.got_flcs_bm_flag || !hm.got_vs_bm_flag) {
                    printf("test_starter(VMC): WARN missing full BM_FLAG CBIT "
                           "(FLCS=%d VS=%d) — fault scan partial\n",
                           hm.got_flcs_bm_flag, hm.got_vs_bm_flag);
                    fflush(stdout);
                }
                char faults[512];
                size_t foff = 0;
                faults[0] = '\0';
                int nf = 0;
                if (hm.got_flcs_bm_flag)
                    nf += bm_flag_scan(hm.flcs_bm_flag, "FLCS", faults, sizeof(faults), &foff);
                if (hm.got_vs_bm_flag)
                    nf += bm_flag_scan(hm.vs_bm_flag, "VS", faults, sizeof(faults), &foff);

                if (nf > 0) {
                    printf("test_starter(VMC): BM flag reports %d fault(s)\n", nf);
                    // Detail line consumed by MainSoftware for the operator prompt.
                    printf("TEST_STARTER_BM_FLAG=%s\n", faults);
                    printf("TEST_STARTER_RESULT=BM_FLAG_FAULT\n");
                } else {
                    printf("test_starter(VMC): BM flag clean — handshake complete, "
                           "data accepted\n");
                    printf("TEST_STARTER_RESULT=OK\n");
                }
                fflush(stdout);
                return 0;
            }
        }
    }
}

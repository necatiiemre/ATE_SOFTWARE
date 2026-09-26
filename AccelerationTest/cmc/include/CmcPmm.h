/**
 * @file CmcPmm.h
 * @brief The PMM lines: listen only.
 *
 * Nothing is generated on these. The SMMM sends to each PMM and we sit on the
 * receiving side, count what arrives, check its CRC and follow its sequence.
 * There is no transmit path here at all - the reference calls it the SMMM UDP
 * channel and this is that channel with one difference: it tells the lines
 * apart by which interface a frame came in on, where the reference tells them
 * apart by the VLAN tag the switch put on. Two lines here rather than three,
 * because that is what the rig has.
 *
 * Wire format, as it reaches us on a direct cable:
 *
 *   offset  size  field
 *   0..5      6   destination MAC
 *   6..11     6   source MAC
 *   12..13    2   ethertype 0x0800   (0x8100 and a tag first, behind a switch)
 *   14..33   20   IPv4
 *   34..41    8   UDP
 *   42..    ...   MSG
 *
 * MSG is 1036 bytes: an 8-byte sequence, 1024 bytes of data, a 4-byte CRC. The
 * SMMM's own 2-byte header (packet type and encryption state) sometimes precedes
 * it, and which it is can be told from the UDP length - 1038 rather than 1036.
 *
 * Two things about it are not pinned down by any document, so the code deals
 * with both rather than assuming: the sequence is big-endian (read the other way
 * round, a real captured packet's counter came out as 10^16 and the loss column
 * filled with nonsense), and the CRC's algorithm, coverage and byte order are
 * worked out from the first packets that arrive by trying the eight
 * combinations and locking onto the one that matches.
 */

#ifndef CMC_PMM_H
#define CMC_PMM_H

#include "AppConfig.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* MSG - the wire structure, copied from the reference's SmmmUdp.h     */
/* ------------------------------------------------------------------ */

#define SMMM_CRC_32_BYTE   32u     /* CRC-32 (bit width) */
#define SMMM_SEQ_NUM_BYTE  8u
#define SMMM_DATA_BYTE     1024u

#pragma pack(push, 1)
typedef struct {
    uint64_t seq_num;
    uint8_t  data[SMMM_DATA_BYTE];
    uint32_t data_crc;
} smmm_msg_t;
#pragma pack(pop)

#define SMMM_MSG_LEN  (SMMM_SEQ_NUM_BYTE + SMMM_DATA_BYTE + 4u)   /* 1036 */

_Static_assert(sizeof(smmm_msg_t) == SMMM_MSG_LEN,
               "smmm_msg_t must be 1036 bytes - the wire format is broken");

/* The SMMM's 2-byte header. Whether a packet carries it is told from the UDP
 * payload length. */
#define SMMM_UDP_SMMM_HDR_LEN      2
#define SMMM_UDP_PAYLOAD_NO_HDR    SMMM_MSG_LEN                              /* 1036 */
#define SMMM_UDP_PAYLOAD_WITH_HDR  (SMMM_MSG_LEN + SMMM_UDP_SMMM_HDR_LEN)    /* 1038 */

/* Header field values (the wire form of smmm_udp_types.h's enums) */
#define SMMM_PKT_TYPE_MDRS_TO_IPPP  0x83u
#define SMMM_PKT_TYPE_IPPP_TO_MDRS  0x84u
#define SMMM_PKT_TYPE_PMM_TO_IPPP   0x85u
#define SMMM_PKT_TYPE_IPPP_TO_PMM   0x86u   /* SMMM -> PMM: the direction we expect */

#define SMMM_ENC_CIPHERTEXT         0x00u
#define SMMM_ENC_PLAINTEXT          0xFBu

#define SMMM_UDP_ETH_HDR_LEN    14
#define SMMM_UDP_VLAN_HDR_LEN   4
#define SMMM_UDP_IP_HDR_LEN     20
#define SMMM_UDP_L4_HDR_LEN     8

/* The biggest sequence jump that is still counted as loss. Above it the stream
 * has restarted or the counter was reset, and the loss column must not gain a
 * billion packets for it. */
#define SMMM_UDP_MAX_SEQ_GAP 1000000ull

/** Silence, in seconds, after which a line stops counting as live. */
#define SMMM_UDP_STALE_S 3

/* ------------------------------------------------------------------ */
/* Counters                                                           */
/* ------------------------------------------------------------------ */

/**
 * One line's counters. Single writer (that line's listener), read by the
 * dashboard - the reference's own arrangement, and the reason these are plain
 * integers.
 */
typedef struct {
    bool     seq_initialized;
    uint64_t expected_seq;
    uint64_t last_seq;

    uint64_t rx_pkts;
    uint64_t rx_bytes;
    uint64_t crc_ok;
    uint64_t crc_fail;
    uint64_t crc_unknown;      /**< arrived before the variant was worked out */
    uint64_t lost_pkts;
    uint64_t out_of_order_pkts;
    uint64_t duplicate_pkts;
    uint64_t bad_len_pkts;     /**< not MSG-shaped */
    uint64_t resync_events;    /**< a jump too big to be loss */
    uint64_t addr_mismatch;    /**< IP or UDP port not what was expected */
    uint64_t hdr_present_pkts; /**< carried the SMMM 2-byte header */
    uint64_t last_rx_ns;
    bool     first_bad_logged;

    /* For the per-second rate in the table. */
    uint64_t prev_rx_bytes;
    uint64_t prev_print_ns;
} cmc_pmm_stats_t;

typedef struct cmc_pmm cmc_pmm_t;

cmc_pmm_t *cmc_pmm_create(const cmc_config_t *config, volatile bool *stop);
void cmc_pmm_destroy(cmc_pmm_t *pmm);

/** Open a socket on each PMM interface. */
bool cmc_pmm_open(cmc_pmm_t *pmm);

/** One listener thread per line. */
bool cmc_pmm_start(cmc_pmm_t *pmm);

void cmc_pmm_stop(cmc_pmm_t *pmm);
bool cmc_pmm_running(const cmc_pmm_t *pmm);

/** Zero the counters - the warm-up boundary. The CRC variant is kept. */
void cmc_pmm_reset(cmc_pmm_t *pmm);

/** The table, as the reference prints it. Not const: it holds the rate baseline. */
void cmc_pmm_print_table(cmc_pmm_t *pmm);

const cmc_pmm_stats_t *cmc_pmm_stats(const cmc_pmm_t *pmm, uint8_t link);

/** The CRC variant in use, or NULL while it is still being worked out. */
const char *cmc_pmm_crc_variant(const cmc_pmm_t *pmm);

/** One frame, as the listener takes it. Exposed for the tests. */
void cmc_pmm_ingest(cmc_pmm_t *pmm, uint8_t link, const uint8_t *frame, size_t len);

#endif /* CMC_PMM_H */

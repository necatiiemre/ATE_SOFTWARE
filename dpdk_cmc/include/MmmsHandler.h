#ifndef MMMS_HANDLER_H
#define MMMS_HANDLER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * MMMS file-transfer handover (CMC shutdown phase).
 *
 * On Ctrl+C the normal PRBS TX is halted and a single request packet is
 * emitted from port 0:
 *
 *   VLAN 97 / VL-IDX 8110 / payload = [0x05 0x09] "read-smmm" 0x00*89 [seq=0x00]
 *
 * The peer then streams back files via VLAN 225 / VL-IDX 8010:
 *
 *   filename packet (136 B payload):
 *     "start"(5) + name_len(1) + dir_name_len(1) +
 *     log_name(64, 0x00 padded) + dir_name(64, 0x00 padded) + seq(1)
 *
 *     name_len / dir_name_len give the used length of the two fixed-width
 *     name fields. Each file is stored under its own directory:
 *       <output_dir>/<dir_name>/<log_name>
 *
 *   content packet (1467 B payload):
 *     data(1466) + seq(1)
 *
 *   end-of-transfer packet (136 B payload):
 *     "finish-smmm"(11) + 0x00 pad to 135 + seq(1)
 *
 * Control packets are dispatched on payload length exactly as before; the
 * pre-directory 101 B control size is still accepted for "finish-smmm" so a
 * peer that only grew its start packet keeps working.
 *
 * Sequence numbers wrap 1..255 (0 is reserved for the trigger). The handler
 * detects gaps in this single global stream and counts corrupted packets
 * without aborting.
 */

#define MMMS_TRIGGER_VLAN          97
#define MMMS_TRIGGER_VL_ID         8110
#define MMMS_RESPONSE_VLAN         225
#define MMMS_RESPONSE_VL_ID        8010

/* Trigger ("read-smmm") payload we emit — unrelated to the peer's control
 * packet size below, so it keeps its own constant. */
#define MMMS_TRIGGER_PAYLOAD_LEN   101    /* "read-smmm" request + seq */

/* Fixed-width name fields inside the "start" packet. */
#define MMMS_NAME_FIELD_LEN        64     /* log_name[64]  */
#define MMMS_DIR_FIELD_LEN         64     /* dir_name[64]  */

/* "start" field offsets. */
#define MMMS_START_OFF_NAME_LEN    5
#define MMMS_START_OFF_DIR_LEN     6
#define MMMS_START_OFF_LOG_NAME    7
#define MMMS_START_OFF_DIR_NAME    (MMMS_START_OFF_LOG_NAME + MMMS_NAME_FIELD_LEN)  /* 71 */

#define MMMS_CONTROL_PAYLOAD_LEN   136    /* "start" / "finish-smmm" + seq */
#define MMMS_CONTROL_PAYLOAD_LEN_LEGACY 101 /* pre-directory control size */
#define MMMS_CONTENT_PAYLOAD_LEN   1467   /* 1466 data + 1 seq */
#define MMMS_CONTENT_DATA_LEN      1466

#define MMMS_FIRST_PACKET_TIMEOUT_S 60

/*
 * Mirror Common.h's stop_normal_tx symbol here so translation units that
 * pull MmmsHandler.h (notably TxRxManager.c, which does not include
 * Common.h) can reference the flag without an extra include.
 */
extern volatile bool stop_normal_tx;

/* Forward declarations (DPDK types kept opaque for header simplicity). */
struct ports_config;

typedef enum {
    MMMS_IDLE      = 0,  /* handover not active */
    MMMS_ARMED     = 1,  /* trigger sent, waiting for first response */
    MMMS_RECEIVING = 2,  /* at least one response packet seen */
    MMMS_DONE_OK   = 3,  /* finish-smmm received */
    MMMS_DONE_TIMEOUT = 4 /* no response within timeout */
} mmms_state_t;

/**
 * Prepare output directory and reset state. Idempotent.
 *
 * The directory's existing contents are deleted first, so a run never mixes
 * its files with the previous run's. Call this before the handover can start
 * (main does so at startup), never mid-transfer.
 *
 * @param output_dir e.g. "/tmp/mmms_logs". Created if missing.
 * @return 0 on success, -1 on failure.
 */
int mmms_init(const char *output_dir);

/**
 * Send the trigger packet from port 0, queue 0 and arm the timeout.
 * Caller must guarantee TX workers are halted (stop_normal_tx == true).
 * @return 0 on success, -1 on failure.
 */
int mmms_send_trigger(struct ports_config *ports_config);

/**
 * RX hot-path entry. Called from rx_worker when VLAN 225 + VL-IDX 8010 is seen.
 * payload points at the UDP payload start; payload_len is the on-wire length.
 */
void mmms_handle_packet(const uint8_t *payload, uint16_t payload_len);

/**
 * Called once per second from the main loop. Promotes ARMED -> DONE_TIMEOUT
 * if no response packet arrived within MMMS_FIRST_PACKET_TIMEOUT_S.
 */
void mmms_check_timeout(void);

/** True once the handover finished (success or timeout). */
bool mmms_is_done(void);

/** True after mmms_send_trigger() succeeds. */
bool mmms_is_armed(void);

/** Flush any open file, log final counters. Safe to call multiple times. */
void mmms_finalize(void);

#ifdef __cplusplus
}
#endif

#endif /* MMMS_HANDLER_H */

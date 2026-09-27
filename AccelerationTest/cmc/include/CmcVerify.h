/**
 * @file CmcVerify.h
 * @brief What the CMC does to a payload, checked in reverse.
 *
 * The frame that comes back is not the frame that went out. The CMC rewrites
 * the front of the payload and returns it on a VL id 520 higher, and the whole
 * of the test's verdict is whether that rewrite is exactly right:
 *
 *   [seq 8][SplitMix XOR 64][CRC32C 4][XOR byte 1][PRBS ...][DTN_SEQ 1]
 *    0..7   8..71            72..75    76          77..       last
 *
 *   - the SplitMix zone is the original PRBS bytes XOR'd with splitmix64 fed
 *     from the big-endian sequence, eight bytes at a time
 *   - the CRC covers bytes 0..71, big-endian on the wire, and uses the unit's
 *     own CRC table rather than standard CRC-32C (see CmcPayloadVerify.h)
 *   - the XOR byte is the original PRBS byte at that offset run through the
 *     chain {6,7,8,13,15}, which folds to a single XOR with 0x0B
 *   - the rest is the PRBS stream untouched, except the last byte, which
 *     carries DTN_SEQ and so is skipped
 *
 * Nothing has to be remembered between packets: the sequence in the payload
 * says which PRBS offset to regenerate, so every check is against something
 * derived, and a packet that arrives out of order still verifies on its own.
 *
 * This is the reference's rx_worker inner loop, lifted out of the loop so that
 * a test can hand it a payload instead of a wire.
 */

#ifndef CMC_VERIFY_H
#define CMC_VERIFY_H

#include <stdbool.h>
#include <stdint.h>

/** Which of the four checks passed, and how far off the payload was. */
typedef struct {
    bool     crc_ok;
    bool     splitmix_ok;
    bool     xor_ok;
    bool     prbs_ok;
    uint64_t bit_errors;   /**< over the whole payload, counted as the reference counts it */
} cmc_verify_t;

/**
 * @brief Check one returned payload.
 *
 * @param payload  the CMC_PAYLOAD_SIZE bytes after the UDP header
 * @param prbs_exp the PRBS stream for this payload's sequence, at least
 *                 CMC_NUM_PRBS_BYTES long - prbs31_at() gives it
 * @param out      filled in either way; bit_errors is only counted when
 *                 something failed, as the reference only counts it then
 * @return true when all four checks passed
 */
bool cmc_verify_payload(const uint8_t *payload, const uint8_t *prbs_exp,
                        cmc_verify_t *out);

/** The sequence number out of a payload: a raw host-order 64-bit word. */
uint64_t cmc_payload_seq(const uint8_t *payload);

#endif /* CMC_VERIFY_H */

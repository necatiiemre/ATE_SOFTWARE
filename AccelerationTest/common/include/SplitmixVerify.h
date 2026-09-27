/**
 * @file SplitmixVerify.h
 * @brief The transform the units apply to a payload, checked in reverse.
 *
 * Two of the ATE's units rewrite the front of a payload before returning it, and
 * they do it the same way bar one byte. The workstation sends
 *
 *     [seq 8][PRBS ...]
 *
 * and gets back
 *
 *     [seq 8][SplitMix XOR 64][CRC32C 4][XOR byte 0 or 1][PRBS ...][last byte]
 *
 *   - the SplitMix zone is the original PRBS bytes XOR'd with splitmix64 fed
 *     from 8 * the big-endian form of the sequence plus the block index, the
 *     result written big-endian, eight bytes at a time
 *   - the CRC covers the sequence and the SplitMix zone, is big-endian on the
 *     wire, and uses the unit's own CRC table rather than standard CRC-32C
 *   - the XOR byte is the CMC's alone: the original PRBS byte at that offset run
 *     through the chain {6,7,8,13,15}, which folds to a single XOR with 0x0B.
 *     dpdk_vmc has no such byte, so its overhead is 68 where the CMC's is 69
 *   - the rest is the PRBS stream untouched, short of the last payload byte,
 *     which the DTN overwrites with its own sequence on the way through and
 *     which both units therefore leave out of the comparison
 *
 * The sequence is a raw host-order 64-bit word, which is what
 * `*seq_ptr = sequence_number` in the reference writes and what its receiver
 * reads straight back out. It is *not* big-endian on the wire, and reading it
 * either way round changes which PRBS offset is regenerated, so this is the one
 * field where a guess would make every frame read as bad.
 *
 * Nothing has to be remembered between packets: the sequence says which PRBS
 * offset to regenerate, so every check is against something derived, and a
 * packet that arrives out of order still verifies on its own.
 */

#ifndef SPLITMIX_VERIFY_H
#define SPLITMIX_VERIFY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Which of the two shapes a unit returns.
 *
 * prbs_bytes is the payload minus the eight-byte sequence - the reference calls
 * it NUM_PRBS_BYTES - and is what the PRBS zone would be if nothing were written
 * over it.
 */
typedef struct {
    uint16_t prbs_bytes;
    bool     xor_zone;   /**< true for the CMC, false for the VMC */
    bool     skip_last;  /**< leave the trailing byte out; true for both */
} splitmix_layout_t;

/** dpdk_cmc: the XOR'd byte, overhead 69. */
#define SPLITMIX_LAYOUT_CMC(prbs) \
    ((splitmix_layout_t){.prbs_bytes = (prbs), .xor_zone = true, .skip_last = true})

/** dpdk_vmc: no XOR'd byte, overhead 68. */
#define SPLITMIX_LAYOUT_VMC(prbs) \
    ((splitmix_layout_t){.prbs_bytes = (prbs), .xor_zone = false, .skip_last = true})

/** Which of the checks passed, and how far off the payload was. */
typedef struct {
    bool     crc_ok;
    bool     splitmix_ok;
    bool     xor_ok;       /**< always true when the layout has no XOR byte */
    bool     prbs_ok;
    uint64_t bit_errors;   /**< over the whole payload, counted as the reference counts it */
} splitmix_result_t;

/** Bytes the transform writes over the front of the PRBS zone: 68 or 69. */
size_t splitmix_overhead(const splitmix_layout_t *layout);

/** The shortest payload the layout can be checked in. */
size_t splitmix_min_payload(const splitmix_layout_t *layout);

/**
 * @brief Check one returned payload.
 *
 * @param payload  the 8 + layout->prbs_bytes bytes after the UDP header
 * @param prbs_exp the PRBS stream for this payload's sequence, at least
 *                 layout->prbs_bytes long - prbs31_at() gives it
 * @param out      filled in either way; bit_errors is only counted when
 *                 something failed, as the reference only counts it then
 * @return true when every check passed
 */
bool splitmix_verify(const uint8_t *payload, const uint8_t *prbs_exp,
                     const splitmix_layout_t *layout, splitmix_result_t *out);

/** The sequence out of a payload: a raw host-order 64-bit word. */
uint64_t splitmix_payload_seq(const uint8_t *payload);

/**
 * @brief Apply the transform, as the unit does.
 *
 * Not used in a run - the unit does this, not us - but it is what the tests
 * build a returned payload with, and having it beside the verifier is what keeps
 * the two from drifting into agreeing with each other about something wrong.
 */
void splitmix_apply(uint8_t *payload, const uint8_t *prbs_exp,
                    const splitmix_layout_t *layout);

#endif /* SPLITMIX_VERIFY_H */

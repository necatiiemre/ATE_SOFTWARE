#include "SplitmixVerify.h"

#include "PayloadVerify.h"

#include <string.h>

size_t splitmix_overhead(const splitmix_layout_t *layout)
{
    return (size_t)SPLITMIX_XOR_BYTES + SPLITMIX_CRC_BYTES +
           (layout->xor_zone ? XOR_ZONE_BYTES : 0);
}

size_t splitmix_min_payload(const splitmix_layout_t *layout)
{
    return SPLITMIX_SEQ_BYTES + splitmix_overhead(layout);
}

uint64_t splitmix_payload_seq(const uint8_t *payload)
{
    uint64_t seq;

    memcpy(&seq, payload, sizeof seq);
    return seq;
}

/* How much of the PRBS zone is still PRBS: what the transform did not write
 * over, less the trailing byte the DTN overwrites on the way through. */
static size_t prbs_check_len(const splitmix_layout_t *layout)
{
    const size_t overhead = splitmix_overhead(layout);
    size_t len = layout->prbs_bytes - overhead;

    if (layout->skip_last && len)
        len--;
    return len;
}

void splitmix_apply(uint8_t *payload, const uint8_t *prbs_exp,
                    const splitmix_layout_t *layout)
{
    const uint64_t seq = splitmix_payload_seq(payload);
    uint8_t sm[SPLITMIX_XOR_BYTES];

    build_expected_splitmix(sm, seq, prbs_exp);
    memcpy(payload + SPLITMIX_SEQ_BYTES, sm, SPLITMIX_XOR_BYTES);

    const uint32_t crc = sw_crc32c(payload, SPLITMIX_SEQ_BYTES + SPLITMIX_XOR_BYTES);
    const uint32_t be = __builtin_bswap32(crc);

    memcpy(payload + SPLITMIX_SEQ_BYTES + SPLITMIX_XOR_BYTES, &be, sizeof be);

    if (layout->xor_zone)
        payload[SPLITMIX_SEQ_BYTES + SPLITMIX_XOR_BYTES + SPLITMIX_CRC_BYTES] ^=
            XOR_ZONE_MASK;
}

/* The zones, in the order the reference checks them. The arithmetic is the
 * reference's; what is different here is that the payload and its length are
 * parameters rather than compile-time constants, and every multi-byte read goes
 * through memcpy because a payload off the wire is not aligned to anything. */
bool splitmix_verify(const uint8_t *payload, const uint8_t *prbs_exp,
                     const splitmix_layout_t *layout, splitmix_result_t *out)
{
    const size_t xor_off = SPLITMIX_XOR_BYTES + SPLITMIX_CRC_BYTES;   /* 68 */
    const uint64_t seq = splitmix_payload_seq(payload);

    memset(out, 0, sizeof *out);
    out->xor_ok = true;             /* nothing to check when there is no byte */

    /* 1. CRC32C over the sequence and the SplitMix zone, big-endian on the wire. */
    uint32_t calc_crc = sw_crc32c(payload, SPLITMIX_SEQ_BYTES + SPLITMIX_XOR_BYTES);
    uint32_t wire_crc;
    memcpy(&wire_crc, payload + SPLITMIX_SEQ_BYTES + SPLITMIX_XOR_BYTES,
           sizeof wire_crc);
    out->crc_ok = (calc_crc == __builtin_bswap32(wire_crc));

    /* 2. The SplitMix zone, regenerated from the sequence and the PRBS stream. */
    uint8_t expected_sm[SPLITMIX_XOR_BYTES];
    build_expected_splitmix(expected_sm, seq, prbs_exp);
    out->splitmix_ok = memcmp(payload + SPLITMIX_SEQ_BYTES, expected_sm,
                              SPLITMIX_XOR_BYTES) == 0;

    /* 3. The one XOR'd byte, where the layout has one. */
    uint8_t wire_xor = 0, expected_xor = 0;

    if (layout->xor_zone) {
        wire_xor = payload[SPLITMIX_SEQ_BYTES + xor_off];
        expected_xor = (uint8_t)(prbs_exp[xor_off] ^ XOR_ZONE_MASK);
        out->xor_ok = (wire_xor == expected_xor);
    }

    /* 4. The rest of the PRBS stream. */
    const size_t overhead = splitmix_overhead(layout);
    const uint8_t *wire_prbs = payload + SPLITMIX_SEQ_BYTES + overhead;
    const uint8_t *exp_prbs  = prbs_exp + overhead;
    const size_t prbs_len = prbs_check_len(layout);

    out->prbs_ok = memcmp(wire_prbs, exp_prbs, prbs_len) == 0;

    if (out->crc_ok && out->splitmix_ok && out->xor_ok && out->prbs_ok)
        return true;

    /* Bit errors over every zone, the way the reference adds them up: the
     * SplitMix and CRC zones always, the other two only when they failed. */
    uint64_t bits = payload_bit_errors(payload + SPLITMIX_SEQ_BYTES, expected_sm,
                                      SPLITMIX_XOR_BYTES);
    {
        uint8_t expected_crc_be[SPLITMIX_CRC_BYTES];
        uint32_t be = __builtin_bswap32(calc_crc);

        memcpy(expected_crc_be, &be, sizeof be);
        bits += payload_bit_errors(payload + SPLITMIX_SEQ_BYTES + SPLITMIX_XOR_BYTES,
                                   expected_crc_be, SPLITMIX_CRC_BYTES);
    }
    if (!out->xor_ok)
        bits += (uint64_t)__builtin_popcount((unsigned)(wire_xor ^ expected_xor));
    if (!out->prbs_ok)
        bits += payload_bit_errors(wire_prbs, exp_prbs, (uint32_t)prbs_len);

    out->bit_errors = bits;
    return false;
}

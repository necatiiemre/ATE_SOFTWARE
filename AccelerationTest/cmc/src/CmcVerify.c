#include "CmcVerify.h"

#include "CmcPacket.h"
#include "CmcPayloadVerify.h"

#include <string.h>

uint64_t cmc_payload_seq(const uint8_t *payload)
{
    uint64_t seq;

    memcpy(&seq, payload, sizeof seq);
    return seq;
}

/* The four zones, in the order the reference checks them. The arithmetic is
 * the reference's; what is different here is that the payload is a parameter
 * rather than a pointer into an mbuf, and every multi-byte read goes through
 * memcpy because a payload off the wire is not aligned to anything. */
bool cmc_verify_payload(const uint8_t *payload, const uint8_t *prbs_exp,
                        cmc_verify_t *out)
{
    const size_t xor_off = SPLITMIX_XOR_BYTES + SPLITMIX_CRC_BYTES;   /* 68 */
    const uint64_t seq = cmc_payload_seq(payload);

    memset(out, 0, sizeof *out);

    /* 1. CRC32C over the sequence and the SplitMix zone, big-endian on the wire. */
    uint32_t calc_crc = sw_crc32c(payload, CMC_SEQ_BYTES + SPLITMIX_XOR_BYTES);
    uint32_t wire_crc;
    memcpy(&wire_crc, payload + CMC_SEQ_BYTES + SPLITMIX_XOR_BYTES, sizeof wire_crc);
    out->crc_ok = (calc_crc == __builtin_bswap32(wire_crc));

    /* 2. The SplitMix zone, regenerated from the sequence and the PRBS stream. */
    uint8_t expected_sm[SPLITMIX_XOR_BYTES];
    build_expected_splitmix(expected_sm, seq, prbs_exp);
    out->splitmix_ok = memcmp(payload + CMC_SEQ_BYTES, expected_sm,
                              SPLITMIX_XOR_BYTES) == 0;

    /* 3. The one XOR'd byte. */
    const uint8_t wire_xor = payload[CMC_SEQ_BYTES + xor_off];
    const uint8_t expected_xor = (uint8_t)(prbs_exp[xor_off] ^ XOR_ZONE_MASK);
    out->xor_ok = (wire_xor == expected_xor);

    /* 4. The rest of the PRBS stream, short of the trailing DTN_SEQ byte -
     *    the sender overwrites that on purpose, so it is not PRBS any more. */
    const uint8_t *wire_prbs = payload + CMC_SEQ_BYTES + SPLITMIX_TOTAL_OVERHEAD;
    const uint8_t *exp_prbs  = prbs_exp + SPLITMIX_TOTAL_OVERHEAD;
    const uint32_t prbs_len  = CMC_NUM_PRBS_BYTES - SPLITMIX_TOTAL_OVERHEAD - 1;
    out->prbs_ok = memcmp(wire_prbs, exp_prbs, prbs_len) == 0;

    if (out->crc_ok && out->splitmix_ok && out->xor_ok && out->prbs_ok)
        return true;

    /* Bit errors over every zone, the way the reference adds them up: the
     * SplitMix and CRC zones always, the other two only when they failed. */
    uint64_t bits = payload_bit_errors(payload + CMC_SEQ_BYTES, expected_sm,
                                      SPLITMIX_XOR_BYTES);
    {
        uint8_t expected_crc_be[4];
        uint32_t be = __builtin_bswap32(calc_crc);

        memcpy(expected_crc_be, &be, sizeof be);
        bits += payload_bit_errors(payload + CMC_SEQ_BYTES + SPLITMIX_XOR_BYTES,
                                   expected_crc_be, 4);
    }
    if (!out->xor_ok)
        bits += (uint64_t)__builtin_popcount((unsigned)(wire_xor ^ expected_xor));
    if (!out->prbs_ok)
        bits += payload_bit_errors(wire_prbs, exp_prbs, prbs_len);

    out->bit_errors = bits;
    return false;
}

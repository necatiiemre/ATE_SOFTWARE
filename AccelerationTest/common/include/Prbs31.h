/**
 * @file Prbs31.h
 * @brief PRBS-31, generated once and read out of a cache.
 *
 * Two of the units' tests fill their payloads from a pseudo-random stream and
 * check what comes back against it. The stream is the same in both - x^31 + x^28
 * + 1, walked one bit at a time and packed eight bits to a byte, most
 * significant first, which is what the DPDK reference applications do and
 * therefore what the units are checked against - so it lives here rather than in
 * either of them.
 *
 * It is generated once, at the start of a run, and never per packet. The whole
 * period is a quarter of a gigabyte and takes about four seconds to produce; a
 * packet is then a memcpy out of it at an offset the sequence number decides,
 * which is what lets the receiver regenerate what it should have got from the
 * sequence alone and remember nothing between packets.
 *
 * The buffer holds one period plus one packet's worth repeated. The offset wraps
 * with the period, which is not a whole number of packets, so the last packet of
 * each period starts inside the period and runs off its end; the repeat absorbs
 * that, and every read stays a plain memcpy.
 */

#ifndef PRBS31_H
#define PRBS31_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PRBS31_PERIOD     0x7FFFFFFF
#define PRBS31_CACHE_SIZE ((size_t)(PRBS31_PERIOD / 8) + 1)   /* ~268 MB */

/** The initial state the reference applications use for their one server port. */
#define PRBS31_INITIAL_STATE 0x0000000Fu

typedef struct {
    uint8_t *bytes;          /**< PRBS31_CACHE_SIZE + stride */
    size_t   stride;         /**< bytes one sequence number advances by */
    uint32_t initial_state;
    bool     ready;
} prbs31_cache_t;

/**
 * @brief Generate @p len bytes of the stream from @p state; return the state after.
 *
 * The cache is one call of this over the whole period. Exposed so a test can
 * check the first bytes without waiting for a quarter of a gigabyte.
 */
uint32_t prbs31_fill(uint8_t *out, size_t len, uint32_t state);

/**
 * @brief Generate the whole period. Allocates PRBS31_CACHE_SIZE + @p stride.
 *
 * @param stride   how many bytes a packet takes from the stream, which is what
 *                 one step of the sequence number advances the offset by
 * @param progress called every 10 MB, so a few seconds of silence looks like work
 */
bool prbs31_cache_init(prbs31_cache_t *cache, uint32_t initial_state, size_t stride,
                       void (*progress)(size_t done, size_t total));

void prbs31_cache_free(prbs31_cache_t *cache);

/** Where this sequence's bytes start. At least @p stride of them follow. */
const uint8_t *prbs31_at(const prbs31_cache_t *cache, uint64_t seq);

#endif /* PRBS31_H */

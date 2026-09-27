#include "Prbs31.h"

#include <stdlib.h>
#include <string.h>

/* The reference applications' generator, unchanged. It taps bits 0 and 3 of a
 * 31-bit state, shifts right and feeds the new bit in at bit 30. Written the
 * obvious way rather than the fast way: it runs once, for a quarter of a
 * gigabyte, and being recognisably the same loop as theirs matters more than the
 * seconds it costs. */
static inline uint32_t prbs31_next(uint32_t *state)
{
    uint32_t output = *state & 0x01;
    uint32_t new_bit = ((*state & 0x01) ^ ((*state >> 3) & 0x01)) & 0x01;

    *state = (new_bit << 30 | (*state >> 1)) & 0x7FFFFFFF;
    return output;
}

uint32_t prbs31_fill(uint8_t *out, size_t len, uint32_t state)
{
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = 0;

        for (int bit = 0; bit < 8; bit++)
            byte = (uint8_t)((byte << 1) | prbs31_next(&state));
        out[i] = byte;
    }
    return state;
}

bool prbs31_cache_init(prbs31_cache_t *cache, uint32_t initial_state, size_t stride,
                       void (*progress)(size_t done, size_t total))
{
    if (!cache || stride == 0)
        return false;

    memset(cache, 0, sizeof *cache);

    cache->bytes = malloc(PRBS31_CACHE_SIZE + stride);
    if (!cache->bytes)
        return false;

    cache->stride = stride;
    cache->initial_state = initial_state;

    /* Generated in 10 MB pieces so the progress callback has something to say.
     * The stream is the same as one call over the whole period, because the
     * state is carried across. */
    const size_t chunk = 10u * 1024u * 1024u;
    uint32_t state = initial_state;

    for (size_t done = 0; done < PRBS31_CACHE_SIZE; done += chunk) {
        const size_t n = (PRBS31_CACHE_SIZE - done < chunk)
                             ? PRBS31_CACHE_SIZE - done : chunk;

        state = prbs31_fill(cache->bytes + done, n, state);
        if (progress)
            progress(done + n, PRBS31_CACHE_SIZE);
    }
    memcpy(cache->bytes + PRBS31_CACHE_SIZE, cache->bytes, stride);

    cache->ready = true;
    return true;
}

void prbs31_cache_free(prbs31_cache_t *cache)
{
    if (!cache)
        return;
    free(cache->bytes);
    cache->bytes = NULL;
    cache->ready = false;
}

const uint8_t *prbs31_at(const prbs31_cache_t *cache, uint64_t seq)
{
    if (!cache || !cache->ready)
        return NULL;
    return cache->bytes + (seq * (uint64_t)cache->stride) % (uint64_t)PRBS31_CACHE_SIZE;
}

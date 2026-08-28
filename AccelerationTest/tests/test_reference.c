/*
 * Regression test for the DTN configuration encoder.
 *
 * tests/fixtures/reference_frames.bin holds the 47 frames that
 * RemoteConfigSender ships as hard-coded hex. That configuration is accepted by
 * real hardware, so reproducing it byte for byte is the only correctness
 * evidence available without the DUT.
 *
 * For each frame: strip the headers, walk the Addr/Len/Data blocks (validating
 * every end-of-block marker on the way), feed the blocks back through
 * dtn_build_payload/dtn_build_frame and compare the result with the original.
 */

#include "DtnConfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FIXTURE "tests/fixtures/reference_frames.bin"

static uint8_t  g_blob[128 * 1024];
static size_t   g_blob_len;

static uint16_t rd_be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }

static int load_fixture(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "cannot open %s - run from the AccelerationTest directory\n", path);
        return -1;
    }
    g_blob_len = fread(g_blob, 1, sizeof g_blob, f);
    fclose(f);
    return g_blob_len > 0 ? 0 : -1;
}

/* Split a frame into its UDP payload and trailing sequence byte. */
static int split_frame(const uint8_t *frame, size_t len, const uint8_t **payload,
                       size_t *payload_len, uint8_t *seq, int *vlan)
{
    size_t off = 14;

    if (len < 43)
        return -1;
    if (frame[12] == 0x81 && frame[13] == 0x00) {
        *vlan = rd_be16(frame + 14) & 0x0FFF;
        off = 18;
    } else {
        *vlan = -1;
    }

    size_t udp_len = rd_be16(frame + off + 24);
    if (udp_len < 8 || off + 28 + udp_len - 8 > len)
        return -1;

    *payload     = frame + off + 28;
    *payload_len = udp_len - 8;
    *seq         = frame[len - 1];
    return 0;
}

/* Inverse of dtn_build_payload. Rejects any marker that does not match. */
static int parse_blocks(const uint8_t *payload, size_t len, dtn_block_t *blocks,
                        size_t max_blocks, size_t *count, bool *terminated)
{
    size_t i = 3, n = 0;

    *terminated = false;
    while (i < len) {
        if (n) {
            uint8_t want = dtn_block_marker(blocks[n - 1].addr);
            if (payload[i] != want) {
                fprintf(stderr, "  marker 0x%02x != expected 0x%02x at offset %zu\n",
                        payload[i], want, i);
                return -1;
            }
            i++;
            if (i == len) {          /* that marker terminated the datagram */
                *terminated = true;
                break;
            }
        }
        if (n == max_blocks || i + 3 > len)
            return -1;
        blocks[n].addr = payload[i];
        blocks[n].len  = rd_be16(payload + i + 1);
        blocks[n].data = payload + i + 3;
        if (i + 3 + blocks[n].len > len)
            return -1;
        i += 3 + blocks[n].len;
        n++;
    }
    *count = n;
    return 0;
}

int main(void)
{
    uint8_t payload[DTN_MAX_FRAME], rebuilt[DTN_MAX_FRAME];
    size_t  pos = 0, frame_no = 0, vl_records = 0;
    int     failures = 0;

    if (load_fixture(FIXTURE) < 0)
        return 1;

    while (pos + 2 <= g_blob_len) {
        size_t len = rd_be16(g_blob + pos);
        const uint8_t *frame = g_blob + pos + 2;
        pos += 2 + len;
        if (pos > g_blob_len) {
            fprintf(stderr, "truncated fixture\n");
            return 1;
        }

        const uint8_t *ref_payload;
        size_t ref_payload_len, block_count;
        uint8_t seq;
        int vlan;
        bool terminated;
        dtn_block_t blocks[DTN_MAX_BLOCKS];

        if (split_frame(frame, len, &ref_payload, &ref_payload_len, &seq, &vlan) < 0) {
            fprintf(stderr, "[FAIL] frame %zu: malformed\n", frame_no);
            failures++;
            goto next;
        }
        int plen, flen;
        if (ref_payload[2] == DTN_OP_WRITE) {
            if (parse_blocks(ref_payload, ref_payload_len, blocks, DTN_MAX_BLOCKS,
                             &block_count, &terminated) < 0) {
                fprintf(stderr, "[FAIL] frame %zu: block structure not understood\n", frame_no);
                failures++;
                goto next;
            }
            for (size_t b = 0; b < block_count; b++)
                if (blocks[b].addr == DTN_ADDR_VL_TABLE)
                    vl_records += blocks[b].len / DTN_VL_RECORD_LEN;

            plen = dtn_build_payload(blocks, block_count, DTN_OP_WRITE, terminated,
                                     payload, sizeof payload);
        } else {
            /* The 0x52 status query is a fixed payload, not a block chain. */
            plen = (int)ref_payload_len;
            memcpy(payload, ref_payload, ref_payload_len);
        }
        if (plen < 0) {
            fprintf(stderr, "[FAIL] frame %zu: payload build failed\n", frame_no);
            failures++;
            goto next;
        }

        flen = dtn_build_frame(payload, (size_t)plen, seq, 0, vlan, DTN_NET_A,
                               rebuilt, sizeof rebuilt);
        if (flen != (int)len || memcmp(rebuilt, frame, len) != 0) {
            fprintf(stderr, "[FAIL] frame %zu: rebuilt %d bytes, reference %zu bytes\n",
                    frame_no, flen, len);
            for (int k = 0; k < flen && k < (int)len; k++)
                if (rebuilt[k] != frame[k]) {
                    fprintf(stderr, "       first difference at byte %d: got 0x%02x want 0x%02x\n",
                            k, rebuilt[k], frame[k]);
                    break;
                }
            failures++;
        }
next:
        frame_no++;
    }

    printf("%zu reference frames, %zu VL records\n", frame_no, vl_records);
    if (failures) {
        printf("FAILED: %d frame(s) did not match\n", failures);
        return 1;
    }
    printf("PASS: every reference frame reproduced byte for byte\n");
    return 0;
}

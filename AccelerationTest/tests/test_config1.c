/*
 * Byte-exact check against the captured configuration.
 *
 * tests/fixtures/config1_switch.bin holds the two switch datagrams of the
 * config1 capture - the UDP payload plus the trailing AFDX sequence byte, which
 * is how the capture was read out. They are seq 2 and seq 3, so two datagrams
 * precede them; the fixture covers the part that changes with the round.
 *
 * The check is what makes the encoder trustworthy: record layout, record order,
 * flag nibble, block chain, end-of-block markers, the 104-record split and the
 * sequence numbering all have to be right at once for this to pass.
 */

#include "VlProfile.h"

#include <stdio.h>
#include <string.h>

#define FIXTURE "tests/fixtures/config1_switch.bin"

static uint8_t     g_blob[8 * 1024];
static size_t      g_blob_len;
static dtn_vl_t    g_records[VL_PROFILE_MAX_RECORDS];
static dtn_frame_t g_frames[DTN_MAX_CONFIG_FRAMES];

static uint16_t rd_be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }

/* The part of a built frame the fixture records: UDP payload plus sequence byte. */
static const uint8_t *frame_payload(const dtn_frame_t *frame, size_t *len)
{
    size_t off = (frame->data[12] == 0x81 && frame->data[13] == 0x00) ? 18 : 14;
    size_t udp_len = rd_be16(frame->data + off + 24);

    *len = udp_len - 8 + 1;              /* + the trailing sequence byte */
    return frame->data + off + 28;
}

static void dump_first_difference(const uint8_t *ours, size_t ours_len,
                                  const uint8_t *ref, size_t ref_len)
{
    size_t n = ours_len < ref_len ? ours_len : ref_len;

    for (size_t i = 0; i < n; i++)
        if (ours[i] != ref[i]) {
            printf("         first difference at offset %zu: 0x%02x, capture has 0x%02x\n",
                   i, ours[i], ref[i]);
            return;
        }
    printf("         identical for %zu bytes, then the lengths differ\n", n);
}

int main(void)
{
    FILE *f = fopen(FIXTURE, "rb");
    if (!f) {
        fprintf(stderr, "cannot open %s - run from the AccelerationTest directory\n",
                FIXTURE);
        return 1;
    }
    g_blob_len = fread(g_blob, 1, sizeof g_blob, f);
    fclose(f);

    size_t profile_count;
    const vl_profile_t *profiles = vl_profile_all(&profile_count);
    const vl_profile_t *config1 = NULL;

    for (size_t i = 0; i < profile_count; i++)
        if (strcmp(profiles[i].name, "config1") == 0)
            config1 = &profiles[i];
    if (!config1) {
        puts("[FAIL] there is no config1 profile");
        return 1;
    }
    if (config1->management) {
        puts("[FAIL] config1 carries the management VLs; the capture does not");
        return 1;
    }

    int count = vl_profile_expand(config1, g_records, VL_PROFILE_MAX_RECORDS);
    if (count != 122) {
        printf("[FAIL] config1 expands to %d records, the capture has 122\n", count);
        return 1;
    }

    size_t protocol_len;
    const uint8_t *protocol = vl_profile_protocol_block(&protocol_len);
    int frames = dtn_build_config_frames(g_records, (size_t)count, protocol,
                                         protocol_len, -1, &DTN_CONFIG_REFERENCE,
                                         g_frames, DTN_MAX_CONFIG_FRAMES);
    if (frames < 0) {
        puts("[FAIL] the configuration frames could not be built");
        return 1;
    }

    /* The capture's switch datagrams are seq 2 and 3, so ours have to be too. */
    int failures = 0;
    size_t pos = 0;
    for (uint8_t seq = 2; seq <= 3; seq++) {
        if (pos + 2 > g_blob_len) {
            printf("[FAIL] the fixture has no datagram for seq %u\n", seq);
            failures++;
            break;
        }
        size_t ref_len = rd_be16(g_blob + pos);
        const uint8_t *ref = g_blob + pos + 2;
        pos += 2 + ref_len;

        const dtn_frame_t *ours = NULL;
        for (int i = 0; i < frames; i++)
            if (g_frames[i].seq == seq)
                ours = &g_frames[i];
        if (!ours) {
            printf("[FAIL] we do not send a datagram with seq %u\n", seq);
            failures++;
            continue;
        }

        size_t our_len;
        const uint8_t *our_payload = frame_payload(ours, &our_len);
        if (our_len != ref_len || memcmp(our_payload, ref, ref_len) != 0) {
            printf("[FAIL] seq %u: %zu bytes against the capture's %zu\n",
                   seq, our_len, ref_len);
            dump_first_difference(our_payload, our_len, ref, ref_len);
            failures++;
            continue;
        }
        printf("[ OK ] seq %u matches the capture byte for byte (%zu bytes, %s)\n",
               seq, ref_len, ours->label);
    }

    if (failures) {
        printf("FAILED: %d datagram(s) differ from the capture\n", failures);
        return 1;
    }
    printf("PASS: config1's switch datagrams reproduce the capture\n");
    return 0;
}

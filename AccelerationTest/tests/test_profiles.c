/*
 * Profile tests.
 *
 * The management VLs are copied from the reference configuration rather than
 * derived, so the thing worth guarding is that the copy stays faithful: pull
 * VL 4484-4490 out of the reference fixture and compare them with what
 * vl_profile_management() produces. Then check every round expands, validates
 * and fits into its frames.
 */

#include "VlProfile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FIXTURE "tests/fixtures/reference_frames.bin"

static uint8_t g_blob[128 * 1024];
static size_t  g_blob_len;
static dtn_vl_t    g_records[VL_PROFILE_MAX_RECORDS];
static dtn_frame_t g_frames[DTN_MAX_CONFIG_FRAMES];

static uint16_t rd_be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }

/* Find one VL record in the reference configuration. */
static const uint8_t *reference_record(uint16_t vl_id)
{
    size_t pos = 0;

    while (pos + 2 <= g_blob_len) {
        size_t len = rd_be16(g_blob + pos);
        const uint8_t *frame = g_blob + pos + 2;
        pos += 2 + len;

        size_t off = (frame[12] == 0x81 && frame[13] == 0x00) ? 18 : 14;
        const uint8_t *payload = frame + off + 28;
        size_t payload_len = (size_t)rd_be16(frame + off + 24) - 8;
        if (payload[2] != DTN_OP_WRITE)
            continue;

        size_t i = 3;
        uint8_t prev_addr = 0;
        bool first = true;
        while (i < payload_len) {
            if (!first) {
                if (payload[i] != dtn_block_marker(prev_addr))
                    break;
                if (++i >= payload_len)
                    break;
            }
            uint8_t addr = payload[i];
            uint16_t blen = rd_be16(payload + i + 1);
            const uint8_t *data = payload + i + 3;

            if (addr == DTN_ADDR_VL_TABLE)
                for (uint16_t k = 0; k < blen / DTN_VL_RECORD_LEN; k++)
                    if (rd_be16(data + k * DTN_VL_RECORD_LEN) == vl_id)
                        return data + k * DTN_VL_RECORD_LEN;

            i += 3u + blen;
            prev_addr = addr;
            first = false;
        }
    }
    return NULL;
}

static int check_management(void)
{
    size_t count;
    const dtn_vl_t *mgmt = vl_profile_management(&count);
    int failures = 0;

    for (size_t i = 0; i < count; i++) {
        uint8_t ours[DTN_VL_RECORD_LEN];
        const uint8_t *ref = reference_record(mgmt[i].vl_id);

        if (dtn_vl_encode(&mgmt[i], ours) < 0) {
            printf("[FAIL] management VL %u does not encode\n", mgmt[i].vl_id);
            failures++;
            continue;
        }
        if (!ref) {
            printf("[FAIL] VL %u is not in the reference configuration\n", mgmt[i].vl_id);
            failures++;
            continue;
        }
        if (memcmp(ours, ref, DTN_VL_RECORD_LEN) != 0) {
            printf("[FAIL] management VL %u differs from the reference\n", mgmt[i].vl_id);
            failures++;
        }
    }
    /* And the protocol block, which has to name the same VLs. */
    size_t protocol_len;
    const uint8_t *protocol = vl_profile_protocol_block(&protocol_len);
    if (protocol_len != 200) {
        printf("[FAIL] protocol block is %zu bytes, the reference is 200\n", protocol_len);
        failures++;
    } else if (protocol[0] != 0x00 || protocol[1] != 0x04) {
        printf("[FAIL] protocol block does not start like the reference\n");
        failures++;
    }

    if (!failures)
        printf("[ OK ] %zu management VLs and the %zu-byte protocol block match "
               "the reference\n", count, protocol_len);
    return failures;
}

static int check_profiles(bool dense)
{
    size_t count;
    const vl_profile_t *profiles = vl_profile_all(&count);
    int failures = 0;

    for (size_t i = 0; i < count; i++) {
        char reason[128];
        int records = vl_profile_expand(&profiles[i], g_records, VL_PROFILE_MAX_RECORDS, dense);
        if (records < 0) {
            printf("[FAIL] %s does not fit in the VL table\n", profiles[i].name);
            failures++;
            continue;
        }
        if (!vl_profile_validate(g_records, (size_t)records, reason, sizeof reason)) {
            printf("[FAIL] %s is not usable: %s\n", profiles[i].name, reason);
            failures++;
            continue;
        }
        for (int k = 1; k < records; k++)
            if (g_records[k].vl_id <= g_records[k - 1].vl_id) {
                printf("[FAIL] %s: VL ids are not sorted at index %d\n", profiles[i].name, k);
                failures++;
                break;
            }

        size_t protocol_len;
        const uint8_t *protocol_block = vl_profile_protocol_block(&protocol_len);
        int frames = dtn_build_config_frames(g_records, (size_t)records,
                                             protocol_block, protocol_len, -1,
                                             g_frames, DTN_MAX_CONFIG_FRAMES);
        if (frames < 0) {
            printf("[FAIL] %s: frames could not be built\n", profiles[i].name);
            failures++;
            continue;
        }
        size_t bytes = 0;
        for (int f = 0; f < frames; f++)
            bytes += g_frames[f].len;
        printf("[ OK ] %-8s %s  %4d records (%zu enabled), %2d frames, %6zu bytes\n",
               profiles[i].name, dense ? "dense " : "sparse", records,
               vl_profile_enabled_count(g_records, (size_t)records), frames, bytes);
    }
    return failures;
}

int main(void)
{
    FILE *f = fopen(FIXTURE, "rb");
    if (!f) {
        fprintf(stderr, "cannot open %s - run from the AccelerationTest directory\n", FIXTURE);
        return 1;
    }
    g_blob_len = fread(g_blob, 1, sizeof g_blob, f);
    fclose(f);

    int failures = check_management() + check_profiles(false) + check_profiles(true);
    if (failures) {
        printf("FAILED: %d check(s)\n", failures);
        return 1;
    }
    puts("PASS: profiles are consistent with the reference configuration");
    return 0;
}

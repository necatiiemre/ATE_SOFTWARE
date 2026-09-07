/*
 * Byte-exact check against the captured configuration.
 *
 * tests/fixtures/config1_switch.bin holds the two switch datagrams of the
 * config1 capture - the UDP payload plus the trailing AFDX sequence byte, which
 * is how the capture was read out.
 *
 * We send one record more than the capture does: DTN_HEALTH_MONITOR_VL, the
 * device's own health monitor out to copper, which the capture leaves out. So
 * the check is in two halves. The 122 records the capture does carry have to
 * come out byte for byte in the capture's order, and the framing around them -
 * block chain, markers, the 104-record split, the sequence numbering - has to
 * be right.
 *
 * The capture's datagrams are numbered seq 2 and seq 3 because they were taken
 * from a run that also sent the 0x46 datagram. We send three datagrams, so ours
 * are seq 1 and seq 2; the sequence byte is ours to assign and is checked
 * separately from the payload.
 */

#include "VlProfile.h"

#include <stdio.h>
#include <string.h>

#define FIXTURE "tests/fixtures/config1_switch.bin"
#define CAPTURE_RECORDS 122

static uint8_t     g_blob[8 * 1024];
static size_t      g_blob_len;
static uint8_t     g_capture[CAPTURE_RECORDS * DTN_VL_RECORD_LEN];
static size_t      g_capture_len;
static dtn_vl_t    g_records[VL_PROFILE_MAX_RECORDS];
static dtn_frame_t g_frames[DTN_MAX_CONFIG_FRAMES];

static uint16_t rd_be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }

/* ------------------------------------------------------------------ */
/* Walking a datagram's Addr/Len/Data chain, validating every marker. */

typedef struct {
    uint8_t  addr[DTN_MAX_BLOCKS];
    uint16_t len[DTN_MAX_BLOCKS];
    const uint8_t *data[DTN_MAX_BLOCKS];
    size_t   count;
} chain_t;

static int walk_chain(const uint8_t *payload, size_t len, chain_t *out)
{
    size_t i = 3;

    out->count = 0;
    if (len < 3 || rd_be16(payload) != DTN_LRU_ID || payload[2] != DTN_OP_WRITE)
        return -1;

    while (i < len) {
        if (out->count) {
            if (payload[i] != dtn_block_marker(out->addr[out->count - 1]))
                return -1;
            if (++i == len)
                return 0;                /* that marker closed the datagram */
        }
        if (out->count == DTN_MAX_BLOCKS || i + 3 > len)
            return -1;
        out->addr[out->count] = payload[i];
        out->len[out->count]  = rd_be16(payload + i + 1);
        out->data[out->count] = payload + i + 3;
        i += 3u + out->len[out->count];
        if (i > len)
            return -1;
        out->count++;
    }
    return 0;
}

/* The capture's VL records, concatenated across its two datagrams. */
static int load_capture(void)
{
    size_t pos = 0;

    while (pos + 2 <= g_blob_len) {
        size_t len = rd_be16(g_blob + pos);
        const uint8_t *payload = g_blob + pos + 2;
        chain_t chain;

        pos += 2 + len;
        if (len == 0)
            return -1;
        /* The last byte is the AFDX sequence byte, outside the payload. */
        if (walk_chain(payload, len - 1, &chain) < 0)
            return -1;

        for (size_t b = 0; b < chain.count; b++) {
            if (chain.addr[b] != DTN_ADDR_VL_TABLE)
                continue;
            if (g_capture_len + chain.len[b] > sizeof g_capture)
                return -1;
            memcpy(g_capture + g_capture_len, chain.data[b], chain.len[b]);
            g_capture_len += chain.len[b];
        }
    }
    return g_capture_len == sizeof g_capture ? 0 : -1;
}

/* ------------------------------------------------------------------ */

static int check_records(int count)
{
    uint8_t ours[DTN_VL_RECORD_LEN];

    for (int i = 0; i < CAPTURE_RECORDS; i++) {
        const uint8_t *ref = g_capture + (size_t)i * DTN_VL_RECORD_LEN;

        if (dtn_vl_encode(&g_records[i], ours) < 0) {
            printf("[FAIL] record %d does not encode\n", i);
            return 1;
        }
        if (memcmp(ours, ref, DTN_VL_RECORD_LEN) != 0) {
            printf("[FAIL] record %d (VL %u) differs from the capture\n"
                   "         ours    :", i, g_records[i].vl_id);
            for (int k = 0; k < DTN_VL_RECORD_LEN; k++) printf(" %02x", ours[k]);
            printf("\n         capture :");
            for (int k = 0; k < DTN_VL_RECORD_LEN; k++) printf(" %02x", ref[k]);
            putchar('\n');
            return 1;
        }
    }
    printf("[ OK ] %d records match the capture byte for byte, in its order\n",
           CAPTURE_RECORDS);

    if (count != CAPTURE_RECORDS + 1) {
        printf("[FAIL] %d records; expected the capture's %d plus the DTN's own "
               "health monitor\n", count, CAPTURE_RECORDS);
        return 1;
    }
    const dtn_vl_t *hm = &g_records[CAPTURE_RECORDS];
    if (hm->vl_id != DTN_HEALTH_MONITOR_VL || hm->src_port != DTN_PORT_MANAGEMENT ||
        hm->dest_mask != 1ull << DTN_HEALTH_MONITOR_PORT) {
        printf("[FAIL] the extra record is VL %u, port %u -> mask %llx\n",
               hm->vl_id, hm->src_port, (unsigned long long)hm->dest_mask);
        return 1;
    }
    if (dtn_vl_encode(hm, ours) < 0) {
        puts("[FAIL] the health-monitor record does not encode");
        return 1;
    }
    printf("[ OK ] plus VL %u, port %u -> port %u  (", hm->vl_id, hm->src_port,
           DTN_HEALTH_MONITOR_PORT);
    for (int k = 0; k < DTN_VL_RECORD_LEN; k++) printf("%s%02x", k ? " " : "", ours[k]);
    puts(")");
    return 0;
}

/* seq 0 end system, seq 1 and 2 the switch table, seq 3 the status query. */
static int check_frames(int frames)
{
    static const struct {
        uint8_t  seq;
        size_t   blocks;
        uint8_t  addr[4];
        const char *what;
    } want[] = {
        {0, 2, {DTN_ADDR_ES_GLOBAL, DTN_ADDR_ES_PARAMS},                     "end system"},
        {1, 2, {DTN_ADDR_SW_BEGIN,  DTN_ADDR_VL_TABLE},                      "switch table, part 1"},
        {2, 3, {DTN_ADDR_VL_TABLE,  DTN_ADDR_SW_MISC, DTN_ADDR_SW_END},      "switch table, part 2"},
    };

    if (frames != 4) {
        printf("[FAIL] %d frames; the configuration is three plus a status query\n",
               frames);
        return 1;
    }
    for (size_t i = 0; i < sizeof want / sizeof want[0]; i++) {
        const dtn_frame_t *f = &g_frames[i];
        size_t off = (f->data[12] == 0x81 && f->data[13] == 0x00) ? 18 : 14;
        size_t payload_len = (size_t)rd_be16(f->data + off + 24) - 8;
        chain_t chain;

        if (f->seq != want[i].seq) {
            printf("[FAIL] frame %zu is seq %u, expected %u\n", i, f->seq, want[i].seq);
            return 1;
        }
        if (walk_chain(f->data + off + 28, payload_len, &chain) < 0 ||
            chain.count != want[i].blocks) {
            printf("[FAIL] seq %u: block chain is not what %s should be\n",
                   f->seq, want[i].what);
            return 1;
        }
        for (size_t b = 0; b < chain.count; b++)
            if (chain.addr[b] != want[i].addr[b]) {
                printf("[FAIL] seq %u: block %zu is 0x%02x, expected 0x%02x\n",
                       f->seq, b, chain.addr[b], want[i].addr[b]);
                return 1;
            }
        printf("[ OK ] seq %u  %-21s %4u bytes,", f->seq, want[i].what, f->len);
        for (size_t b = 0; b < chain.count; b++)
            printf(" 0x%02x(%u)", chain.addr[b], chain.len[b]);
        putchar('\n');
    }
    if (g_frames[3].seq != 3 || strcmp(g_frames[3].label, "status query") != 0) {
        puts("[FAIL] the configuration does not end with a status query at seq 3");
        return 1;
    }
    printf("[ OK ] seq 3  %-21s %4u bytes\n", "status query", g_frames[3].len);
    return 0;
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

    if (load_capture() != 0) {
        printf("[FAIL] the fixture does not hold %d VL records\n", CAPTURE_RECORDS);
        return 1;
    }

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
        puts("[FAIL] config1 carries the full management path; the capture does not");
        return 1;
    }

    int count = vl_profile_expand(config1, g_records, VL_PROFILE_MAX_RECORDS);
    if (count < 0) {
        puts("[FAIL] config1 does not fit in the VL table");
        return 1;
    }

    size_t protocol_len;
    const uint8_t *protocol = vl_profile_protocol_block(&protocol_len);
    int frames = dtn_build_config_frames(g_records, (size_t)count, protocol,
                                         protocol_len, -1, &DTN_CONFIG_DEFAULT,
                                         g_frames, DTN_MAX_CONFIG_FRAMES);
    if (frames < 0) {
        puts("[FAIL] the configuration frames could not be built");
        return 1;
    }

    if (check_records(count) || check_frames(frames)) {
        puts("FAILED: config1 does not match the capture");
        return 1;
    }
    puts("PASS: config1 reproduces the capture, plus the DTN's health monitor");
    return 0;
}

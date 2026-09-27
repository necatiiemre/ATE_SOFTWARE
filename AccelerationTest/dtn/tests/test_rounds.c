/*
 * The captured configuration, and the rounds that still have its shape.
 *
 * fixtures/config1_switch.bin holds the two switch datagrams of the capture -
 * the UDP payload plus the trailing AFDX sequence byte, which is how it was read
 * out. It is the only evidence there is that the encoder emits frames real
 * hardware accepts, so it is checked byte for byte against the profile that
 * reproduces it: vl_profile_reference(), which is not in the menu and does not
 * change when the rounds do.
 *
 * config2 and config3 are still the captured table with the fibre ports moved,
 * so they are checked the same way: take each captured record, substitute the
 * ports that round uses, and nothing else may differ.
 *
 * config1 is no longer that table. It pairs four adjacent fibre ports, taps the
 * VMC on ports 8 and 9 into both copper links, and adds 240 records for the
 * copper legs. Nothing was captured of it, so the claim made about it here is
 * the one that can be made without hardware: it expands to exactly the routing
 * its profile declares, and the framing around it is still right.
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

#define FIXTURE "fixtures/config1_switch.bin"
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

/* One round's ports. The VL ids, timing and flags come from the capture. */
typedef struct {
    const char *name;
    uint8_t     fwd_base;   /**< first of the six low fibre ports */
    uint8_t     rev_base;   /**< first of the six high fibre ports */
    uint8_t     tap[2];     /**< where the fibre-side unit's health monitor taps */
} round_ports_t;

/* config1 is not here: it is no longer the captured table with different ports,
 * so there is nothing to substitute. It has its own check further down. */
static const round_ports_t g_rounds[] = {
    {"config2",  6, 22, {15, 31}},
    {"config3", 10, 26, { 0, 16}},
};

/* The capture's record i, with this round's ports written into it. Records 0-59
 * are the six forward links, 60-119 the six reverse links, 120-121 the taps. */
static void expected_record(const round_ports_t *r, int i, uint8_t out[DTN_VL_RECORD_LEN])
{
    uint8_t src, dst;

    memcpy(out, g_capture + (size_t)i * DTN_VL_RECORD_LEN, DTN_VL_RECORD_LEN);
    if (i < 60) {
        src = (uint8_t)(r->fwd_base + i / 10);
        dst = (uint8_t)(r->rev_base + i / 10);
    } else if (i < 120) {
        src = (uint8_t)(r->rev_base + (i - 60) / 10);
        dst = (uint8_t)(r->fwd_base + (i - 60) / 10);
    } else {
        src = r->tap[i - 120];
        dst = DTN_HEALTH_MONITOR_PORT;
    }
    uint64_t mask = 1ull << dst;
    out[8]  = (uint8_t)(mask >> 32);
    out[9]  = src;
    out[10] = (uint8_t)(mask >> 24);
    out[11] = (uint8_t)(mask >> 16);
    out[12] = (uint8_t)(mask >> 8);
    out[13] = (uint8_t)mask;
}

static int check_round(const round_ports_t *r)
{
    const vl_profile_t *profile = NULL;
    size_t profile_count;
    const vl_profile_t *profiles = vl_profile_all(&profile_count);
    uint8_t ours[DTN_VL_RECORD_LEN], want[DTN_VL_RECORD_LEN];

    for (size_t i = 0; i < profile_count; i++)
        if (strcmp(profiles[i].name, r->name) == 0)
            profile = &profiles[i];
    if (!profile) {
        printf("[FAIL] there is no %s profile\n", r->name);
        return 1;
    }

    int count = vl_profile_expand(profile, g_records, VL_PROFILE_MAX_RECORDS);
    if (count != CAPTURE_RECORDS + 1) {
        printf("[FAIL] %s expands to %d records, expected %d\n",
               r->name, count, CAPTURE_RECORDS + 1);
        return 1;
    }
    for (int i = 0; i < CAPTURE_RECORDS; i++) {
        expected_record(r, i, want);
        if (dtn_vl_encode(&g_records[i], ours) < 0 ||
            memcmp(ours, want, DTN_VL_RECORD_LEN) != 0) {
            printf("[FAIL] %s record %d (VL %u) is not the capture with this "
                   "round's ports\n         ours    :", r->name, i, g_records[i].vl_id);
            for (int k = 0; k < DTN_VL_RECORD_LEN; k++) printf(" %02x", ours[k]);
            printf("\n         expected:");
            for (int k = 0; k < DTN_VL_RECORD_LEN; k++) printf(" %02x", want[k]);
            putchar('\n');
            return 1;
        }
    }
    printf("[ OK ] %s  %d records: the capture with ports %u-%u <-> %u-%u, "
           "taps %u and %u\n", r->name, count, r->fwd_base, r->fwd_base + 5,
           r->rev_base, r->rev_base + 5, r->tap[0], r->tap[1]);
    return 0;
}

/* config1, against the routing its own profile declares.
 *
 * Nothing was captured of this configuration, so the check is not "it matches a
 * blob" but "every record says what the profile says it should": the right VL id
 * on the right port pair, in the right order, with the flags the device expects.
 * That is what catches a fat-fingered port number or a VL run that overlaps
 * another, which is the mistake this table invites.
 */
static int check_config1(void)
{
    const vl_profile_t *profiles;
    const vl_profile_t *p = NULL;
    size_t n;

    profiles = vl_profile_all(&n);
    for (size_t i = 0; i < n; i++)
        if (strcmp(profiles[i].name, "config1") == 0)
            p = &profiles[i];
    if (!p) {
        puts("[FAIL] there is no config1 profile");
        return 1;
    }

    static dtn_vl_t rec[VL_PROFILE_MAX_RECORDS];
    int count = vl_profile_expand(p, rec, VL_PROFILE_MAX_RECORDS);

    /* What the round is: four fibre pairs both ways at ten VLs each, two taps,
     * the DTN's own health monitor, and four copper legs of sixty. */
    const int want_count = 4 * 10 * 2 + 2 + 1 + 4 * 60;

    if (count != want_count) {
        printf("[FAIL] config1 expands to %d records, expected %d\n", count, want_count);
        return 1;
    }

    /* Every record, in order, as the profile declares it. */
    struct { uint16_t vl; uint8_t src, dst; uint8_t flags; } want[VL_PROFILE_MAX_RECORDS];
    int w = 0;

    static const uint8_t fwd[4][2] = {{0,4},{1,5},{2,6},{3,7}};
    for (int l = 0; l < 4; l++)
        for (int k = 0; k < 10; k++)
            want[w++] = (typeof(want[0])){(uint16_t)(1024 + l * 10 + k),
                                          fwd[l][0], fwd[l][1], 0x9};
    for (int l = 0; l < 4; l++)
        for (int k = 0; k < 10; k++)
            want[w++] = (typeof(want[0])){(uint16_t)(2024 + l * 10 + k),
                                          fwd[l][1], fwd[l][0], 0x9};

    want[w++] = (typeof(want[0])){100, 8, 32, 0x9};    /* the VMC's health monitor */
    want[w++] = (typeof(want[0])){101, 9, 33, 0x9};
    want[w++] = (typeof(want[0])){DTN_HEALTH_MONITOR_VL, 34, 32, 0xD};

    static const uint16_t leg_first[4] = {3024, 4024, 5024, 6024};
    static const uint8_t  leg_ports[4][2] = {{32,8},{8,32},{33,9},{9,33}};
    for (int g = 0; g < 4; g++)
        for (int k = 0; k < 60; k++)
            want[w++] = (typeof(want[0])){(uint16_t)(leg_first[g] + k),
                                          leg_ports[g][0], leg_ports[g][1], 0x9};

    if (w != want_count) {
        printf("[FAIL] the test's own expectation is %d records, not %d\n", w, want_count);
        return 1;
    }

    for (int i = 0; i < count; i++) {
        const uint64_t mask = 1ull << want[i].dst;

        if (rec[i].vl_id != want[i].vl || rec[i].src_port != want[i].src ||
            rec[i].dest_mask != mask || rec[i].flags != want[i].flags) {
            printf("[FAIL] config1 record %d: VL %u port %u -> mask %llx flags 0x%X, "
                   "expected VL %u port %u -> port %u flags 0x%X\n",
                   i, rec[i].vl_id, rec[i].src_port,
                   (unsigned long long)rec[i].dest_mask, rec[i].flags,
                   want[i].vl, want[i].src, want[i].dst, want[i].flags);
            return 1;
        }
    }

    /* The routing has to be sound on the hardware as well as on paper: no port
     * carrying fibre traffic and health-monitor data at once, no repeated VL. */
    char reason[128];
    if (!vl_profile_validate(rec, (size_t)count, reason, sizeof reason)) {
        printf("[FAIL] config1 does not validate: %s\n", reason);
        return 1;
    }

    /* And it has to fit in frames. 323 records is four switch datagrams where
     * the capture needed two, which is the part of this change most likely to
     * run into a limit. */
    int frames = dtn_build_config_frames(rec, (size_t)count,
                                         NULL, 0, -1, &DTN_CONFIG_DEFAULT,
                                         g_frames, DTN_MAX_CONFIG_FRAMES);
    if (frames < 0) {
        puts("[FAIL] config1 does not fit in configuration frames");
        return 1;
    }

    printf("[ OK ] config1  %d records, %d frames: fibre 0-3 <-> 4-7, VMC taps on "
           "8 and 9, copper legs 32<->8 and 33<->9\n", count, frames);
    return 0;
}

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

    const vl_profile_t *reference = vl_profile_reference();

    if (reference->management) {
        puts("[FAIL] the reference profile carries the full management path; "
             "the capture does not");
        return 1;
    }
    if (reference->comm_count != 0) {
        puts("[FAIL] the reference profile carries copper legs; the capture does not");
        return 1;
    }

    int count = vl_profile_expand(reference, g_records, VL_PROFILE_MAX_RECORDS);
    if (count < 0) {
        puts("[FAIL] the reference profile does not fit in the VL table");
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

    int failures = check_records(count) || check_frames(frames);
    for (size_t i = 0; i < sizeof g_rounds / sizeof g_rounds[0]; i++)
        failures += check_round(&g_rounds[i]);
    failures += check_config1();

    if (failures) {
        puts("FAILED: a round does not match the capture");
        return 1;
    }
    puts("PASS: the capture is reproduced byte for byte, plus the DTN's "
         "health monitor");
    return 0;
}

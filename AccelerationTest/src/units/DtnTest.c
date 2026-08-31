/*
 * DTN acceleration test.
 *
 * The operator picks one of the three rounds, the profile is expanded into a VL
 * table and turned into the frames that configure the DTN. Sending them over
 * the copper link and reading the health-monitor stream back is the next piece
 * of work; for now the test shows exactly what it would put on the wire.
 */

#include "units/DtnTest.h"

#include "DtnConfig.h"
#include "Prompt.h"
#include "VlProfile.h"

#include <stdio.h>

static dtn_vl_t    g_records[VL_PROFILE_MAX_RECORDS];
static dtn_frame_t g_frames[DTN_MAX_CONFIG_FRAMES];

static const vl_profile_t *select_profile(void)
{
    size_t count;
    const vl_profile_t *profiles = vl_profile_all(&count);

    puts("\nSelect configuration round");
    for (size_t i = 0; i < count; i++)
        printf("  %zu) %-8s %s\n", i + 1, profiles[i].name, profiles[i].description);
    puts("  0) Back");

    int choice = prompt_menu("Choice", 0, (int)count, 0);
    return choice == 0 ? NULL : &profiles[choice - 1];
}

/* Collapse the record list into "port A -> port B, VL x..y" runs. */
static void print_routing(const dtn_vl_t *records, size_t count)
{
    size_t i = 0;

    puts("\n  routing");
    while (i < count) {
        size_t j = i + 1;
        while (j < count &&
               records[j].src_port  == records[i].src_port &&
               records[j].dest_mask == records[i].dest_mask &&
               records[j].vl_id     == records[j - 1].vl_id + 1)
            j++;

        int dst = -1;
        for (int p = 0; p < DTN_PORT_COUNT; p++)
            if (records[i].dest_mask >> p & 1) {
                dst = p;
                break;
            }

        bool copper = (dst == 32 || dst == 33);
        printf("    %-3s port %2u -> %2d   VL %u", copper ? "HM" : "", records[i].src_port,
               dst, records[i].vl_id);
        if (j - i > 1)
            printf("-%u  (%zu VLs)", records[j - 1].vl_id, j - i);
        putchar('\n');
        i = j;
    }
}

unit_result_t dtn_test_run(void)
{
    const vl_profile_t *profile = select_profile();
    if (!profile)
        return UNIT_RESULT_ABORTED;

    int count = vl_profile_expand(profile, g_records, VL_PROFILE_MAX_RECORDS);
    if (count < 0) {
        puts("Profile does not fit in the VL table.");
        return UNIT_RESULT_ERROR;
    }

    char reason[128];
    if (!vl_profile_validate(g_records, (size_t)count, reason, sizeof reason)) {
        printf("Profile is not usable: %s\n", reason);
        return UNIT_RESULT_ERROR;
    }

    /* Untagged: the workstation is wired straight to the DTN's copper
     * end-system ports, with no bridge in between to steer on a VLAN tag. */
    int frames = dtn_build_config_frames(g_records, (size_t)count, -1,
                                         g_frames, DTN_MAX_CONFIG_FRAMES);
    if (frames < 0) {
        puts("Could not build the configuration frames.");
        return UNIT_RESULT_ERROR;
    }

    size_t total = 0;
    for (int i = 0; i < frames; i++)
        total += g_frames[i].len;

    printf("\n  profile     : %s - %s\n", profile->name, profile->description);
    printf("  VL records  : %d  (VL %u..%u)\n", count,
           g_records[0].vl_id, g_records[count - 1].vl_id);
    printf("  frames      : %d, %zu bytes, untagged\n", frames, total);
    for (int i = 0; i < frames; i++)
        printf("      seq %3u  %5u B  %s\n", g_frames[i].seq, g_frames[i].len,
               g_frames[i].label);

    print_routing(g_records, (size_t)count);

    puts("\n  Sending these over the copper link and reading the health monitor");
    puts("  back is not wired up yet.");
    return UNIT_RESULT_ABORTED;
}

#include "VlWatch.h"

#include "AppConfig.h"
#include "HealthMonitor.h"
#include "Log.h"

#include <stdio.h>
#include <string.h>

#define COPPER_MASK ((1ull << 32) | (1ull << 33))

static vl_sighting_t *find(vl_watch_t *watch, uint8_t dtn_port, uint16_t vl_id)
{
    for (size_t i = 0; i < watch->count; i++)
        if (watch->entries[i].vl_id == vl_id && watch->entries[i].dtn_port == dtn_port)
            return &watch->entries[i];
    return NULL;
}

static vl_sighting_t *add(vl_watch_t *watch, uint8_t dtn_port, uint16_t vl_id, bool expected)
{
    if (watch->count == VL_WATCH_MAX_ENTRIES) {
        watch->overflow = true;
        return NULL;
    }
    vl_sighting_t *e = &watch->entries[watch->count++];
    memset(e, 0, sizeof *e);
    e->vl_id    = vl_id;
    e->dtn_port = dtn_port;
    e->expected = expected;
    return e;
}

void vl_watch_init(vl_watch_t *watch, const dtn_vl_t *records, size_t count)
{
    memset(watch, 0, sizeof *watch);

    for (size_t i = 0; i < count; i++) {
        uint64_t copper = records[i].dest_mask & COPPER_MASK;
        if (!copper)
            continue;
        for (uint8_t port = 32; port <= 33; port++)
            if (copper >> port & 1)
                add(watch, port, records[i].vl_id, true);
    }
}

void vl_watch_saw(vl_watch_t *watch, uint8_t dtn_port, uint16_t vl_id, size_t len)
{
    vl_sighting_t *e = find(watch, dtn_port, vl_id);

    if (!e) {
        e = add(watch, dtn_port, vl_id, false);
        if (!e)
            return;
        log_line("first sight: VL %u on DTN port %u (%zu bytes)%s",
                 vl_id, dtn_port, len, " - not in the profile");
    } else if (e->packets == 0) {
        e->first_ms = hm_now_ms();
        log_line("first sight: VL %u on DTN port %u (%zu bytes)", vl_id, dtn_port, len);
    }

    e->packets++;
    e->bytes  += len;
    e->last_ms = hm_now_ms();
    if (!e->first_ms)
        e->first_ms = e->last_ms;

    for (uint8_t i = 0; i < e->size_count; i++)
        if (e->sizes[i] == (uint16_t)len)
            return;
    if (e->size_count < VL_WATCH_MAX_SIZES)
        e->sizes[e->size_count++] = (uint16_t)len;
}

void vl_watch_unclassified(vl_watch_t *watch)
{
    watch->unclassified++;
}

size_t vl_watch_expected_seen(const vl_watch_t *watch, size_t *expected_total)
{
    size_t seen = 0, total = 0;

    for (size_t i = 0; i < watch->count; i++)
        if (watch->entries[i].expected) {
            total++;
            if (watch->entries[i].packets)
                seen++;
        }
    if (expected_total)
        *expected_total = total;
    return seen;
}

static void format_sizes(const vl_sighting_t *e, char *out, size_t cap)
{
    size_t used = 0;

    out[0] = '\0';
    for (uint8_t i = 0; i < e->size_count && used + 8 < cap; i++)
        used += (size_t)snprintf(out + used, cap - used, "%s%u",
                                 i ? "," : "", e->sizes[i]);
    if (!e->size_count)
        snprintf(out, cap, "-");
}

void vl_watch_render(const vl_watch_t *watch, uint64_t elapsed_s,
                     const char *profile_name, bool unit_alive,
                     unsigned interruptions)
{
    size_t total_expected, seen = vl_watch_expected_seen(watch, &total_expected);
    uint64_t now = hm_now_ms();

    printf("\033[H\033[2J");
    printf("Acceleration Test - DTN - %s        %llus elapsed\n",
           profile_name, (unsigned long long)elapsed_s);
    printf("unit %s   expected VLs seen %zu/%zu   power interruptions %u\n\n",
           unit_alive ? "ALIVE" : "LOST ", seen, total_expected, interruptions);

    printf("  link       DTN  VL-ID   packets      bytes   last   sizes           status\n");
    printf("  ---------- ---  -----  --------  ---------  -----   --------------  ------\n");

    for (size_t i = 0; i < watch->count; i++) {
        const vl_sighting_t *e = &watch->entries[i];
        const char *iface = app_config_iface_for_port(e->dtn_port);
        char sizes[32];
        char last[16];

        format_sizes(e, sizes, sizeof sizes);
        if (e->packets)
            snprintf(last, sizeof last, "%4.1fs", (double)(now - e->last_ms) / 1000.0);
        else
            snprintf(last, sizeof last, "   -");

        printf("  %-10s %3u  %5u  %8llu  %9llu  %5s   %-14s  %s\n",
               iface ? iface : "?", e->dtn_port, e->vl_id,
               (unsigned long long)e->packets, (unsigned long long)e->bytes,
               last, sizes,
               !e->expected  ? "extra"
               : e->packets  ? "ok"
                             : "MISSING");
    }

    if (watch->unclassified)
        printf("\n  %llu frame(s) arrived that were not from the device\n",
               (unsigned long long)watch->unclassified);
    if (watch->overflow)
        printf("\n  more distinct VLs than the table holds - some are not shown\n");

    printf("\nCtrl+C to end the test\n");
    fflush(stdout);
}

void vl_watch_log_summary(const vl_watch_t *watch)
{
    size_t total_expected, seen = vl_watch_expected_seen(watch, &total_expected);

    log_line("VL summary: %zu of %zu expected VLs seen", seen, total_expected);
    for (size_t i = 0; i < watch->count; i++) {
        const vl_sighting_t *e = &watch->entries[i];
        char sizes[32];

        format_sizes(e, sizes, sizeof sizes);
        log_file_only("  DTN port %2u  VL %5u  %10llu packets  %12llu bytes  "
                      "sizes %-14s %s",
                      e->dtn_port, e->vl_id, (unsigned long long)e->packets,
                      (unsigned long long)e->bytes, sizes,
                      !e->expected ? "extra" : e->packets ? "ok" : "MISSING");
        if (e->expected && !e->packets)
            log_line("  VL %u on DTN port %u never arrived", e->vl_id, e->dtn_port);
    }
}

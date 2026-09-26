#include "CmcStats.h"

#include "CmcPacket.h"
#include "Log.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

void cmc_stats_view_reset(cmc_stats_view_t *view)
{
    memset(view, 0, sizeof *view);
}

void cmc_stats_print_banner(const cmc_config_t *config, bool warmup_complete,
                            unsigned elapsed_s)
{
    printf("╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    if (!warmup_complete)
        printf("║                                                              CMC PORT STATS - WARM-UP (%3u/%3u sec)                                                                                                                                  ║\n",
               elapsed_s, config->warmup_s);
    else
        printf("║                                                              CMC PORT STATS - TEST Duration: %5u sec                                                                                                                                 ║\n",
               elapsed_s);
    printf("╚══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝\n");
}

static void print_head(void)
{
    printf("  ┌─────────┬────────┬─────────────────────────────────────────────────────────────────────┬─────────────────────────────────────────────────────────────────────┬──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐\n");
    printf("  │ Server  │  CMC   │                          CMC TX (CMC→Server)                        │                          CMC RX (Server→CMC)                        │                                                  Payload Verification                                                                                  │\n");
    printf("  │  Port   │  Port  ├─────────────────────┬─────────────────────┬─────────────────────────┼─────────────────────┬─────────────────────┬─────────────────────────┼─────────────────────┬─────────────────────┬─────────────────────┬─────────────────────┬─────────────────────┬─────────────────────┬─────────────────────┬─────────────┤\n");
    printf("  │         │        │       Packets       │        Bytes        │          Gbps           │       Packets       │        Bytes        │          Gbps           │        Good         │         Bad         │  SplitMix64 Fail    │    CRC32 Fail       │      XOR Fail       │        Loss         │      Bit Error      │     BER     │\n");
    printf("  ├─────────┼────────┼─────────────────────┼─────────────────────┼─────────────────────────┼─────────────────────┼─────────────────────┼─────────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼─────────────┤\n");
}

static void print_foot(void)
{
    printf("  └─────────┴────────┴─────────────────────┴─────────────────────┴─────────────────────────┴─────────────────────┴─────────────────────┴─────────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴─────────────┘\n");
}

static double to_gbps(uint64_t bytes)
{
    return (bytes * 8.0) / 1e9;
}

/* A lost packet is every bit of a packet gone, so it counts as a frame's worth
 * of bit errors and its bits go into the total the BER is over as well - the
 * reference's arithmetic, kept because the number has to mean the same thing on
 * both. */
static uint64_t lost_bits_of(uint64_t lost, const cmc_config_t *config)
{
    return lost * (uint64_t)CMC_FRAME_LEN(config->vlan_tagged) * 8;
}

void cmc_stats_print_net(cmc_stats_view_t *view, const cmc_data_plane_t *dp,
                         const cmc_config_t *config, uint8_t net)
{
    const cmc_net_stats_t *st = cmc_data_plane_stats(dp, net);

    if (!st)
        return;

    /* CMC TX (CMC→Server) is what arrived here; CMC RX (Server→CMC) is what we
     * sent. The names are the CMC's point of view, which is the reference's. */
    const uint64_t cmc_tx_pkts  = st->frames;
    const uint64_t cmc_tx_bytes = st->frame_bytes;
    const uint64_t cmc_rx_pkts  = st->tx_pkts;
    const uint64_t cmc_rx_bytes = st->tx_bytes;

    const double tx_gbps = to_gbps(cmc_tx_bytes - view->prev_rx_bytes[net]);
    const double rx_gbps = to_gbps(cmc_rx_bytes - view->prev_tx_bytes[net]);

    view->prev_rx_bytes[net] = cmc_tx_bytes;
    view->prev_tx_bytes[net] = cmc_rx_bytes;

    const uint64_t lost_bits  = lost_bits_of(st->lost, config);
    const uint64_t bit_errors = st->bit_errors + lost_bits;
    const uint64_t total_bits = cmc_tx_bytes * 8 + lost_bits;
    const double   ber = total_bits ? (double)bit_errors / (double)total_bits : 0.0;

    printf("  │ %-7s │ %-6s │ %19" PRIu64 " │ %19" PRIu64 " │ %23.4f │ %19" PRIu64
           " │ %19" PRIu64 " │ %23.4f │ %19" PRIu64 " │ %19" PRIu64 " │ %19" PRIu64
           " │ %19" PRIu64 " │ %19" PRIu64 " │ %19" PRIu64 " │ %19" PRIu64
           " │ %11.2e │\n",
           config->nets[net].iface, config->nets[net].label,
           cmc_tx_pkts, cmc_tx_bytes, tx_gbps,
           cmc_rx_pkts, cmc_rx_bytes, rx_gbps,
           st->good, st->bad, st->splitmix_fail, st->crc32_fail, st->xor_fail,
           st->lost, bit_errors, ber);
}

void cmc_stats_print_all(cmc_stats_view_t *view, const cmc_data_plane_t *dp,
                         const cmc_config_t *config)
{
    for (uint8_t n = 0; n < config->net_count; n++) {
        const cmc_net_link_t *link = &config->nets[n];

        printf("\n  === %s (%s | %s | VL %u..%u -> %u..%u | SRC MAC tail 0x%02X) ===\n",
               link->label, link->unit_label, link->iface,
               config->tx_vl_start, config->tx_vl_start + config->vl_count - 1,
               config->rx_vl_start, config->rx_vl_start + config->vl_count - 1,
               link->src_mac_tail);
        print_head();
        cmc_stats_print_net(view, dp, config, n);
        print_foot();
    }
}

void cmc_stats_print_warnings(const cmc_data_plane_t *dp, const cmc_config_t *config)
{
    bool has_warning = false;

    for (uint8_t n = 0; n < config->net_count; n++) {
        const cmc_net_stats_t *st = cmc_data_plane_stats(dp, n);
        const char *label = config->nets[n].label;

        if (!st)
            continue;
        if (st->bad == 0 && st->bit_errors == 0 && st->lost == 0 &&
            st->tx_refused == 0 && st->other_vl == 0)
            continue;

        if (!has_warning) {
            printf("\n  WARNINGS:\n");
            has_warning = true;
        }
        if (st->bad > 0)
            printf("      %s: %" PRIu64 " bad packets! (SM:%" PRIu64 " CRC:%" PRIu64
                   " XOR:%" PRIu64 ")\n",
                   label, st->bad, st->splitmix_fail, st->crc32_fail, st->xor_fail);
        if (st->bit_errors > 0)
            printf("      %s: %" PRIu64 " bit errors!\n", label, st->bit_errors);
        if (st->lost > 0)
            printf("      %s: %" PRIu64 " lost packets!\n", label, st->lost);

        /* Ours, not the reference's: the kernel is in the send path here, so a
         * frame it would not take is a thing that can happen and is not the
         * unit's fault. It has to be visible or it would read as loss. */
        if (st->tx_refused > 0)
            printf("      %s: %" PRIu64 " frame(s) the link would not take - "
                   "that is this end, not the unit\n", label, st->tx_refused);
        if (st->other_vl > 0)
            printf("      %s: %" PRIu64 " frame(s) on an unexpected VL id "
                   "(last was %u)\n", label, st->other_vl, st->last_other_vl);
    }
}

void cmc_stats_log_net(const cmc_data_plane_t *dp, const cmc_config_t *config,
                       uint8_t net)
{
    const cmc_net_stats_t *st = cmc_data_plane_stats(dp, net);

    if (!st)
        return;

    const cmc_net_link_t *link = &config->nets[net];

    log_line("  %s (%s on %s): sent %" PRIu64 " frame(s) / %" PRIu64 " byte(s), "
             "received %" PRIu64 " / %" PRIu64,
             link->label, link->unit_label, link->iface,
             st->tx_pkts, st->tx_bytes, st->frames, st->frame_bytes);
    log_line("    verified %" PRIu64 ", good %" PRIu64 ", bad %" PRIu64
             " (SplitMix %" PRIu64 ", CRC %" PRIu64 ", XOR %" PRIu64 ")",
             st->total_rx_pkts, st->good, st->bad,
             st->splitmix_fail, st->crc32_fail, st->xor_fail);
    log_line("    lost %" PRIu64 ", bit errors %" PRIu64 ", short %" PRIu64
             ", health monitor %" PRIu64 ", MMMS %" PRIu64,
             st->lost, st->bit_errors, st->short_pkts,
             st->health_frames, st->mmms_frames);
    if (st->tx_refused)
        log_line("    %" PRIu64 " frame(s) the link refused", st->tx_refused);
    if (st->other_vl)
        log_line("    %" PRIu64 " frame(s) on an unexpected VL id, last %u",
                 st->other_vl, st->last_other_vl);
}

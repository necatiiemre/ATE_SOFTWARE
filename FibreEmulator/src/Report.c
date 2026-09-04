#include "Report.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

void report_init(report_t *report, const scenario_t *scenario,
                 const scenario_flow_t *flows, size_t count)
{
    memset(report, 0, sizeof *report);
    report->scenario   = scenario;
    report->flows      = flows;
    report->flow_count = count;
}

void report_sent(report_t *report, size_t flow_index)
{
    if (flow_index < report->flow_count)
        report->results[flow_index].sent++;
}

uint64_t report_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)(ts.tv_nsec / 1000000L);
}

void report_frame_seen(report_t *report, uint8_t server_port)
{
    if (server_port < 8)
        report->rx.frames[server_port]++;
}

void report_foreign(report_t *report)
{
    report->rx.foreign++;
}

bool report_received(report_t *report, uint16_t vl_id, uint16_t vlan)
{
    report->rx.ours++;

    for (size_t i = 0; i < report->flow_count; i++) {
        if (report->flows[i].vl_id != vl_id || !report->flows[i].expect_return)
            continue;

        /* It came back, which is the thing being tested. */
        report->results[i].received++;
        report->results[i].last_ms = report_now_ms();

        if (report->flows[i].rx_vlan != vlan) {
            report->results[i].wrong_vlan++;
            report->rx.wrong_vlan++;
        }
        return true;
    }
    report->rx.unmatched++;
    return false;
}

bool report_all_links_up(const report_t *report)
{
    for (size_t i = 0; i < report->flow_count; i++)
        if (report->flows[i].expect_return && report->results[i].received == 0)
            return false;
    return true;
}

/* Collapse the flows back into the links they came from, so a table has one row
 * per fibre link rather than one per VL. */
typedef struct {
    const scenario_flow_t *first;
    uint16_t vls;
    uint64_t sent, received, wrong, last_ms;
} link_rollup_t;

/**
 * Advance @p cursor to the next fibre link and total up its VLs.
 * Returns false once there are none left.
 */
static bool next_link(const report_t *report, size_t *cursor, link_rollup_t *out)
{
    size_t i = *cursor;

    while (i < report->flow_count && !report->flows[i].expect_return)
        i++;
    if (i >= report->flow_count)
        return false;

    const scenario_flow_t *first = &report->flows[i];
    memset(out, 0, sizeof *out);
    out->first = first;

    while (i < report->flow_count &&
           report->flows[i].expect_return &&
           report->flows[i].src_dtn_port == first->src_dtn_port &&
           report->flows[i].dst_dtn_port == first->dst_dtn_port) {
        out->sent     += report->results[i].sent;
        out->received += report->results[i].received;
        out->wrong    += report->results[i].wrong_vlan;
        if (report->results[i].last_ms > out->last_ms)
            out->last_ms = report->results[i].last_ms;
        out->vls++;
        i++;
    }
    *cursor = i;
    return true;
}

static void count_links(const report_t *report, size_t *up, size_t *total)
{
    link_rollup_t link;
    size_t cursor = 0;

    *up = *total = 0;
    while (next_link(report, &cursor, &link)) {
        (*total)++;
        if (link.received)
            (*up)++;
    }
}

void report_render_live(const report_t *report, uint64_t elapsed_s, uint64_t cycles)
{
    uint64_t now = report_now_ms();
    link_rollup_t link;
    size_t cursor = 0, up, total;

    count_links(report, &up, &total);

    printf("\033[H\033[2J");
    printf("Fibre Emulator - %s        %llus elapsed, %llu cycles\n",
           report->scenario->name, (unsigned long long)elapsed_s,
           (unsigned long long)cycles);
    printf("links returning traffic: %zu/%zu\n\n", up, total);

    printf("  DTN link      VLAN in/out    sent   returned   loss   last\n");
    printf("  ------------  -----------  ------  ---------  -----  -----\n");

    while (next_link(report, &cursor, &link)) {
        double loss = link.sent
                    ? 100.0 * (double)(link.sent - link.received) / (double)link.sent : 0.0;
        char last[16];

        if (link.received)
            snprintf(last, sizeof last, "%4.1fs", (double)(now - link.last_ms) / 1000.0);
        else
            snprintf(last, sizeof last, "    -");

        printf("  port %2u -> %2u   %3u / %3u  %6llu  %9llu  %4.0f%%  %s%s\n",
               link.first->src_dtn_port, link.first->dst_dtn_port,
               link.first->tx_vlan, link.first->rx_vlan,
               (unsigned long long)link.sent, (unsigned long long)link.received,
               loss, last, link.received ? "" : "   NOTHING BACK");
    }

    printf("\n  received: ");
    for (uint8_t p = 0; p < 8; p++)
        if (report->rx.frames[p])
            printf("port %u: %llu  ", p, (unsigned long long)report->rx.frames[p]);
    if (!report->rx.ours && !report->rx.foreign)
        printf("nothing at all");
    printf("\n  of those: %llu ours, %llu foreign, %llu unmatched, %llu wrong VLAN\n",
           (unsigned long long)report->rx.ours,
           (unsigned long long)report->rx.foreign,
           (unsigned long long)report->rx.unmatched,
           (unsigned long long)report->rx.wrong_vlan);
    printf("\nCtrl+C to stop\n");
    fflush(stdout);
}

void report_render(const report_t *report)
{
    link_rollup_t link;
    size_t cursor = 0;

    printf("\n  fibre links\n");
    printf("    DTN link      VLAN in/out   VLs   sent   returned   status\n");
    printf("    ------------  -----------  ----  -----  ---------   ------\n");

    while (next_link(report, &cursor, &link)) {
        const char *status = link.received == 0        ? "NOTHING BACK"
                           : link.received < link.sent ? "partial"
                                                       : "ok";
        printf("    port %2u -> %2u   %3u / %3u   %4u  %5llu  %9llu   %s",
               link.first->src_dtn_port, link.first->dst_dtn_port,
               link.first->tx_vlan, link.first->rx_vlan, link.vls,
               (unsigned long long)link.sent, (unsigned long long)link.received, status);
        if (link.wrong)
            printf(" (%llu on the wrong VLAN)", (unsigned long long)link.wrong);
        putchar('\n');
    }

    bool titled = false;
    for (size_t k = 0; k < report->flow_count; k++) {
        if (report->flows[k].expect_return)
            continue;
        if (!titled) {
            printf("\n  taps to copper - confirmed on the workstation, not here\n");
            printf("    DTN link      VLAN out   sent\n");
            printf("    ------------  --------  -----\n");
            titled = true;
        }
        printf("    port %2u -> %2u   %8u  %5llu\n",
               report->flows[k].src_dtn_port, report->flows[k].dst_dtn_port,
               report->flows[k].tx_vlan,
               (unsigned long long)report->results[k].sent);
    }

    printf("\n  receive path\n");
    for (uint8_t p = 0; p < 8; p++)
        if (report->rx.frames[p])
            printf("    server port %u: %llu frame(s)\n", p,
                   (unsigned long long)report->rx.frames[p]);
    if (!report->rx.ours && !report->rx.foreign)
        puts("    nothing arrived on any receive port");
    printf("    %llu ours, %llu foreign, %llu unmatched, %llu on an unexpected VLAN\n",
           (unsigned long long)report->rx.ours,
           (unsigned long long)report->rx.foreign,
           (unsigned long long)report->rx.unmatched,
           (unsigned long long)report->rx.wrong_vlan);
}

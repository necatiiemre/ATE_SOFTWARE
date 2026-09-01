#include "Report.h"

#include <stdio.h>
#include <string.h>

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

bool report_received(report_t *report, uint16_t vl_id, uint16_t vlan)
{
    for (size_t i = 0; i < report->flow_count; i++) {
        if (report->flows[i].vl_id != vl_id || !report->flows[i].expect_return)
            continue;
        if (report->flows[i].rx_vlan == vlan)
            report->results[i].received++;
        else
            report->results[i].wrong_vlan++;
        return true;
    }
    report->unrecognised++;
    return false;
}

bool report_all_links_up(const report_t *report)
{
    for (size_t i = 0; i < report->flow_count; i++)
        if (report->flows[i].expect_return && report->results[i].received == 0)
            return false;
    return true;
}

/* Collapse the flows back into the links they came from, so the table has one
 * row per fibre link rather than one per VL. */
void report_render(const report_t *report)
{
    printf("\n  fibre links\n");
    printf("    DTN link      VLAN in/out   VLs   sent   returned   status\n");
    printf("    ------------  -----------  ----  -----  ---------   ------\n");

    size_t i = 0;
    while (i < report->flow_count) {
        const scenario_flow_t *first = &report->flows[i];
        if (!first->expect_return) {
            i++;
            continue;
        }
        uint64_t sent = 0, received = 0, wrong = 0;
        uint16_t vls = 0;
        size_t j = i;

        while (j < report->flow_count &&
               report->flows[j].expect_return &&
               report->flows[j].src_dtn_port == first->src_dtn_port &&
               report->flows[j].dst_dtn_port == first->dst_dtn_port) {
            sent     += report->results[j].sent;
            received += report->results[j].received;
            wrong    += report->results[j].wrong_vlan;
            vls++;
            j++;
        }

        const char *status = received == 0     ? "NOTHING BACK"
                           : received < sent   ? "partial"
                                               : "ok";
        printf("    port %2u -> %2u   %3u / %3u   %4u  %5llu  %9llu   %s",
               first->src_dtn_port, first->dst_dtn_port,
               first->tx_vlan, first->rx_vlan, vls,
               (unsigned long long)sent, (unsigned long long)received, status);
        if (wrong)
            printf(" (%llu on the wrong VLAN)", (unsigned long long)wrong);
        putchar('\n');
        i = j;
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

    if (report->unrecognised)
        printf("\n  %llu frame(s) came back that were not ours\n",
               (unsigned long long)report->unrecognised);
}

/*
 * The printers run.
 *
 * VmcPrint.c is dpdk_vmc's print section copied verbatim, so what it prints is
 * not this project's to decide and there is nothing here to assert about the
 * text. What is worth holding is that every one of the six runs over a filled
 * report without walking off the end of a bitfield table or a port array - and
 * that the output is not empty, which a printer quietly reduced to a stub would
 * be.
 *
 * The output goes to a file so the harness can measure it rather than fill the
 * terminal; run the application to read it.
 */

#include "VmcHealth.h"
#include "VmcPrint.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures;

static void check(bool ok, const char *what)
{
    if (!ok) {
        printf("[FAIL] %s\n", what);
        failures++;
    }
}

/* Fill a report with something that is not zero, so a printer that reads past
 * its own struct is reading a pattern rather than a field of zeros. */
static void fill(void *data, size_t len)
{
    uint8_t *p = data;

    for (size_t i = 0; i < len; i++)
        p[i] = (uint8_t)(0x40 + (i % 0x30));
}

static long run_printer(const char *name, void (*body)(void))
{
    char path[] = "/tmp/vmcprintXXXXXX";
    int fd = mkstemp(path);
    long size;

    if (fd < 0) {
        printf("[FAIL] %s: cannot open a temporary file\n", name);
        failures++;
        return -1;
    }

    fflush(stdout);
    int saved = dup(STDOUT_FILENO);
    dup2(fd, STDOUT_FILENO);
    body();
    fflush(stdout);
    dup2(saved, STDOUT_FILENO);
    close(saved);

    size = lseek(fd, 0, SEEK_END);
    close(fd);
    unlink(path);
    return size;
}

static vmc_pbit_data_t              g_pbit;
static bm_engineering_cbit_report_t g_bm_eng;
static bm_flag_cbit_report_t        g_bm_flag;
static dtn_es_cbit_report_t         g_es;
static dtn_sw_cbit_report_t         g_sw;
static Pcs_profile_stats            g_cpu;
static REPORT_MSG                   g_counters;

static void body_pbit(void)    { print_vmc_pbit_report(&g_pbit, "VS"); }
static void body_bm_eng(void)  { print_bm_cbit_report(&g_bm_eng, "BM ENGINEERING CBIT REPORT", "VS"); }
static void body_bm_flag(void) { print_bm_flag_cbit_report(&g_bm_flag, "FLCS"); }
static void body_es(void)      { print_dtn_es_cbit_report(&g_es, "VS"); }
static void body_sw(void)      { print_dtn_sw_cbit_report(&g_sw, "FLCS"); }
static void body_cpu(void)     { print_pcs_profile_stats(&g_cpu, "VS"); }
static void body_counters(void){ print_phy_counter_report(&g_counters, "FLCS"); }

/* Every printer in the original starts by refusing a null report. */
static void body_nulls(void)
{
    print_vmc_pbit_report(NULL, "VS");
    print_bm_flag_cbit_report(NULL, "VS");
    print_dtn_es_cbit_report(NULL, "VS");
    print_dtn_sw_cbit_report(NULL, "VS");
    print_pcs_profile_stats(NULL, "VS");
    print_phy_counter_report(NULL, "VS");
}

int main(void)
{
    static const struct {
        const char *name;
        void (*body)(void);
    } printers[] = {
        {"PBIT report",         body_pbit},
        {"BM engineering CBIT", body_bm_eng},
        {"BM flag CBIT",        body_bm_flag},
        {"DTN end system CBIT", body_es},
        {"DTN switch CBIT",     body_sw},
        {"CPU usage",           body_cpu},
        {"PHY port counters",   body_counters},
    };

    fill(&g_pbit,    sizeof g_pbit);
    fill(&g_bm_eng,  sizeof g_bm_eng);
    fill(&g_bm_flag, sizeof g_bm_flag);
    fill(&g_es,      sizeof g_es);
    fill(&g_sw,      sizeof g_sw);
    fill(&g_cpu,     sizeof g_cpu);
    fill(&g_counters, sizeof g_counters);

    /* Eight ports, numbered, so the switch printer walks a plausible array. */
    for (int i = 0; i < 8; i++)
        g_sw.dtn_sw_monitoring_st.port[i].A664_SW_PORT_ID = (uint16_t)i;
    g_sw.dtn_sw_monitoring_st.status.A664_SW_PORT_COUNT = 8;
    g_pbit.number_of_policy_step = 80;

    for (size_t i = 0; i < sizeof printers / sizeof printers[0]; i++) {
        long size = run_printer(printers[i].name, printers[i].body);

        if (size < 0)
            continue;
        check(size > 100, printers[i].name);
        printf("[ OK ] %-21s %ld bytes\n", printers[i].name, size);
    }

    check(run_printer("null guard", body_nulls) == 0,
          "a null report prints nothing rather than crashing");

    if (failures) {
        printf("FAILED: %d check(s)\n", failures);
        return 1;
    }
    puts("PASS: every printer runs over a filled report");
    return 0;
}

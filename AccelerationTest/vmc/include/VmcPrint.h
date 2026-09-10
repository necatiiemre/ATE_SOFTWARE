/**
 * @file VmcPrint.h
 * @brief Printing a VMC health-monitor report exactly as dpdk_vmc prints it.
 *
 * The bodies in VmcPrint.c are that project's print functions copied verbatim,
 * so a report shown here and the same report shown by dpdk_vmc are the same
 * text. Anything this application wants to say about a run goes around them,
 * never inside them.
 */

#ifndef VMC_PRINT_H
#define VMC_PRINT_H

#include "VmcMessages.h"

#include <stdbool.h>

/* From dpdk_vmc/include/health_monitor/health_monitor.h, verbatim: the limits
 * the temperature check applies. */

// ============================================================================
// Sıcaklık kabul aralığı (°C)
// ----------------------------------------------------------------------------
// HM raporlarında gelen herhangi bir sıcaklık değeri bu aralığın ALTINA düşer
// (< -50 °C) ya da ÜSTÜNE çıkarsa (> +120 °C) ölümcül arıza sayılır: dashboard
// hangi alanın neden fail ettiğini yazan bir FAIL banner'ı basar ve testi
// durdurur (hm_set_abort_flag ile verilen bayrağı set eder).
// ============================================================================
#define HM_TEMP_MIN_DEGC  (-50000.0f)
#define HM_TEMP_MAX_DEGC  (50000.0f)

// Aralık dışı sıcaklık tek seferde değil, PEŞ PEŞE bu kadar ölçümde görülürse
// test durdurulur. Dashboard 1 Hz çalıştığı için bu ~bu kadar saniyeye denktir.
// Arada bir kez bile aralık içine dönen değer o sensörün sayacını sıfırlar.
#define HM_TEMP_FAIL_STREAK_LIMIT  10

/**
 * @brief Point the temperature check at a flag to raise when a limit is broken.
 *
 * dpdk_vmc gives this its global quit flag: a temperature outside
 * HM_TEMP_MIN_DEGC..HM_TEMP_MAX_DEGC on ten consecutive reports prints a banner
 * and stops the test. Copied along with everything else so the behaviour
 * matches, not only the text.
 */
void hm_set_abort_flag(volatile bool *flag);

void print_vmc_pbit_report     (const vmc_pbit_data_t *data, const char *device_name);
void print_bm_cbit_report      (const bm_engineering_cbit_report_t *data,
                                const char *report_title, const char *device_name);
void print_bm_flag_cbit_report (const bm_flag_cbit_report_t *data, const char *device_name);
void print_dtn_es_cbit_report  (const dtn_es_cbit_report_t *data, const char *device_name);
void print_dtn_sw_cbit_report  (const dtn_sw_cbit_report_t *data, const char *device_name);
void print_pcs_profile_stats   (const Pcs_profile_stats *data, const char *device_name);

/* The temperature checks the dashboard runs after printing each report. */
void hm_check_bm_engineering_temps(const bm_engineering_cbit_report_t *d, const char *device);
void hm_check_dtn_es_temps        (const dtn_es_cbit_report_t *d, const char *device);
void hm_check_dtn_sw_temps        (const dtn_sw_cbit_report_t *d, const char *device);

/** True once a temperature limit has been broken; the dashboard says so. */
bool hm_temperature_failed(void);

#endif /* VMC_PRINT_H */

#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include <vmc_message_types.h>

// NOT: Health Monitor trafiği ayrı bir interface değil, normal DPDK data-plane
// port'ları üzerinden taşınır. Tüm HM paketleri (CPU usage, CBIT, PBIT) DUT
// tarafından otonom olarak yollanır; rx_worker fast-path'inde VL-ID aralığı
// kontrolüyle hm_handle_packet()'e düşer.

// ============================================================================
// VL-ID tanımları
// ============================================================================
#define HEALTH_MONITOR_FLCS_CPU_USAGE_VLID      0x0009
#define HEALTH_MONITOR_VS_CPU_USAGE_VLID        0x0010

#define HEALTH_MONITOR_FLCS_PBIT_REQUEST_VLID   0x000c
#define HEALTH_MONITOR_VS_PBIT_REQUEST_VLID     0x000f

#define HEALTH_MONITOR_FLCS_PBIT_RESPONSE_VLID  0x000a
#define HEALTH_MONITOR_VS_PBIT_RESPONSE_VLID    0x000d

#define HEALTH_MONITOR_FLCS_CBIT_VLID           0x000b
#define HEALTH_MONITOR_VS_CBIT_VLID             0x000e

// HM VL-ID aralığı (rx_worker erken-dallanmasında aralık kontrolü için).
// Bu aralığa düşen paketler PRBS doğrulamasına girmez.
#define HM_VL_ID_MIN 0x0009
#define HM_VL_ID_MAX 0x0010

// CBIT paketleri içindeki vmp_cmsw_header_t.message_identifier değerleri.
// (VL-ID 11 / 14 aynı anda 4 farklı struct taşıyabiliyor; ayrım msg_id ile.)
#define HM_CBIT_MSG_ID_DTN_ES         2
#define HM_CBIT_MSG_ID_DTN_SW         3
#define HM_CBIT_MSG_ID_BM_ENGINEERING 5
#define HM_CBIT_MSG_ID_BM_FLAG        6

// PBIT response paketleri (VL-ID 0x0a/0x0d) içindeki msg_id. Range check
// (0x0009-0x0010) tek başına yetmez; aynı VL-ID'ye farklı trafik gelebildiği
// için hm_handle_packet içinde msg_id doğrulaması yapılır.
#define HM_PBIT_RESPONSE_MSG_ID       100

static inline bool hm_is_health_monitor_vl_id(uint16_t vl_id)
{
    return (vl_id >= HM_VL_ID_MIN && vl_id <= HM_VL_ID_MAX);
}

// Dashboard basım aralığı (ms)
#define HEALTH_MONITOR_DASHBOARD_INTERVAL_MS 1000

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

// Health monitor'ü global kapatma bayrağına bağla; aralık dışı bir sıcaklık
// testi durdurabilsin diye main loop'tan önce &force_quit geçilir.
void hm_set_abort_flag(volatile bool *flag);

// ============================================================================
// RX fast-path entry point: rx_worker içinden çağrılır.
// payload  = UDP payload başı
// len      = UDP payload uzunluğu (>= beklenen struct boyutu olmalı)
// ============================================================================
void hm_handle_packet(uint16_t vl_id, const uint8_t *payload, uint16_t len);

// ============================================================================
// Dashboard — main thread içinden çağrılır.
// hm_print_dashboard() her çağrıldığında şu anki en güncel snapshot'ı basar
// (tüm HM slot'larını sırayla). Stats tablosundan sonra çağrılarak sıralı
// çıktı elde edilir.
// ============================================================================
void hm_print_dashboard(void);

// ============================================================================
// Preload the FLCS/VS PBIT report slots from snapshot files written by the
// pre-flight test_starter (which performs the PBIT request/response handshake
// before DPDK takes the NIC). PBIT is a one-shot power-on result, so the
// snapshot is shown as-is in the dashboard. Missing files are ignored.
// Files: /tmp/vmc_pbit_flcs.bin, /tmp/vmc_pbit_vs.bin (raw 454-byte payloads).
// ============================================================================
void hm_preload_pbit_snapshots(void);

// ============================================================================
// Print fonksiyonları (printer thread ya da debug amaçlı dışarıdan çağrılır)
// ============================================================================
void print_vmc_pbit_report     (const vmc_pbit_data_t *data,           const char *device_name);
void print_bm_cbit_report      (const bm_engineering_cbit_report_t *data, const char *report_title, const char *device_name);
void print_bm_flag_cbit_report (const bm_flag_cbit_report_t *data,     const char *device_name);
void print_dtn_es_cbit_report  (const dtn_es_cbit_report_t *data,      const char *device_name);
void print_dtn_sw_cbit_report  (const dtn_sw_cbit_report_t *data,      const char *device_name);
void print_pcs_profile_stats   (const Pcs_profile_stats *data,         const char *device_name);

#endif /* HEALTH_MONITOR_H */
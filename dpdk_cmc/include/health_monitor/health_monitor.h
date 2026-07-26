#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "cmc_es_monitoring.h"   // tA664ESMonitoring
#include "cmc_sw_monitoring.h"   // tA664SWMonitoring (status + port[12])
#include "cmc_other_types.h"     // Pcs_profile_stats, COUNTERS
#include "cl_cmsw_message_types.h" // Cl_cmsw_status_report_msg_type (VL 18/19)
#include "smmm_message_types.h"    // Smmm_monitoring_data_t (VL 2021)
#include "clcmsw_ipmc_types.h"     // CL CMSW IPMC log board data (VL 50)

// ============================================================================
// CMC Health Monitor
// ----------------------------------------------------------------------------
// HM trafiği ayrı bir interface değil; normal DPDK data-plane portları üzerinde
// taşınır. rx_worker fast-path'inde VL-ID set kontrolü ile yakalanıp
// hm_handle_packet()'e düşer. PRBS doğrulama yolundan çıkar.
//
// VL-ID'ler iki bloğa ayrılır:
//   * 2021, 2042, 2063, 2084, 2105 → ES_MON / PCS / COUNTERS paketleri
//   * 8009, 8109                   → ES_MON / PCS / COUNTERS + ek olarak SW_MON
//
// Aynı VL-ID üzerinden farklı struct'lar farklı paketlerde gelir; tür ayrımı
// payload uzunluğuna göre yapılır.
// ============================================================================

// HM VL-ID set (rx_worker erken-dallanması için)
#define HM_VLID_18 18 // CLCMSW DSM-A
#define HM_VLID_19 19 // CLCMSW DSM-B
#define HM_VLID_31 31 // CLCMSW SMMMM
#define HM_VLID_50 50 // CL CMSW IPMC log (DPM/DSM/GPM temp) — debug/inceleme
#define HM_VLID_2021   2021
#define HM_VLID_2042   2042
#define HM_VLID_2063   2063
#define HM_VLID_2084   2084
#define HM_VLID_2105   2105
#define HM_VLID_8009   8009
#define HM_VLID_8109   8109

// True ⇒ paket HM dispatch'ine girer (PRBS path'inden çıkar).
static inline bool hm_is_health_monitor_vl_id(uint16_t vl_id)
{
    switch (vl_id) {
        case HM_VLID_18:
        case HM_VLID_19:
        case HM_VLID_50:
        case HM_VLID_2021:
        case HM_VLID_2042:
        case HM_VLID_2063:
        case HM_VLID_2084:
        case HM_VLID_2105:
        case HM_VLID_8009:
        case HM_VLID_8109:
            return true;
        default:
            return false;
    }
}

// SW_MON sadece 8009 ve 8109'dan beklenir.
static inline bool hm_vl_id_accepts_sw_mon(uint16_t vl_id)
{
    return (vl_id == HM_VLID_8009 || vl_id == HM_VLID_8109);
}

// Dashboard basım aralığı (ms). Main loop sleep(1) ile uyumlu.
#define HEALTH_MONITOR_DASHBOARD_INTERVAL_MS 1000

// ============================================================================
// Queue item — tagged union. RX worker doldurur, dashboard tüketir.
// ============================================================================
typedef enum {
    HM_ITEM_NONE = 0,
    HM_ITEM_PCS_PROFILE,        // Pcs_profile_stats
    HM_ITEM_COUNTERS_DPM,       // COUNTERS_DPM
    HM_ITEM_COUNTERS_INTER_DPM, // COUNTERS_INTER_DPM
    HM_ITEM_COUNTERS_DSM,       // COUNTERS_DSM
    HM_ITEM_DTN_ES_MONITORING,  // tA664ESMonitoring
    HM_ITEM_DTN_SW_MONITORING,  // tA664SWMonitoring
    HM_ITEM_COUNTERS_DPM_VL,    // COUNTERS_DPM_VL (per-VL RX/TX flow counters)
    HM_ITEM_CLCMSW,             // CL CMSW (DSM-A, DSM-B)
    HM_ITEM_SMMM,               // Smmm_monitoring_data_t (VL 2021)
    HM_ITEM_IPMC,               // CL CMSW IPMC board temp (VL 50)
} hm_item_kind_t;

// CL CMSW IPMC log (VL 50) — TAM board data. Alan sayısı çok (DPM 84, GPM 78,
// DSM 128) olduğu için ham payload'ı taşıyıp parse'ı print tarafında descriptor
// tablosuyla yaparız. component_type ile board tipi ayrılır (56/57/59).
typedef struct {
    uint16_t device_id;
    uint32_t component_type;               // 56=DPM 57=DSM 59=GPM
    uint16_t len;                          // ham payload uzunluğu
    uint8_t  raw[IPMC_RAW_MAX];            // ham payload (big-endian, packed)
} hm_ipmc_raw_t;

typedef struct {
    hm_item_kind_t kind;
    uint16_t       vl_id;
    uint64_t       rx_timestamp_ns;
    union {
        Pcs_profile_stats   pcs;
        COUNTERS_DPM        counters_dpm;
        COUNTERS_INTER_DPM  counters_inter_dpm;
        COUNTERS_DSM        counters_dsm;
        tA664ESMonitoring   es_mon;
        tA664SWMonitoring   sw_mon;   // status + port[12]
        COUNTERS_DPM_VL     counters_dpm_vl; // 104 RX + 104 TX uint32
        Cl_cmsw_status_report_msg_type clcmsw; // CL CMSW status report (662 B)
        Smmm_monitoring_data_t smmm;           // SMMM monitoring (130 B)
        hm_ipmc_raw_t          ipmc;           // IPMC log ham payload (VL 50)
    } payload;
} hm_queue_item_t;

// Ring kapasitesi. ~200 pps'lik HM yükü için fazlasıyla yeterli.
#define HM_RING_CAPACITY 256

// ============================================================================
// RX fast-path entry — rx_worker'dan çağrılır.
//   vl_id    : pakette extract edilmiş VL-ID
//   payload  : UDP payload başı
//   len      : UDP payload uzunluğu (bayt)
// Tür payload uzunluğuna göre belirlenir; bilinmeyen uzunluk → drop + sayaç.
// ============================================================================
void hm_handle_packet(uint16_t vl_id, const uint8_t *payload, uint16_t len);

// ============================================================================
// Dashboard — main thread'den 1 Hz çağrılır. Ring'i drain eder, her item için
// uygun print_* fonksiyonunu çağırır, sonunda tick + diag sayaç satırı basar.
// ============================================================================
void hm_print_dashboard(void);

// ============================================================================
// Print fonksiyonları — health_monitor_cmc.c içinde.
// vl_id: paketin geldiği VL-ID, başlıkta gösterilir.
// packets: bu tick'te aynı (vl_id, kind) çiftinden kaç paket dedup edildi
//          (başlıkta "×N" notu olarak görünür). 1 ise not yazılmaz.
// ============================================================================
void print_pcs_profile_stats   (const Pcs_profile_stats   *data, uint16_t vl_id, unsigned packets);
void print_counters_dpm        (const COUNTERS_DPM        *data, uint16_t vl_id, unsigned packets);
void print_counters_inter_dpm  (const COUNTERS_INTER_DPM  *data, uint16_t vl_id, unsigned packets);
void print_counters_dsm        (const COUNTERS_DSM        *data, uint16_t vl_id, unsigned packets);
void print_counters_dpm_vl     (const COUNTERS_DPM_VL     *data, uint16_t vl_id, unsigned packets);

// ============================================================================
// DPM VL kümülatif biriktirme + Inter-DPM paket kaybı — health_monitor_cmc.c.
//   dpm_vl_accumulate : DPM VL paketi SANİYELİK (delta) değerler taşır;
//                       dashboard her drained paket için çağırır, ilgili
//                       DPM'in kümülatif toplamına ekler. Per-DPM tablo ve
//                       loss hesabı bu kümülatif toplamlar üzerinden çalışır.
//   print_dpm_vl_loss_table : komşu DPM'ler arası TX(sender) - RX(receiver)
//                       kümülatif farkını (loss>0) basar.
// ============================================================================
void dpm_vl_accumulate(uint16_t vl_id, const COUNTERS_DPM_VL *data);
void print_dpm_vl_loss_table(void);
void print_dtn_es_monitoring   (const tA664ESMonitoring   *data, uint16_t vl_id, unsigned packets);
void print_dtn_sw_monitoring   (const tA664SWMonitoring   *data, uint16_t vl_id, unsigned packets);

// ============================================================================
// Sıcaklık özeti — tüm struct'lardaki sıcaklık alanları tek tabloda.
//   Değerler print_dtn_es_monitoring / print_dtn_sw_monitoring çağrıldıkça
//   kaynak (VL-ID) bazında otomatik yakalanır ("en son okunan" semantiği,
//   kalıcı). print_temperature_summary dashboard'un en altında basar.
// ============================================================================
void print_temperature_summary(void);

// ============================================================================
// CL CMSW status report (VL 18/19) — chassis içindeki tüm LRM'lerin durumu.
// Detaylı bölümler halinde basılır (computer, PSM, HUM, SMMM, IOCM, DPM, DSM,
// GPM, HSM). Wire big-endian; multi-byte scalar alanlar swap edilir.
// ============================================================================
void print_clcmsw(const Cl_cmsw_status_report_msg_type *data, uint16_t vl_id, unsigned packets);

// ============================================================================
// SMMM monitoring (VL 2021) — voltage/current/temperature ADC dönüşümlü tablo.
// ============================================================================
void print_smmm(const Smmm_monitoring_data_t *data, uint16_t vl_id, unsigned packets);

// ============================================================================
// CL CMSW IPMC log (VL 50) — DPM/DSM/GPM tam board data.
//   ipmc_temp_store        : bir board'un ham payload'ını device_id bazında
//                            saklar (tek thread: dashboard). Parse print'te.
//   print_ipmc_temperatures: dashboard'un altında per-board tablo basar.
//                            DEBUG_MODE=0 → kimlik + sıcaklık + özet;
//                            DEBUG_MODE=1 → tüm alanlar (pgood/valid/voltage/
//                            current/other) açık.
// ============================================================================
void ipmc_temp_store(const hm_ipmc_raw_t *src);
void print_ipmc_temperatures(void);

#endif /* HEALTH_MONITOR_H */
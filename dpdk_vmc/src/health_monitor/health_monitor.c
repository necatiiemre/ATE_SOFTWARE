#include <health_monitor.h>
#include <vmc_message_types.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "PsuTelemetryReceiver.h"

// ============================================================================
// Slot altyapısı
// ----------------------------------------------------------------------------
// rx_worker fast-path (her port için ayrı lcore) hm_handle_packet() içinden
// slot'a memcpy yapar; printer thread 1 Hz'de slot'u okur ve print eder.
// 200 pps toplam yük için mutex yeterli — spinlock'a gerek yok.
// ============================================================================

typedef struct {
    Pcs_profile_stats data;
    pthread_mutex_t   lock;
    bool              updated;
    uint64_t          update_count;
} hm_pcs_slot_t;

typedef struct {
    vmc_pbit_data_t   data;
    pthread_mutex_t   lock;
    bool              updated;
    uint64_t          update_count;
} hm_pbit_slot_t;

typedef struct {
    bm_engineering_cbit_report_t data;
    pthread_mutex_t              lock;
    bool                         updated;
    uint64_t                     update_count;
} hm_bm_slot_t;

typedef struct {
    bm_flag_cbit_report_t data;
    pthread_mutex_t       lock;
    bool                  updated;
    uint64_t              update_count;
} hm_bm_flag_slot_t;

typedef struct {
    dtn_es_cbit_report_t data;
    pthread_mutex_t      lock;
    bool                 updated;
    uint64_t             update_count;
} hm_dtn_es_slot_t;

typedef struct {
    dtn_sw_cbit_report_t data;
    pthread_mutex_t      lock;
    bool                 updated;
    uint64_t             update_count;
} hm_dtn_sw_slot_t;

// VS tarafı — CPU USAGE + PBIT + 4 CBIT türü
static hm_pcs_slot_t      vs_cpu_usage_slot      = {.lock = PTHREAD_MUTEX_INITIALIZER};
static hm_pbit_slot_t     vs_pbit_slot           = {.lock = PTHREAD_MUTEX_INITIALIZER};
static hm_bm_slot_t       vs_bm_engineering_slot = {.lock = PTHREAD_MUTEX_INITIALIZER};
static hm_bm_flag_slot_t  vs_bm_flag_slot        = {.lock = PTHREAD_MUTEX_INITIALIZER};
static hm_dtn_es_slot_t   vs_dtn_es_slot         = {.lock = PTHREAD_MUTEX_INITIALIZER};
static hm_dtn_sw_slot_t   vs_dtn_sw_slot         = {.lock = PTHREAD_MUTEX_INITIALIZER};

// FLCS tarafı — ayna
static hm_pcs_slot_t      flcs_cpu_usage_slot      = {.lock = PTHREAD_MUTEX_INITIALIZER};
static hm_pbit_slot_t     flcs_pbit_slot           = {.lock = PTHREAD_MUTEX_INITIALIZER};
static hm_bm_slot_t       flcs_bm_engineering_slot = {.lock = PTHREAD_MUTEX_INITIALIZER};
static hm_bm_flag_slot_t  flcs_bm_flag_slot        = {.lock = PTHREAD_MUTEX_INITIALIZER};
static hm_dtn_es_slot_t   flcs_dtn_es_slot         = {.lock = PTHREAD_MUTEX_INITIALIZER};
static hm_dtn_sw_slot_t   flcs_dtn_sw_slot         = {.lock = PTHREAD_MUTEX_INITIALIZER};

// (Önceden printer için pthread state vardı; dashboard artık main thread'de.)

// Debug/diagnostic sayaçları (atomic artırılır, lock gerekmez)
static volatile uint64_t g_hm_rx_total        = 0;  // hm_handle_packet çağrı sayısı
static volatile uint64_t g_hm_rx_vs_cpu       = 0;
static volatile uint64_t g_hm_rx_flcs_cpu     = 0;
static volatile uint64_t g_hm_rx_vs_pbit      = 0;  // VL-ID 0x0d
static volatile uint64_t g_hm_rx_flcs_pbit    = 0;  // VL-ID 0x0a
static volatile uint64_t g_hm_rx_cbit         = 0;  // VL-ID 11/14 toplam
static volatile uint64_t g_hm_rx_unknown_vlid = 0;
static volatile uint64_t g_hm_rx_unknown_msg  = 0;  // CBIT içinde bilinmeyen msg_id
static volatile uint64_t g_hm_rx_short        = 0;
static volatile uint64_t g_hm_rx_empty        = 0;  // boş CBIT paketleri (tx/rx total = 0)

// Aralık dışı sıcaklık görülünce set edilecek global kapatma bayrağı.
// main, döngüden önce hm_set_abort_flag(&force_quit) ile bağlar.
static volatile bool    *g_hm_abort_flag      = NULL;
// Aynı fail mesajının her tick tekrar basılmaması için: bir kez tetiklendi mi?
static bool              g_hm_temp_failed      = false;

void hm_set_abort_flag(volatile bool *flag)
{
    g_hm_abort_flag = flag;
}

// CBIT paketlerinde "boşluk" kontrolü — traffic counter'ları tamamen sıfırsa
// paket VMC tarafının henüz veri toplamadığı bir döngüde gönderildiğini
// gösterir. Bu paketlerle slot'u güncellemiyoruz; dolu paketleri
// ezmemeleri için updated bayrağı set edilmez.
static inline bool dtn_sw_is_empty(const dtn_sw_cbit_report_t *d)
{
    const dtn_sw_status_mon_t *s = &d->dtn_sw_monitoring_st.status;
    return (s->A664_SW_TX_TOTAL_COUNT == 0 && s->A664_SW_RX_TOTAL_COUNT == 0);
}

static inline bool dtn_es_is_empty(const dtn_es_cbit_report_t *d)
{
    const dtn_es_monitoring_t *m = &d->dtn_es_monitoring_st;
    return (m->A664_ES_TX_INCOMING_COUNT   == 0 &&
            m->A664_ES_RX_A_INCOMING_COUNT == 0 &&
            m->A664_ES_RX_B_INCOMING_COUNT == 0);
}

// ============================================================================
// BE→host endian yardımcıları
// ============================================================================
static inline uint64_t be64_to_host(uint64_t v) { return __builtin_bswap64(v); }
static inline uint32_t be32_to_host(uint32_t v) { return __builtin_bswap32(v); }
static inline uint16_t be16_to_host(uint16_t v) { return __builtin_bswap16(v); }

// Pcs_monitor_type içindeki tek uint64 alan
static inline void swap_monitor(Pcs_monitor_type *m)
{
    m->usage = be64_to_host(m->usage);
}

// Pcs_mem_profile_type içindeki 3 uint64 alan
static inline void swap_mem(Pcs_mem_profile_type *m)
{
    m->total_size    = be64_to_host(m->total_size);
    m->used_size     = be64_to_host(m->used_size);
    m->max_used_size = be64_to_host(m->max_used_size);
}

// Float dizisini yerinde BE→host swap et (IEEE-754 4-byte değerler).
static inline void bswap_float_array(float *arr, size_t count)
{
    uint32_t *u = (uint32_t *)arr;
    for (size_t i = 0; i < count; i++) {
        u[i] = __builtin_bswap32(u[i]);
    }
}

// vmp_cmsw_header_t alanlarını (msg_len, timestamp) swap et.
static inline void swap_vmp_header(vmp_cmsw_header_t *h)
{
    h->message_len = be16_to_host(h->message_len);
    h->timestamp   = be64_to_host(h->timestamp);
}

// ============================================================================
// VS CPU USAGE parse
// ============================================================================
static void parse_pcs_profile_stats(Pcs_profile_stats *dst, const uint8_t *payload)
{
    memcpy(dst, payload, sizeof(*dst));

    dst->sample_count     = be64_to_host(dst->sample_count);
    dst->latest_read_time = be64_to_host(dst->latest_read_time);
    dst->total_run_time   = be64_to_host(dst->total_run_time);

    swap_monitor(&dst->cpu_exec_time.min_exec_time);
    swap_monitor(&dst->cpu_exec_time.max_exec_time);
    swap_monitor(&dst->cpu_exec_time.avg_exec_time);
    swap_monitor(&dst->cpu_exec_time.last_exec_time);

    swap_mem(&dst->heap_mem);
    swap_mem(&dst->stack_mem);
}

// BM Engineering raporunu parse eder. Struct 397 B'lik wire formatıyla
// BIRE BIR uyumlu (kanıtlandı). Status blokları yalnızca float32 alanlardan
// oluşuyor; header sonrası 384 byte = 96 adet float kesintisiz sıralı.
static void parse_bm_engineering(bm_engineering_cbit_report_t *dst, const uint8_t *payload)
{
    memcpy(dst, payload, sizeof(*dst));

    swap_vmp_header(&dst->header_st);

    // Float region start: vmp_cmsw_header_t(11) + lru_id(1) + comm_status(1) = 13
    // 96 float × 4 byte = 384 byte. Packed struct içinde unaligned erişim
    // olmasın diye memcpy ile okuyup yazıyoruz.
    uint8_t *bytes = (uint8_t *)dst + 13;
    for (size_t i = 0; i < 96; i++) {
        uint32_t v;
        memcpy(&v, bytes + i * 4, sizeof(v));
        v = __builtin_bswap32(v);
        memcpy(bytes + i * 4, &v, sizeof(v));
    }
    (void)bswap_float_array; // helper leri ilerideki yeni parse'larda kullanacağız
}

// BM FLAG (105 B) parse — header + semantic 16-bit flag fields.
static void parse_bm_flag(bm_flag_cbit_report_t *dst, const uint8_t *payload)
{
    memcpy(dst, payload, sizeof(*dst));
    swap_vmp_header(&dst->header_st);

    // Header(11) + lru(1) + comm_status(1) sonrası kalan tüm alanlar uint16_t.
    uint16_t *w = (uint16_t *)((uint8_t *)dst + 13);
    size_t n = (sizeof(*dst) - 13) / sizeof(uint16_t);
    for (size_t i = 0; i < n; i++) {
        w[i] = be16_to_host(w[i]);
    }
}

// DTN ES parse — wire 352 B (HW_VCC_INT çıkarıldı).
// A664_ES_FW_VER alanı bitfield (40-bit reserved + 3 u8), swap gerekmez.
// Kalan uint16/uint32/uint64 alanları BE→host çevrilir.
static void parse_dtn_es(dtn_es_cbit_report_t *dst, const uint8_t *payload)
{
    memcpy(dst, payload, sizeof(*dst));
    swap_vmp_header(&dst->header_st);

    dtn_es_monitoring_t *m = &dst->dtn_es_monitoring_st;

    // A664_ES_FW_VER: bitfield (40 reserved + major/minor/bugfix) — swap yok

    // uint64 ve uint32 alanları tek tek swap
    m->A664_ES_DEV_ID        = be64_to_host(m->A664_ES_DEV_ID);
    m->A664_ES_MODE          = be64_to_host(m->A664_ES_MODE);
    m->A664_ES_CONFIG_ID     = be64_to_host(m->A664_ES_CONFIG_ID);
    m->A664_ES_BIT_STATUS    = be64_to_host(m->A664_ES_BIT_STATUS);
    m->A664_ES_CONFIG_STATUS = be64_to_host(m->A664_ES_CONFIG_STATUS);

    m->A664_PTP_CONFIG_ID  = be16_to_host(m->A664_PTP_CONFIG_ID);
    m->A664_PTP_SYNC_VL_ID = be16_to_host(m->A664_PTP_SYNC_VL_ID);
    m->A664_PTP_REQ_VL_ID  = be16_to_host(m->A664_PTP_REQ_VL_ID);
    m->A664_PTP_RES_VL_ID  = be16_to_host(m->A664_PTP_RES_VL_ID);

    // HW_TEMP, HW_VCC_INT, TRANSCEIVER_TEMP hepsi float32 BE — bytes-swap edip yorum değişmiyor
    {
        uint32_t v;
        memcpy(&v, &m->A664_ES_HW_TEMP, 4);           v = __builtin_bswap32(v); memcpy(&m->A664_ES_HW_TEMP,          &v, 4);
        memcpy(&v, &m->A664_ES_HW_VCC_INT, 4);        v = __builtin_bswap32(v); memcpy(&m->A664_ES_HW_VCC_INT,       &v, 4);
        memcpy(&v, &m->A664_ES_TRANSCEIVER_TEMP, 4);  v = __builtin_bswap32(v); memcpy(&m->A664_ES_TRANSCEIVER_TEMP, &v, 4);
    }
    m->A664_ES_PORT_A_STATUS                      = be64_to_host(m->A664_ES_PORT_A_STATUS);
    m->A664_ES_PORT_B_STATUS                      = be64_to_host(m->A664_ES_PORT_B_STATUS);
    m->A664_ES_TX_INCOMING_COUNT                  = be64_to_host(m->A664_ES_TX_INCOMING_COUNT);
    m->A664_ES_TX_A_OUTGOING_COUNT                = be64_to_host(m->A664_ES_TX_A_OUTGOING_COUNT);
    m->A664_ES_TX_B_OUTGOING_COUNT                = be64_to_host(m->A664_ES_TX_B_OUTGOING_COUNT);
    m->A664_ES_TX_VLID_DROP_COUNT                 = be64_to_host(m->A664_ES_TX_VLID_DROP_COUNT);
    m->A664_ES_TX_LMIN_LMAX_DROP_COUNT            = be64_to_host(m->A664_ES_TX_LMIN_LMAX_DROP_COUNT);
    m->A664_ES_TX_MAX_JITTER_DROP_COUNT           = be64_to_host(m->A664_ES_TX_MAX_JITTER_DROP_COUNT);
    m->A664_ES_RX_A_INCOMING_COUNT                = be64_to_host(m->A664_ES_RX_A_INCOMING_COUNT);
    m->A664_ES_RX_B_INCOMING_COUNT                = be64_to_host(m->A664_ES_RX_B_INCOMING_COUNT);
    m->A664_ES_RX_OUTGOING_COUNT                  = be64_to_host(m->A664_ES_RX_OUTGOING_COUNT);
    m->A664_ES_RX_A_VLID_DROP_COUNT               = be64_to_host(m->A664_ES_RX_A_VLID_DROP_COUNT);
    m->A664_ES_RX_A_LMIN_LMAX_DROP_COUNT          = be64_to_host(m->A664_ES_RX_A_LMIN_LMAX_DROP_COUNT);
    m->A664_ES_RX_A_NET_ERR_COUNT                 = be64_to_host(m->A664_ES_RX_A_NET_ERR_COUNT);
    m->A664_ES_RX_A_SEQ_ERR_COUNT                 = be64_to_host(m->A664_ES_RX_A_SEQ_ERR_COUNT);
    m->A664_ES_RX_A_CRC_ERROR_COUNT               = be64_to_host(m->A664_ES_RX_A_CRC_ERROR_COUNT);
    m->A664_ES_RX_A_IP_CHECKSUM_ERROR_COUNT       = be64_to_host(m->A664_ES_RX_A_IP_CHECKSUM_ERROR_COUNT);
    m->A664_ES_RX_B_VLID_DROP_COUNT               = be64_to_host(m->A664_ES_RX_B_VLID_DROP_COUNT);
    m->A664_ES_RX_B_LMIN_LMAX_DROP_COUNT          = be64_to_host(m->A664_ES_RX_B_LMIN_LMAX_DROP_COUNT);
    m->A664_ES_RX_B_SEQ_ERR_COUNT                 = be64_to_host(m->A664_ES_RX_B_SEQ_ERR_COUNT);
    m->A664_ES_RX_B_NET_ERR_COUNT                 = be64_to_host(m->A664_ES_RX_B_NET_ERR_COUNT);
    m->A664_ES_RX_B_CRC_ERROR_COUNT               = be64_to_host(m->A664_ES_RX_B_CRC_ERROR_COUNT);
    m->A664_ES_RX_B_IP_CHECKSUM_ERROR_COUNT       = be64_to_host(m->A664_ES_RX_B_IP_CHECKSUM_ERROR_COUNT);
    m->A664_BSP_TX_PACKET_COUNT                   = be64_to_host(m->A664_BSP_TX_PACKET_COUNT);
    m->A664_BSP_TX_BYTE_COUNT                     = be64_to_host(m->A664_BSP_TX_BYTE_COUNT);
    m->A664_BSP_TX_ERROR_COUNT                    = be64_to_host(m->A664_BSP_TX_ERROR_COUNT);
    m->A664_BSP_RX_PACKET_COUNT                   = be64_to_host(m->A664_BSP_RX_PACKET_COUNT);
    m->A664_BSP_RX_BYTE_COUNT                     = be64_to_host(m->A664_BSP_RX_BYTE_COUNT);
    m->A664_BSP_RX_ERROR_COUNT                    = be64_to_host(m->A664_BSP_RX_ERROR_COUNT);
    m->A664_BSP_RX_MISSED_FRAME_COUNT             = be64_to_host(m->A664_BSP_RX_MISSED_FRAME_COUNT);
    m->A664_BSP_VER                               = be64_to_host(m->A664_BSP_VER);
    m->A664_ES_VENDOR_TYPE                        = be64_to_host(m->A664_ES_VENDOR_TYPE);
    m->A664_ES_BSP_QUEUING_RX_VL_PORT_DROP_COUNT  = be64_to_host(m->A664_ES_BSP_QUEUING_RX_VL_PORT_DROP_COUNT);
}

// DTN SW parse — wire 1064 B (57 B status + 8×124 B port).
static void parse_dtn_sw(dtn_sw_cbit_report_t *dst, const uint8_t *payload)
{
    memcpy(dst, payload, sizeof(*dst));
    swap_vmp_header(&dst->header_st);

    // Status
    dtn_sw_status_mon_t *s = &dst->dtn_sw_monitoring_st.status;
    s->A664_SW_TX_TOTAL_COUNT      = be64_to_host(s->A664_SW_TX_TOTAL_COUNT);
    s->A664_SW_RX_TOTAL_COUNT      = be64_to_host(s->A664_SW_RX_TOTAL_COUNT);
    // float BE → host (uint32 swap + reinterpret)
    {
        uint32_t v;
        memcpy(&v, &s->A664_SW_TRANSCEIVER_TEMP, 4);
        v = __builtin_bswap32(v);
        memcpy(&s->A664_SW_TRANSCEIVER_TEMP, &v, 4);

        memcpy(&v, &s->A664_SW_SHARED_TRANSCEIVER_TEMP, 4);
        v = __builtin_bswap32(v);
        memcpy(&s->A664_SW_SHARED_TRANSCEIVER_TEMP, &v, 4);

        memcpy(&v, &s->A664_SW_VOLTAGE, 4);
        v = __builtin_bswap32(v);
        memcpy(&s->A664_SW_VOLTAGE, &v, 4);

        memcpy(&v, &s->A664_SW_TEMPERATURE, 4);
        v = __builtin_bswap32(v);
        memcpy(&s->A664_SW_TEMPERATURE, &v, 4);
    }
    s->A664_SW_DEV_ID              = be16_to_host(s->A664_SW_DEV_ID);
    s->A664_SW_FW_VER              = be64_to_host(s->A664_SW_FW_VER);
    s->A664_SW_EMBEDEED_ES_FW_VER  = be64_to_host(s->A664_SW_EMBEDEED_ES_FW_VER);
    s->A664_SW_CONFIGURATION_ID    = be16_to_host(s->A664_SW_CONFIGURATION_ID);

    // Ports
    for (int i = 0; i < 8; i++) {
        dtn_sw_port_mon_t *p = &dst->dtn_sw_monitoring_st.port[i];
        p->A664_SW_PORT_ID = be16_to_host(p->A664_SW_PORT_ID);
        p->A664_SW_PORT_i_CRC_ERR_COUNT              = be64_to_host(p->A664_SW_PORT_i_CRC_ERR_COUNT);
        p->A664_SW_PORT_i_MIN_VL_FRAME_ERR_COUNT     = be64_to_host(p->A664_SW_PORT_i_MIN_VL_FRAME_ERR_COUNT);
        p->A664_SW_PORT_i_MAX_VL_FRAME_ERR_COUNT     = be64_to_host(p->A664_SW_PORT_i_MAX_VL_FRAME_ERR_COUNT);
        p->A664_SW_PORT_i_TRAFFIC_POLCY_DROP_COUNT   = be64_to_host(p->A664_SW_PORT_i_TRAFFIC_POLCY_DROP_COUNT);
        p->A664_SW_PORT_i_BE_COUNT                   = be64_to_host(p->A664_SW_PORT_i_BE_COUNT);
        p->A664_SW_PORT_i_TX_COUNT                   = be64_to_host(p->A664_SW_PORT_i_TX_COUNT);
        p->A664_SW_PORT_i_RX_COUNT                   = be64_to_host(p->A664_SW_PORT_i_RX_COUNT);
        p->A664_SW_PORT_i_VL_SOURCE_ERR_COUNT        = be64_to_host(p->A664_SW_PORT_i_VL_SOURCE_ERR_COUNT);
        p->A664_SW_PORT_i_MAX_DELAY_ERR_COUNT        = be64_to_host(p->A664_SW_PORT_i_MAX_DELAY_ERR_COUNT);
        p->A664_SW_PORT_i_VLID_DROP_COUNT            = be64_to_host(p->A664_SW_PORT_i_VLID_DROP_COUNT);
        p->A664_SW_PORT_i_UNDEF_MAC_COUNT            = be64_to_host(p->A664_SW_PORT_i_UNDEF_MAC_COUNT);
        p->A664_SW_PORT_i_HIGH_PRTY_QUE_OVRFLW_COUNT = be64_to_host(p->A664_SW_PORT_i_HIGH_PRTY_QUE_OVRFLW_COUNT);
        p->A664_SW_PORT_i_LOW_PRTY_QUE_OVRFLW_COUNT  = be64_to_host(p->A664_SW_PORT_i_LOW_PRTY_QUE_OVRFLW_COUNT);
        p->A664_SW_PORT_i_MAX_DELAY                  = be64_to_host(p->A664_SW_PORT_i_MAX_DELAY);
        p->A664_SW_PORT_i_SPEED                      = be64_to_host(p->A664_SW_PORT_i_SPEED);
    }
}

// VMC PBIT REPORT parse — wire 454 B (vmc_message_types.h:96-109).
// Çok-byte alanların hepsi big-endian; bitfield'lar ve uint8 dizileri swap'siz.
// Swap edilen alanlar:
//   - header_st (msg_len uint16, timestamp uint64)
//   - list[80].ret_val (int32 — policy_cmd uint8 swap'siz)
//   - vmc_serial_number (uint32)
//   - dtn_pbit_data_st içindeki üç fw_version'ın reserved_2 alanı (uint32)
//   - vs_cpu_pbit, flcs_cpu_pbit (uint16)
static void parse_vmc_pbit_data(vmc_pbit_data_t *dst, const uint8_t *payload)
{
    memcpy(dst, payload, sizeof(*dst));

    swap_vmp_header(&dst->header_st);

    // Policy step listesi: her elemanda sadece ret_val (int32) swap edilir.
    for (size_t i = 0; i < sizeof(dst->list) / sizeof(dst->list[0]); i++) {
        dst->list[i].ret_val = (int32_t)be32_to_host((uint32_t)dst->list[i].ret_val);
    }

    dst->vmc_serial_number = be32_to_host(dst->vmc_serial_number);

    // dtn_pbit_data_st: üç fw_version struct'ı, her birinde 4-byte reserved_2 + 4 uint8.
    dst->dtn_pbit_data_st.mmp_dtn_es_fw_version.reserved_2 =
        be32_to_host(dst->dtn_pbit_data_st.mmp_dtn_es_fw_version.reserved_2);
    dst->dtn_pbit_data_st.vmp_dtn_sw_es_fw_version.reserved_2 =
        be32_to_host(dst->dtn_pbit_data_st.vmp_dtn_sw_es_fw_version.reserved_2);
    dst->dtn_pbit_data_st.vmp_dtn_sw_fw_version.reserved_2 =
        be32_to_host(dst->dtn_pbit_data_st.vmp_dtn_sw_fw_version.reserved_2);

    dst->vs_cpu_pbit   = be16_to_host(dst->vs_cpu_pbit);
    dst->flcs_cpu_pbit = be16_to_host(dst->flcs_cpu_pbit);
}

// ============================================================================
// PBIT snapshot preload (Seçenek A)
// ----------------------------------------------------------------------------
// test_starter, DPDK NIC'i almadan önce kernel arayüzünde PBIT request/response
// el sıkışmasını yapıyor ve aldığı ham PBIT payload'larını dosyaya yazıyor.
// Burada o dosyaları okuyup PBIT slot'larına yüklüyoruz — byte kaynağı dışında
// canlı RX yoluyla (hm_handle_packet) BİREBİR aynı: aynı parse, aynı slot.
// ============================================================================
#define HM_PBIT_SNAPSHOT_FLCS "/tmp/vmc_pbit_flcs.bin"
#define HM_PBIT_SNAPSHOT_VS   "/tmp/vmc_pbit_vs.bin"

static void hm_load_pbit_file(const char *path, hm_pbit_slot_t *slot, const char *name)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        // No snapshot for this run — leave the slot empty (not an error).
        return;
    }

    uint8_t buf[sizeof(vmc_pbit_data_t)];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    if (n < sizeof(vmc_pbit_data_t)) {
        printf("HM: PBIT snapshot '%s' too short (%zu/%zu), ignoring\n",
               path, n, sizeof(vmc_pbit_data_t));
        return;
    }
    if (buf[0] != HM_PBIT_RESPONSE_MSG_ID) {
        printf("HM: PBIT snapshot '%s' bad msg_id=%u (expected %u), ignoring\n",
               path, buf[0], HM_PBIT_RESPONSE_MSG_ID);
        return;
    }

    pthread_mutex_lock(&slot->lock);
    parse_vmc_pbit_data(&slot->data, buf);
    slot->updated = true;
    slot->update_count++;
    pthread_mutex_unlock(&slot->lock);
    printf("HM: preloaded %s PBIT snapshot from '%s'\n", name, path);
}

void hm_preload_pbit_snapshots(void)
{
    hm_load_pbit_file(HM_PBIT_SNAPSHOT_FLCS, &flcs_pbit_slot, "FLCS");
    hm_load_pbit_file(HM_PBIT_SNAPSHOT_VS,   &vs_pbit_slot,   "VS");
}

// ============================================================================
// RX fast-path entry point
// ============================================================================
void hm_handle_packet(uint16_t vl_id, const uint8_t *payload, uint16_t len)
{
    if (payload == NULL) return;

    __atomic_add_fetch(&g_hm_rx_total, 1, __ATOMIC_RELAXED);

    switch (vl_id)
    {
        case HEALTH_MONITOR_VS_CPU_USAGE_VLID:
            if (len < sizeof(Pcs_profile_stats)) {
                __atomic_add_fetch(&g_hm_rx_short, 1, __ATOMIC_RELAXED);
                return;
            }
            pthread_mutex_lock(&vs_cpu_usage_slot.lock);
            parse_pcs_profile_stats(&vs_cpu_usage_slot.data, payload);
            vs_cpu_usage_slot.updated = true;
            vs_cpu_usage_slot.update_count++;
            pthread_mutex_unlock(&vs_cpu_usage_slot.lock);
            __atomic_add_fetch(&g_hm_rx_vs_cpu, 1, __ATOMIC_RELAXED);
            break;

        case HEALTH_MONITOR_FLCS_CPU_USAGE_VLID:
            if (len < sizeof(Pcs_profile_stats)) {
                __atomic_add_fetch(&g_hm_rx_short, 1, __ATOMIC_RELAXED);
                return;
            }
            pthread_mutex_lock(&flcs_cpu_usage_slot.lock);
            parse_pcs_profile_stats(&flcs_cpu_usage_slot.data, payload);
            flcs_cpu_usage_slot.updated = true;
            flcs_cpu_usage_slot.update_count++;
            pthread_mutex_unlock(&flcs_cpu_usage_slot.lock);
            __atomic_add_fetch(&g_hm_rx_flcs_cpu, 1, __ATOMIC_RELAXED);
            break;

        case HEALTH_MONITOR_FLCS_CBIT_VLID:
        case HEALTH_MONITOR_VS_CBIT_VLID:
        {
            if (len < sizeof(vmp_cmsw_header_t)) {
                __atomic_add_fetch(&g_hm_rx_short, 1, __ATOMIC_RELAXED);
                return;
            }
            __atomic_add_fetch(&g_hm_rx_cbit, 1, __ATOMIC_RELAXED);

            const bool is_vs = (vl_id == HEALTH_MONITOR_VS_CBIT_VLID);
            const uint8_t msg_id = payload[0];

            switch (msg_id)
            {
                case HM_CBIT_MSG_ID_BM_ENGINEERING:
                {
                    if (len < sizeof(bm_engineering_cbit_report_t)) {
                        __atomic_add_fetch(&g_hm_rx_short, 1, __ATOMIC_RELAXED);
                        break;
                    }
                    hm_bm_slot_t *slot = is_vs ? &vs_bm_engineering_slot
                                               : &flcs_bm_engineering_slot;
                    pthread_mutex_lock(&slot->lock);
                    parse_bm_engineering(&slot->data, payload);
                    slot->updated = true;
                    slot->update_count++;
                    pthread_mutex_unlock(&slot->lock);
                    break;
                }

                case HM_CBIT_MSG_ID_BM_FLAG:
                {
                    if (len < sizeof(bm_flag_cbit_report_t)) {
                        __atomic_add_fetch(&g_hm_rx_short, 1, __ATOMIC_RELAXED);
                        break;
                    }
                    hm_bm_flag_slot_t *slot = is_vs ? &vs_bm_flag_slot : &flcs_bm_flag_slot;
                    pthread_mutex_lock(&slot->lock);
                    parse_bm_flag(&slot->data, payload);
                    slot->updated = true;
                    slot->update_count++;
                    pthread_mutex_unlock(&slot->lock);
                    break;
                }

                case HM_CBIT_MSG_ID_DTN_ES:
                {
                    if (len < sizeof(dtn_es_cbit_report_t)) {
                        __atomic_add_fetch(&g_hm_rx_short, 1, __ATOMIC_RELAXED);
                        break;
                    }
                    // Geçici tampona parse et; boşsa slot'u değiştirme.
                    dtn_es_cbit_report_t tmp;
                    parse_dtn_es(&tmp, payload);
                    if (dtn_es_is_empty(&tmp)) {
                        __atomic_add_fetch(&g_hm_rx_empty, 1, __ATOMIC_RELAXED);
                        break;
                    }
                    hm_dtn_es_slot_t *slot = is_vs ? &vs_dtn_es_slot : &flcs_dtn_es_slot;
                    pthread_mutex_lock(&slot->lock);
                    slot->data = tmp;
                    slot->updated = true;
                    slot->update_count++;
                    pthread_mutex_unlock(&slot->lock);
                    break;
                }

                case HM_CBIT_MSG_ID_DTN_SW:
                {
                    if (len < sizeof(dtn_sw_cbit_report_t)) {
                        __atomic_add_fetch(&g_hm_rx_short, 1, __ATOMIC_RELAXED);
                        break;
                    }
                    dtn_sw_cbit_report_t tmp;
                    parse_dtn_sw(&tmp, payload);
                    if (dtn_sw_is_empty(&tmp)) {
                        __atomic_add_fetch(&g_hm_rx_empty, 1, __ATOMIC_RELAXED);
                        break;
                    }
                    hm_dtn_sw_slot_t *slot = is_vs ? &vs_dtn_sw_slot : &flcs_dtn_sw_slot;
                    pthread_mutex_lock(&slot->lock);
                    slot->data = tmp;
                    slot->updated = true;
                    slot->update_count++;
                    pthread_mutex_unlock(&slot->lock);
                    break;
                }

                default:
                    __atomic_add_fetch(&g_hm_rx_unknown_msg, 1, __ATOMIC_RELAXED);
                    break;
            }
            break;
        }

        case HEALTH_MONITOR_FLCS_PBIT_RESPONSE_VLID:    // 0x000a
            // Uzunluk + msg_id filtresi: aynı VL-ID'ye PBIT olmayan trafik
            // gelebiliyor (ör. 1467 B'lik PRBS paketleri); sadece spec'e
            // uyanları slot'a yaz.
            if (len < sizeof(vmc_pbit_data_t)) {
                __atomic_add_fetch(&g_hm_rx_short, 1, __ATOMIC_RELAXED);
                return;
            }
            if (payload[0] != HM_PBIT_RESPONSE_MSG_ID) {
                __atomic_add_fetch(&g_hm_rx_unknown_msg, 1, __ATOMIC_RELAXED);
                return;
            }
            pthread_mutex_lock(&flcs_pbit_slot.lock);
            parse_vmc_pbit_data(&flcs_pbit_slot.data, payload);
            flcs_pbit_slot.updated = true;
            flcs_pbit_slot.update_count++;
            pthread_mutex_unlock(&flcs_pbit_slot.lock);
            __atomic_add_fetch(&g_hm_rx_flcs_pbit, 1, __ATOMIC_RELAXED);
            break;

        case HEALTH_MONITOR_VS_PBIT_RESPONSE_VLID:      // 0x000d
            if (len < sizeof(vmc_pbit_data_t)) {
                __atomic_add_fetch(&g_hm_rx_short, 1, __ATOMIC_RELAXED);
                return;
            }
            if (payload[0] != HM_PBIT_RESPONSE_MSG_ID) {
                __atomic_add_fetch(&g_hm_rx_unknown_msg, 1, __ATOMIC_RELAXED);
                return;
            }
            pthread_mutex_lock(&vs_pbit_slot.lock);
            parse_vmc_pbit_data(&vs_pbit_slot.data, payload);
            vs_pbit_slot.updated = true;
            vs_pbit_slot.update_count++;
            pthread_mutex_unlock(&vs_pbit_slot.lock);
            __atomic_add_fetch(&g_hm_rx_vs_pbit, 1, __ATOMIC_RELAXED);
            break;

        default:
            __atomic_add_fetch(&g_hm_rx_unknown_vlid, 1, __ATOMIC_RELAXED);
            break;
    }
}

// ============================================================================
// Sıcaklık limit kontrolü
// ----------------------------------------------------------------------------
// Drain edilen her rapordaki sıcaklık alanları [HM_TEMP_MIN, HM_TEMP_MAX]
// aralığına göre denetlenir. Aralık dışı bir değer görülürse hangi cihaz/
// rapor/alan'ın hangi sebeple fail ettiğini yazan bir banner basılır ve global
// kapatma bayrağı set edilerek test durdurulur. Kontrol main thread'de
// (dashboard içinde) çalışır; veriler parse + endian-swap edilmiş haldedir.
// ============================================================================
typedef struct {
    const char *name;
    float       value;
} hm_temp_field_t;

// Sensör başına peş-peşe aralık-dışı sayacı.
// Anahtar = (device, report, field) string-literal pointer üçlüsü; bu literal'ler
// statik ömürlü ve her çağrıda aynı adreste olduğu için pointer eşitliği yeterli.
typedef struct {
    const char *device;
    const char *report;
    const char *field;
    int         streak;   // peş peşe aralık-dışı ölçüm sayısı
    bool        used;
} hm_temp_streak_t;

// 13 (BM) + 2 (ES) + 3 (SW) = 18 alan × 2 cihaz (VS/FLCS) = 36; tampon 64.
#define HM_TEMP_STREAK_MAX 64
static hm_temp_streak_t g_hm_temp_streaks[HM_TEMP_STREAK_MAX];

// İlgili sensörün sayaç kaydını döndürür; yoksa ilk boş slota oluşturur.
static hm_temp_streak_t *hm_temp_streak_get(const char *device,
                                            const char *report,
                                            const char *field)
{
    for (int i = 0; i < HM_TEMP_STREAK_MAX; i++) {
        hm_temp_streak_t *e = &g_hm_temp_streaks[i];
        if (e->used && e->device == device && e->report == report && e->field == field) {
            return e;
        }
    }
    for (int i = 0; i < HM_TEMP_STREAK_MAX; i++) {
        hm_temp_streak_t *e = &g_hm_temp_streaks[i];
        if (!e->used) {
            e->used   = true;
            e->device = device;
            e->report = report;
            e->field  = field;
            e->streak = 0;
            return e;
        }
    }
    return NULL;  // tampon dolu (36 alan için olmamalı)
}

// Tek bir sıcaklık alanını denetler. Aralık içindeyse sayacı sıfırlar. Aralık
// dışıysa sayacı artırır; PEŞ PEŞE HM_TEMP_FAIL_STREAK_LIMIT'e ulaşınca FAIL
// banner'ı basar, g_hm_temp_failed'i set eder ve kapatma bayrağını tetikler.
// Limite ulaşana kadar her ölçümde tek satırlık ardışık-sayaç uyarısı basılır.
static void hm_check_temp_value(const char *device, const char *report,
                                const char *field, float value)
{
    const bool in_range = (value >= HM_TEMP_MIN_DEGC && value <= HM_TEMP_MAX_DEGC);
    hm_temp_streak_t *st = hm_temp_streak_get(device, report, field);

    if (in_range) {
        if (st) st->streak = 0;  // düzeldi → peş peşe sayacı sıfırla
        return;
    }

    // Aralık dışı: sayacı artır (kayıt açılamadıysa doğrudan limit say).
    const int streak = st ? (++st->streak) : HM_TEMP_FAIL_STREAK_LIMIT;

    const char *reason = (value < HM_TEMP_MIN_DEGC)
        ? "minimum sinirin altinda (< -50 degC)"
        : "maksimum sinirin ustunde (> +120 degC)";

    // Henüz limite ulaşmadı → sadece uyar, testi durdurma.
    if (streak < HM_TEMP_FAIL_STREAK_LIMIT) {
        printf("[HM] UYARI: %s / %s / %s = %.3f degC (%s) ardisik %d/%d\n",
               device ? device : "UNKNOWN", report ? report : "UNKNOWN", field,
               value, reason, streak, HM_TEMP_FAIL_STREAK_LIMIT);
        fflush(stdout);
        return;
    }

    // Peş peşe limit doldu → FAIL banner + test durdur.
    printf("\n");
    printf("########################################################################################\n");
    printf("###  HEALTH MONITOR SICAKLIK FAIL — TEST DURDURULUYOR                                ###\n");
    printf("########################################################################################\n");
    printf("  Cihaz   : %s\n", device ? device : "UNKNOWN");
    printf("  Rapor   : %s\n", report ? report : "UNKNOWN");
    printf("  Alan    : %s\n", field);
    printf("  Deger   : %.3f degC\n", value);
    printf("  Sebep   : %s\n", reason);
    printf("  Ardisik : %d pes pese olcumde aralik disi (limit %d)\n",
           streak, HM_TEMP_FAIL_STREAK_LIMIT);
    printf("  Limitler: [%.1f , %.1f] degC\n", HM_TEMP_MIN_DEGC, HM_TEMP_MAX_DEGC);
    printf("########################################################################################\n");
    fflush(stdout);

    g_hm_temp_failed = true;
    if (g_hm_abort_flag) {
        *g_hm_abort_flag = true;  // main loop bir sonraki turda çıkar
    }
}

static void hm_check_temp_fields(const char *device, const char *report,
                                 const hm_temp_field_t *fields, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        hm_check_temp_value(device, report, fields[i].name, fields[i].value);
    }
}

// BM ENGINEERING — 13 sıcaklık alanı (VS CPU 5 + FLCS CPU 5 + board 3).
static void hm_check_bm_engineering_temps(const bm_engineering_cbit_report_t *d,
                                          const char *device)
{
    const hm_temp_field_t f[] = {
        { "VSCPU_core_local_temperature",  d->vs_status_st.VSCPU_core_local_temperature  },
        { "VSCPU_core_remote_temperature", d->vs_status_st.VSCPU_core_remote_temperature },
        { "VSCPU_RAM_temperature",         d->vs_status_st.VSCPU_RAM_temperature         },
        { "VSCPU_FLASH_temperature",       d->vs_status_st.VSCPU_FLASH_temperature       },
        { "VSCPU_power_IC_temperature",    d->vs_status_st.VSCPU_power_IC_temperature    },
        { "FCCPU_core_local_temperature",  d->flcs_status_st.FCCPU_core_local_temperature  },
        { "FCCPU_core_remote_temperature", d->flcs_status_st.FCCPU_core_remote_temperature },
        { "FCCPU_RAM_temperature",         d->flcs_status_st.FCCPU_RAM_temperature         },
        { "FCCPU_FLASH_temperature",       d->flcs_status_st.FCCPU_FLASH_temperature       },
        { "FCCPU_power_IC_temperature",    d->flcs_status_st.FCCPU_power_IC_temperature    },
        { "PSM_TEMP",                      d->vmc_board_status_st.PSM_TEMP            },
        { "BM_FPGA_temperature",           d->vmc_board_status_st.BM_FPGA_temperature },
        { "Board_edge_temperature",        d->vmc_board_status_st.Board_edge_temperature },
    };
    hm_check_temp_fields(device, "BM ENGINEERING CBIT", f, sizeof(f) / sizeof(f[0]));
}

// DTN ES — 2 sıcaklık alanı (HW + transceiver).
static void hm_check_dtn_es_temps(const dtn_es_cbit_report_t *d, const char *device)
{
    const dtn_es_monitoring_t *m = &d->dtn_es_monitoring_st;
    const hm_temp_field_t f[] = {
        { "A664_ES_HW_TEMP",          m->A664_ES_HW_TEMP          },
        { "A664_ES_TRANSCEIVER_TEMP", m->A664_ES_TRANSCEIVER_TEMP },
    };
    hm_check_temp_fields(device, "DTN ES CBIT", f, sizeof(f) / sizeof(f[0]));
}

// DTN SW — 3 sıcaklık alanı (transceiver + shared transceiver + switch).
static void hm_check_dtn_sw_temps(const dtn_sw_cbit_report_t *d, const char *device)
{
    const dtn_sw_status_mon_t *s = &d->dtn_sw_monitoring_st.status;
    const hm_temp_field_t f[] = {
        { "A664_SW_TRANSCEIVER_TEMP",        s->A664_SW_TRANSCEIVER_TEMP        },
        { "A664_SW_SHARED_TRANSCEIVER_TEMP", s->A664_SW_SHARED_TRANSCEIVER_TEMP },
        { "A664_SW_TEMPERATURE",             s->A664_SW_TEMPERATURE             },
    };
    hm_check_temp_fields(device, "DTN SW CBIT", f, sizeof(f) / sizeof(f[0]));
}

// ============================================================================
// Printer thread — 1 Hz dashboard
// ----------------------------------------------------------------------------
// Her tick'te SLOT BAŞINA en son veriyi bas — "updated" bayrağı "en az bir kez
// veri geldi mi?" anlamında kullanılıyor; bayrak tick'ler arasında temizlenmez.
// Böylece paket gelmediği saniyelerde bile son güncel snapshot ekrana gelir.
// ============================================================================
static bool drain_and_print_pcs_slot(hm_pcs_slot_t *slot, const char *device_name)
{
    bool has_data = false;
    Pcs_profile_stats local;

    pthread_mutex_lock(&slot->lock);
    if (slot->updated) {
        local = slot->data;
        has_data = true;
    }
    pthread_mutex_unlock(&slot->lock);

    if (has_data) {
        print_pcs_profile_stats(&local, device_name);
    }
    return has_data;
}

static bool drain_and_print_pbit_slot(hm_pbit_slot_t *slot, const char *device_name)
{
    bool has_data = false;
    vmc_pbit_data_t local;

    pthread_mutex_lock(&slot->lock);
    if (slot->updated) {
        local = slot->data;
        has_data = true;
    }
    pthread_mutex_unlock(&slot->lock);

    if (has_data) {
        print_vmc_pbit_report(&local, device_name);
    }
    return has_data;
}

static bool drain_and_print_bm_eng_slot(hm_bm_slot_t *slot, const char *device_name)
{
    bool has_data = false;
    bm_engineering_cbit_report_t local;

    pthread_mutex_lock(&slot->lock);
    if (slot->updated) {
        local = slot->data;
        has_data = true;
    }
    pthread_mutex_unlock(&slot->lock);

    if (has_data) {
        print_bm_cbit_report(&local, "BM ENGINEERING CBIT REPORT", device_name);
        hm_check_bm_engineering_temps(&local, device_name);
    }
    return has_data;
}

static bool drain_and_print_bm_flag_slot(hm_bm_flag_slot_t *slot, const char *device_name)
{
    bool has_data = false;
    bm_flag_cbit_report_t local;
    pthread_mutex_lock(&slot->lock);
    if (slot->updated) { local = slot->data; has_data = true; }
    pthread_mutex_unlock(&slot->lock);
    if (has_data) print_bm_flag_cbit_report(&local, device_name);
    return has_data;
}

static bool drain_and_print_dtn_es_slot(hm_dtn_es_slot_t *slot, const char *device_name)
{
    bool has_data = false;
    dtn_es_cbit_report_t local;
    pthread_mutex_lock(&slot->lock);
    if (slot->updated) { local = slot->data; has_data = true; }
    pthread_mutex_unlock(&slot->lock);
    if (has_data) {
        print_dtn_es_cbit_report(&local, device_name);
        hm_check_dtn_es_temps(&local, device_name);
    }
    return has_data;
}

static bool drain_and_print_dtn_sw_slot(hm_dtn_sw_slot_t *slot, const char *device_name)
{
    bool has_data = false;
    dtn_sw_cbit_report_t local;
    pthread_mutex_lock(&slot->lock);
    if (slot->updated) { local = slot->data; has_data = true; }
    pthread_mutex_unlock(&slot->lock);
    if (has_data) {
        print_dtn_sw_cbit_report(&local, device_name);
        hm_check_dtn_sw_temps(&local, device_name);
    }
    return has_data;
}

// ============================================================================
// Public dashboard — main thread içinden her saniye çağrılır.
// Stats tablosundan sonra çalıştırıldığında çıktı sıralı akar.
// ============================================================================
void hm_print_dashboard(void)
{
    static uint64_t tick = 0;

    // Dashboard cycle header — her saniyenin başlangıcını net işaretler
    printf("\n\n");
    printf("########################################################################################\n");
    printf("###  HEALTH MONITOR DASHBOARD — tick %-6lu                                            ###\n",
           (unsigned long)tick);
    printf("########################################################################################\n");

    bool any = false;
    any |= drain_and_print_pcs_slot(&vs_cpu_usage_slot,   "VS");
    any |= drain_and_print_pcs_slot(&flcs_cpu_usage_slot, "FLCS");

    any |= drain_and_print_pbit_slot(&vs_pbit_slot,   "VS");
    any |= drain_and_print_pbit_slot(&flcs_pbit_slot, "FLCS");

    any |= drain_and_print_bm_eng_slot(&vs_bm_engineering_slot,   "VS");
    any |= drain_and_print_bm_eng_slot(&flcs_bm_engineering_slot, "FLCS");

    any |= drain_and_print_bm_flag_slot(&vs_bm_flag_slot,   "VS");
    any |= drain_and_print_bm_flag_slot(&flcs_bm_flag_slot, "FLCS");

    any |= drain_and_print_dtn_es_slot(&vs_dtn_es_slot,   "VS");
    any |= drain_and_print_dtn_es_slot(&flcs_dtn_es_slot, "FLCS");

    any |= drain_and_print_dtn_sw_slot(&vs_dtn_sw_slot,   "VS");
    any |= drain_and_print_dtn_sw_slot(&flcs_dtn_sw_slot, "FLCS");

    // Her tick sonunda tanı satırı — sayaçlar + paket gelip gelmediği net.
    uint64_t total        = __atomic_load_n(&g_hm_rx_total,        __ATOMIC_RELAXED);
    uint64_t vs_cnt       = __atomic_load_n(&g_hm_rx_vs_cpu,       __ATOMIC_RELAXED);
    uint64_t flcs_cnt     = __atomic_load_n(&g_hm_rx_flcs_cpu,     __ATOMIC_RELAXED);
    uint64_t vs_pbit_cnt  = __atomic_load_n(&g_hm_rx_vs_pbit,      __ATOMIC_RELAXED);
    uint64_t flcs_pbit_cnt= __atomic_load_n(&g_hm_rx_flcs_pbit,    __ATOMIC_RELAXED);
    uint64_t cbit_cnt     = __atomic_load_n(&g_hm_rx_cbit,         __ATOMIC_RELAXED);
    uint64_t unknown_vlid = __atomic_load_n(&g_hm_rx_unknown_vlid, __ATOMIC_RELAXED);
    uint64_t unknown_msg  = __atomic_load_n(&g_hm_rx_unknown_msg,  __ATOMIC_RELAXED);
    uint64_t short_cnt    = __atomic_load_n(&g_hm_rx_short,        __ATOMIC_RELAXED);
    uint64_t empty_cnt    = __atomic_load_n(&g_hm_rx_empty,        __ATOMIC_RELAXED);
    printf("[HM] tick=%lu total=%lu vs_cpu=%lu flcs_cpu=%lu vs_pbit=%lu flcs_pbit=%lu cbit=%lu empty=%lu unk_vlid=%lu unk_msg=%lu short=%lu printed=%d\n",
           (unsigned long)tick,
           (unsigned long)total,
           (unsigned long)vs_cnt,
           (unsigned long)flcs_cnt,
           (unsigned long)vs_pbit_cnt,
           (unsigned long)flcs_pbit_cnt,
           (unsigned long)cbit_cnt,
           (unsigned long)empty_cnt,
           (unsigned long)unknown_vlid,
           (unsigned long)unknown_msg,
           (unsigned long)short_cnt,
           any ? 1 : 0);

    // Sıcaklık limiti bir kez ihlal edildiyse kalıcı uyarı bas.
    if (g_hm_temp_failed) {
        printf("[HM] TEMPERATURE CHECK: FAILED — test durduruluyor (limit %.0f..%.0f degC)\n",
               HM_TEMP_MIN_DEGC, HM_TEMP_MAX_DEGC);
    }

    // PSU telemetry is pushed from MainSoftware via UDP.
    // Print it in the same health-monitor cycle for unified visibility.
    //psu_telem_print_table();

    fflush(stdout);
    tick++;
}

// ============================================================================
// Print fonksiyonları
// ============================================================================

// 1. VMP PBIT REPORT
void print_vmc_pbit_report(const vmc_pbit_data_t *data, const char *device_name)
{
    if (!data) return;

    const char *prefix = (device_name != NULL) ? device_name : "UNKNOWN";

    printf("\n========================================================================================\n");
    printf("                            [%s] VMP PBIT REPORT                                       \n", prefix);
    printf("========================================================================================\n");

    printf("[ GENERAL INFO ]\n");
    printf(" LRU ID            : %u\n", data->lru_id);
    printf(" VMC Serial Number : %u\n", data->vmc_serial_number);
    printf(" FLCS CPU PBIT     : 0x%04X\n", data->flcs_cpu_pbit);
    printf(" VS CPU PBIT       : 0x%04X\n", data->vs_cpu_pbit);
    printf(" Msg Identifier    : 0x%02X | Length: %u bytes\n", data->header_st.message_identifier, data->header_st.message_len);

    printf("\n[ FIRMWARE & SOFTWARE VERSIONS ]\n");
    printf(" BM CD Firmware Version : %u.%u.%u\n", data->bm_cd_firmware_version_st.major, data->bm_cd_firmware_version_st.minor, data->bm_cd_firmware_version_st.bugfix);
    printf(" VMP CMSW Library Ver   : %u.%u.%u\n", data->vmp_cmsw_lib_ver.major, data->vmp_cmsw_lib_ver.minor, data->vmp_cmsw_lib_ver.bugfix);

    printf("\n[ STORAGE & COMPONENT STATUS ]\n");
    // Bit konvansiyonu: 0 = PASS (fault flag temiz), 1 = FAIL (fault flag set).
    printf(" FLCS CPU Status        : %s\n", data->vmp_storage_and_status_st.flcs_cpu_status ? "FAIL" : "PASS");
    printf(" VS CPU Status          : %s\n", data->vmp_storage_and_status_st.vs_cpu_status ? "FAIL" : "PASS");
    printf(" eMMC Storage Status    : %s\n", data->vmp_storage_and_status_st.eMMC_storage_status ? "FAIL" : "PASS");
    printf(" MRAM Storage Status    : %s\n", data->vmp_storage_and_status_st.MRAM_storage_status ? "FAIL" : "PASS");

    printf("\n[ POLICY EXECUTION STATUS ]\n");
    printf(" Exec Status Flag       : %u\n", data->policy_steps_exec_status);
    printf(" Total Policy Steps     : %u\n", data->number_of_policy_step);

    if (data->number_of_policy_step > 0) {
        printf(" -----------------------------------------\n");
        printf(" %-15s | %-15s\n", "STEP COMMAND", "RETURN VALUE");
        printf(" -----------------------------------------\n");
        int steps = (data->number_of_policy_step > 80) ? 80 : data->number_of_policy_step;
        for (int i = 0; i < steps; i++) {
            printf(" CMD: 0x%02X        | RET: %d\n", data->list[i].policy_cmd, data->list[i].ret_val);
        }
    }
    printf("========================================================================================\n");
}

// 2. BM ENGINEERING / FLAG DATA CBIT REPORT — tüm 96 float alanı basılır
// ----------------------------------------------------------------------------
// DELTA column state
// ----------------------------------------------------------------------------
// VALUE sütununun yanında her alanın bir ÖNCEKİ rapora göre değişimini
// (cur - prev) gösteriyoruz. BM engineering raporundaki 96 float alan her
// zaman struct sırasında ve bm_row() ile basıldığı için, alan adlarını tek tek
// taşımak yerine cihaz başına düz bir "önceki değer" tamponu tutuyoruz: bm_row
// her çağrıda sıradaki slotu okuyup delta'yı yazar, sonra güncel değeri aynı
// slota yazarak bir sonraki rapor için "previous" yapar. VS ve FLCS ayrı
// context kullandığından deltalar aynı cihazın önceki raporuyla karşılaştırılır.
// Yalnızca main thread'deki dashboard'dan çağrıldığı için kilit gerekmez.
#define BM_DELTA_FIELDS 96
typedef struct {
    float prev[BM_DELTA_FIELDS];
    bool  valid;   // en az bir rapor görüldü mü (ilk turda delta yok)
} bm_delta_ctx_t;

static bm_delta_ctx_t  bm_delta_vs;
static bm_delta_ctx_t  bm_delta_flcs;
static bm_delta_ctx_t *bm_dc_active = NULL;  // bu basım turunun aktif context'i
static int             bm_dc_idx    = 0;     // sıradaki alan indeksi

static void bm_section_header(const char *title)
{
    printf("\n+------------------------------------------------+------------+------------+\n");
    printf("| %-47s|   VALUE    |   DELTA    |\n", title);
    printf("+------------------------------------------------+------------+------------+\n");
}
static void bm_row(const char *name, float v)
{
    bm_delta_ctx_t *dc = bm_dc_active;
    if (dc != NULL && bm_dc_idx < BM_DELTA_FIELDS) {
        if (dc->valid)
            printf("| %-47s| %10.4f | %+10.4f |\n", name, v, v - dc->prev[bm_dc_idx]);
        else
            printf("| %-47s| %10.4f | %10s |\n", name, v, "-");
        dc->prev[bm_dc_idx] = v;  // bir sonraki rapor için "previous" olur
        bm_dc_idx++;
    } else {
        // Beklenmeyen durum (context yok / alan taşması) — delta'sız bas.
        printf("| %-47s| %10.4f | %10s |\n", name, v, "-");
    }
}
static void bm_section_footer(void)
{
    printf("+------------------------------------------------+------------+------------+\n");
}

void print_bm_cbit_report(const bm_engineering_cbit_report_t *data, const char *report_title, const char *device_name)
{
    if (!data) return;
    const char *prefix = (device_name != NULL) ? device_name : "UNKNOWN";

    // Delta sütunu: bu cihazın çalışan snapshot'ını seç ve imleci başa al.
    bm_dc_active = (strcmp(prefix, "FLCS") == 0) ? &bm_delta_flcs : &bm_delta_vs;
    bm_dc_idx    = 0;

    printf("\n========================================================================================\n");
    printf("                   [%s] %s                      \n", prefix, report_title);
    printf("========================================================================================\n");
    printf("[ GENERAL ]\n");
    printf(" Msg ID       : %-18u | Msg Len      : %u\n",
           data->header_st.message_identifier, data->header_st.message_len);
    printf(" Timestamp    : %-18lu | LRU ID       : %u\n",
           (unsigned long)data->header_st.timestamp, data->lru_id);
    printf(" Comm Status  : %u\n", data->comm_status);

    const bm_vs_cpu_status_data_t *v = &data->vs_status_st;
    bm_section_header("VS CPU STATUS DATA (22 fields)");
    bm_row("VSCPU_12V_current",                v->VSCPU_12V_current);
    bm_row("VSCPU_core_imon",                  v->VSCPU_core_imon);
    bm_row("VSCPU_3v3_rail_input_current",     v->VSCPU_3v3_rail_input_current);
    bm_row("VSCPU_G1VDD_input_current",        v->VSCPU_G1VDD_input_current);
    bm_row("VSCPU_XVDD_input_current",         v->VSCPU_XVDD_input_current);
    bm_row("VSCPU_SVDD_input_current",         v->VSCPU_SVDD_input_current);
    bm_row("VSCPU_G1VDD_1v35_rail_voltage",    v->VSCPU_G1VDD_1v35_rail_voltage);
    bm_row("VSCPU_BM_ADC_3V_1",                v->VSCPU_BM_ADC_3V_1);
    bm_row("VSCPU_1v8_rail_voltage",           v->VSCPU_1v8_rail_voltage);
    bm_row("VSCPU_VCORE_rail_voltage",         v->VSCPU_VCORE_rail_voltage);
    bm_row("VSCPU_S1VDD_rail_voltage",         v->VSCPU_S1VDD_rail_voltage);
    bm_row("VSCPU_S2VDD_rail_voltage",         v->VSCPU_S2VDD_rail_voltage);
    bm_row("VSCPU_X1VDD_rail_voltage",         v->VSCPU_X1VDD_rail_voltage);
    bm_row("VSCPU_X2VDD_rail_voltage",         v->VSCPU_X2VDD_rail_voltage);
    bm_row("VSCPU_12V_main_voltage",           v->VSCPU_12V_main_voltage);
    bm_row("VSCPU_BM_ADC_3V_2",                v->VSCPU_BM_ADC_3V_2);
    bm_row("VSCPU_1v8_rail_input_current",     v->VSCPU_1v8_rail_input_current);
    bm_row("VSCPU_core_local_temperature",     v->VSCPU_core_local_temperature);
    bm_row("VSCPU_core_remote_temperature",    v->VSCPU_core_remote_temperature);
    bm_row("VSCPU_RAM_temperature",            v->VSCPU_RAM_temperature);
    bm_row("VSCPU_FLASH_temperature",          v->VSCPU_FLASH_temperature);
    bm_row("VSCPU_power_IC_temperature",       v->VSCPU_power_IC_temperature);
    bm_section_footer();

    const bm_flcs_cpu_status_data_t *f = &data->flcs_status_st;
    bm_section_header("FLCS CPU STATUS DATA (22 fields)");
    bm_row("FCCPU_12V_current",                f->FCCPU_12V_current);
    bm_row("FCCPU_core_imon",                  f->FCCPU_core_imon);
    bm_row("FCCPU_3v3_rail_input_current",     f->FCCPU_3v3_rail_input_current);
    bm_row("FCCPU_G1VDD_input_current",        f->FCCPU_G1VDD_input_current);
    bm_row("FCCPU_XVDD_input_current",         f->FCCPU_XVDD_input_current);
    bm_row("FCCPU_SVDD_input_current",         f->FCCPU_SVDD_input_current);
    bm_row("FCCPU_G1VDD_1v35_rail_voltage",    f->FCCPU_G1VDD_1v35_rail_voltage);
    bm_row("FCCPU_BM_ADC_3V_1",                f->FCCPU_BM_ADC_3V_1);
    bm_row("FCCPU_1v8_rail_voltage",           f->FCCPU_1v8_rail_voltage);
    bm_row("FCCPU_VCORE_rail_voltage",         f->FCCPU_VCORE_rail_voltage);
    bm_row("FCCPU_S1VDD_rail_voltage",         f->FCCPU_S1VDD_rail_voltage);
    bm_row("FCCPU_S2VDD_rail_voltage",         f->FCCPU_S2VDD_rail_voltage);
    bm_row("FCCPU_X1VDD_rail_voltage",         f->FCCPU_X1VDD_rail_voltage);
    bm_row("FCCPU_X2VDD_rail_voltage",         f->FCCPU_X2VDD_rail_voltage);
    bm_row("FCCPU_12V_main_voltage",           f->FCCPU_12V_main_voltage);
    bm_row("FCCPU_BM_ADC_3V_2",                f->FCCPU_BM_ADC_3V_2);
    bm_row("FCCPU_core_local_temperature",     f->FCCPU_core_local_temperature);
    bm_row("FCCPU_core_remote_temperature",    f->FCCPU_core_remote_temperature);
    bm_row("FCCPU_RAM_temperature",            f->FCCPU_RAM_temperature);
    bm_row("FCCPU_FLASH_temperature",          f->FCCPU_FLASH_temperature);
    bm_row("FCCPU_1v8_input_current",          f->FCCPU_1v8_input_current);
    bm_row("FCCPU_power_IC_temperature",       f->FCCPU_power_IC_temperature);
    bm_section_footer();

    const bm_dtn_es_status_data_t *e = &data->dtn_es_status_st;
    bm_section_header("DTN ES STATUS DATA (15 fields)");
    bm_row("DTN_ES_VDD_input_current",         e->DTN_ES_VDD_input_current);
    bm_row("DTN_ES_3v3_rail_input_current",    e->DTN_ES_3v3_rail_input_current);
    bm_row("DTN_ES_FO_RX_imon_r",              e->DTN_ES_FO_RX_imon_r);
    bm_row("DTN_ES_1v8_rail_input_current",    e->DTN_ES_1v8_rail_input_current);
    bm_row("DTN_ES_1v3_rail_input_current",    e->DTN_ES_1v3_rail_input_current);
    bm_row("DTN_ES_12V_main_current",          e->DTN_ES_12V_main_current);
    bm_row("DTN_ES_BM_3V_1",                   e->DTN_ES_BM_3V_1);
    bm_row("DTN_ES_1V_VDD_rail_voltage",       e->DTN_ES_1V_VDD_rail_voltage);
    bm_row("DTN_ES_2v5_rail_voltage",          e->DTN_ES_2v5_rail_voltage);
    bm_row("DTN_ES_1v8_rail_voltage",          e->DTN_ES_1v8_rail_voltage);
    bm_row("DTN_ES_1V_VDDA_rail_voltage",      e->DTN_ES_1V_VDDA_rail_voltage);
    bm_row("DTN_ES_3v3_VDDIX_rail_voltage",    e->DTN_ES_3v3_VDDIX_rail_voltage);
    bm_row("DTN_ES_2v5_rail_input_current",    e->DTN_ES_2v5_rail_input_current);
    bm_row("DTN_ES_12V_main_voltage",          e->DTN_ES_12V_main_voltage);
    bm_row("DTN_ES_BM_3V_2",                   e->DTN_ES_BM_3V_2);
    bm_section_footer();

    const bm_vs_dtn_sw_status_data_t *vs = &data->vs_dtn_sw_status_st;
    bm_section_header("DTN VSW STATUS DATA (15 fields)");
    bm_row("DTN_VSW_VDD_input_current",        vs->DTN_VSW_VDD_input_current);
    bm_row("DTN_VSW_3v3_rail_input_current",   vs->DTN_VSW_3v3_rail_input_current);
    bm_row("DTN_VSW_FO_V_RX_imon_r",           vs->DTN_VSW_FO_V_RX_imon_r);
    bm_row("DTN_VSW_1v8_rail_input_current",   vs->DTN_VSW_1v8_rail_input_current);
    bm_row("DTN_VSW_1v3_rail_input_current",   vs->DTN_VSW_1v3_rail_input_current);
    bm_row("DTN_VSW_12V_main_current",         vs->DTN_VSW_12V_main_current);
    bm_row("DTN_VSW_BM_ADC_3V_1",              vs->DTN_VSW_BM_ADC_3V_1);
    bm_row("DTN_VSW_1V_VDD_rail_voltage",      vs->DTN_VSW_1V_VDD_rail_voltage);
    bm_row("DTN_VSW_2v5_rail_voltage",         vs->DTN_VSW_2v5_rail_voltage);
    bm_row("DTN_VSW_1v8_rail_voltage",         vs->DTN_VSW_1v8_rail_voltage);
    bm_row("DTN_VSW_1V_VDDA_rail_voltage",     vs->DTN_VSW_1V_VDDA_rail_voltage);
    bm_row("DTN_VSW_2v5_rail_input_current",   vs->DTN_VSW_2v5_rail_input_current);
    bm_row("DTN_VSW_12V_main_voltage",         vs->DTN_VSW_12V_main_voltage);
    bm_row("DTN_VSW_FO_VF_RX_imon_r",          vs->DTN_VSW_FO_VF_RX_imon_r);
    bm_row("DTN_VSW_3v3_VDDIX_voltage",        vs->DTN_VSW_3v3_VDDIX_voltage);
    bm_section_footer();

    const bm_flcs_dtn_sw_status_data_t *fs = &data->flcs_dtn_sw_status_st;
    bm_section_header("DTN FSW STATUS DATA (15 fields)");
    bm_row("DTN_FSW_VDD_input_current",        fs->DTN_FSW_VDD_input_current);
    bm_row("DTN_FSW_3v3_rail_input_current",   fs->DTN_FSW_3v3_rail_input_current);
    bm_row("DTN_FSW_FO_F_RX_imon_r",           fs->DTN_FSW_FO_F_RX_imon_r);
    bm_row("DTN_FSW_1v8_rail_input_current",   fs->DTN_FSW_1v8_rail_input_current);
    bm_row("DTN_FSW_1v3_rail_input_current",   fs->DTN_FSW_1v3_rail_input_current);
    bm_row("DTN_FSW_12V_main_current",         fs->DTN_FSW_12V_main_current);
    bm_row("DTN_FSW_BM_ADC_3V_1",              fs->DTN_FSW_BM_ADC_3V_1);
    bm_row("DTN_FSW_1V_VDD_rail_voltage",      fs->DTN_FSW_1V_VDD_rail_voltage);
    bm_row("DTN_FSW_2v5_rail_voltage",         fs->DTN_FSW_2v5_rail_voltage);
    bm_row("DTN_FSW_1v8_rail_voltage",         fs->DTN_FSW_1v8_rail_voltage);
    bm_row("DTN_FSW_1V_VDDA_rail_voltage",     fs->DTN_FSW_1V_VDDA_rail_voltage);
    bm_row("DTN_FSW_3v3_VDDIX_rail_voltage",   fs->DTN_FSW_3v3_VDDIX_rail_voltage);
    bm_row("DTN_FSW_2v5_rail_input_current",   fs->DTN_FSW_2v5_rail_input_current);
    bm_row("DTN_FSW_12V_main_voltage",         fs->DTN_FSW_12V_main_voltage);
    bm_row("DTN_FSW_BM_ADC_3V_2",              fs->DTN_FSW_BM_ADC_3V_2);
    bm_section_footer();

    const bm_vmc_board_status_data_t *b = &data->vmc_board_status_st;
    bm_section_header("VMC BOARD STATUS DATA (7 fields)");
    bm_row("PSM_PWR_PRI_VOLS",                 b->PSM_PWR_PRI_VOLS);
    bm_row("PSM_PWR_SEC_VOLS",                 b->PSM_PWR_SEC_VOLS);
    bm_row("PSM_INPUT_CURS",                   b->PSM_INPUT_CURS);
    bm_row("PSM_TEMP",                         b->PSM_TEMP);
    bm_row("BM_FPGA_temperature",              b->BM_FPGA_temperature);
    bm_row("Board_edge_temperature",           b->Board_edge_temperature);
    bm_row("BRD_MNGR_12V_main_current",        b->BRD_MNGR_12V_main_current);
    bm_section_footer();
    printf("========================================================================================\n");

    // Bu turda tüm alanlar tamponlandı; sonraki rapor delta hesaplayabilir.
    bm_dc_active->valid = true;
    bm_dc_active = NULL;
}

// 2b. BM FLAG — raw bitfield print helpers
// Konvansiyon (tüm isimlendirilmiş bitler için aynı):
//   BM_POL_FAULT   → bit=1 FAIL, bit=0 PASS. İsim `_p_good` / `_power_good`
//                    ile bitse bile bit aslında fault flag gibi davranıyor
//                    (alan testinde word=0x0000 iken cihaz sağlıklıydı).
//   BM_POL_VALUE   → bit ham değeri basılır (0 / 1). Semantiği PASS/FAIL
//                    değil, bilgi (ör. psm_oring_ch: hangi redundant kanalın
//                    aktif olduğu).
//   BM_POL_NEUTRAL → reserved / diğer isimsiz bitler; SET/CLEAR basılır.
typedef enum {
    BM_POL_NEUTRAL = 0,
    BM_POL_FAULT,
    BM_POL_VALUE
} bm_bit_polarity_t;

typedef struct {
    const char        *name;
    bm_bit_polarity_t  polarity;
} bm_bit_info_t;

// names[bit_index] (0..15).
static void print_bitfield_rows(const char *title, uint16_t v, const bm_bit_info_t *bits)
{

    printf("\n+------------------------------------------------+------------+\n");
    printf("| %-47s| 0x%04X     |\n", title, v);
    printf("+------------------------------------------------+------------+\n");
    printf("| %-47s| %-10s |\n", "BIT NAME", "STATE");
    printf("+------------------------------------------------+------------+\n");
    for (int bit = 15; bit >= 0; --bit) {
        const bm_bit_info_t *info = &bits[bit];
        const char *name = (info->name != NULL) ? info->name : "reserved";
        bool is_set = (v & (1u << bit)) != 0;
        const char *state;
        switch (info->polarity) {
        case BM_POL_FAULT: state = is_set ? "FAIL"  : "PASS";   break;
        case BM_POL_VALUE: state = is_set ? "1"     : "0";      break;
        case BM_POL_NEUTRAL:
        default:           state = is_set ? "SET"   : "CLEAR";  break;
        }
        printf("| b%-2d %-43s| %-10s |\n", bit, name, state);
    }
    printf("+------------------------------------------------+------------+\n");
}

static const bm_bit_info_t BM_BITS_POWER_STATUS[16] = {
    [13] = { "VSCPU_hreset_watchdog",       BM_POL_FAULT },
    [12] = { "VSCPU_hreset_soft_req",       BM_POL_FAULT },
    [11] = { "VSCPU_power_off_power_error", BM_POL_FAULT },
    [10] = { "VSCPU_power_off_soft_req",    BM_POL_FAULT },
    [9]  = { "FCPU_hreset_watchdog",        BM_POL_FAULT },
    [8]  = { "FCPU_hreset_soft_req",        BM_POL_FAULT },
    [7]  = { "FCPU_power_off_power_error",  BM_POL_FAULT },
    [6]  = { "FCPU_power_off_soft_req",     BM_POL_FAULT },
    [5]  = { "ES_power_off_power_error",    BM_POL_FAULT },
    [4]  = { "ES_power_off_soft_req",       BM_POL_FAULT },
    [3]  = { "VSW_power_off_power_error",   BM_POL_FAULT },
    [2]  = { "VSW_power_off_soft_req",      BM_POL_FAULT },
    [1]  = { "FSW_power_off_power_error",   BM_POL_FAULT },
    [0]  = { "FSW_power_off_soft_req",      BM_POL_FAULT },
};
static const bm_bit_info_t BM_BITS_VCPU_PG[16] = {
    [10] = { "VSCPU_vtt_vref_p_good",   BM_POL_FAULT },
    [9]  = { "VSCPU_xvdd_p_good",       BM_POL_FAULT },
    [8]  = { "VSCPU_s2vdd_p_good",      BM_POL_FAULT },
    [7]  = { "VSCPU_s1vdd_p_good",      BM_POL_FAULT },
    [6]  = { "VSCPU_g1vdd_1v35_p_good", BM_POL_FAULT },
    [5]  = { "VSCPU_1v8_p_good",        BM_POL_FAULT },
    [4]  = { "VSCPU_3v3_p_good",        BM_POL_FAULT },
    [3]  = { "VSCPU_5v_p_good",         BM_POL_FAULT },
    [2]  = { "VSCPU_1v3_p_good",        BM_POL_FAULT },
    [1]  = { "VSCPU_3v3_reg_p_good",    BM_POL_FAULT },
    [0]  = { "VSCPU_12v_p_fault",       BM_POL_FAULT },
};
static const bm_bit_info_t BM_BITS_FCPU_PG[16] = {
    [10] = { "FCPU_vtt_vref_p_good",   BM_POL_FAULT },
    [9]  = { "FCPU_xvdd_p_good",       BM_POL_FAULT },
    [8]  = { "FCPU_s2vdd_p_good",      BM_POL_FAULT },
    [7]  = { "FCPU_s1vdd_p_good",      BM_POL_FAULT },
    [6]  = { "FCPU_g1vdd_1v35_p_good", BM_POL_FAULT },
    [5]  = { "FCPU_1v8_p_good",        BM_POL_FAULT },
    [4]  = { "FCPU_3v3_p_good",        BM_POL_FAULT },
    [3]  = { "FCPU_5v_p_good",         BM_POL_FAULT },
    [2]  = { "FCPU_1v3_p_good",        BM_POL_FAULT },
    [1]  = { "FCPU_3v3_reg_p_good",    BM_POL_FAULT },
    [0]  = { "FCPU_12v_p_fault",       BM_POL_FAULT },
};
static const bm_bit_info_t BM_BITS_MMP_ES_PG[16] = {
    [7] = { "MMP_DTN_ES_FPGA_vddix_power_good",   BM_POL_FAULT },
    [6] = { "MMP_DTN_ES_FPGA_vdda_1V_power_good", BM_POL_FAULT },
    [5] = { "MMP_DTN_ES_FPGA_1v8_power_good",     BM_POL_FAULT },
    [4] = { "MMP_DTN_ES_FPGA_2v5_power_good",     BM_POL_FAULT },
    [3] = { "MMP_DTN_ES_FPGA_vdd_1v_power_good",  BM_POL_FAULT },
    [2] = { "MMP_DTN_ES_FPGA_3v3_power_good",     BM_POL_FAULT },
    [1] = { "MMP_DTN_ES_FPGA_1v3_power_good",     BM_POL_FAULT },
    [0] = { "MMP_DTN_ES_FPGA_12v_power_fault",    BM_POL_FAULT },
};
static const bm_bit_info_t BM_BITS_VSW_B_PG[16] = {
    [7] = { "VMP_DTN_SW_B_FPGA_vddix_power_good",   BM_POL_FAULT },
    [6] = { "VMP_DTN_SW_B_FPGA_vdda_1V_power_good", BM_POL_FAULT },
    [5] = { "VMP_DTN_SW_B_FPGA_1v8_power_good",     BM_POL_FAULT },
    [4] = { "VMP_DTN_SW_B_FPGA_2v5_power_good",     BM_POL_FAULT },
    [3] = { "VMP_DTN_SW_B_FPGA_vdd_1v_power_good",  BM_POL_FAULT },
    [2] = { "VMP_DTN_SW_B_FPGA_3v3_power_good",     BM_POL_FAULT },
    [1] = { "VMP_DTN_SW_B_FPGA_1v3_power_good",     BM_POL_FAULT },
    [0] = { "VMP_DTN_SW_B_FPGA_12v_power_fault",    BM_POL_FAULT },
};
static const bm_bit_info_t BM_BITS_VSW_A_PG[16] = {
    [7] = { "VMP_DTN_SW_A_FPGA_vddix_power_good",   BM_POL_FAULT },
    [6] = { "VMP_DTN_SW_A_FPGA_vdda_1V_power_good", BM_POL_FAULT },
    [5] = { "VMP_DTN_SW_A_FPGA_1v8_power_good",     BM_POL_FAULT },
    [4] = { "VMP_DTN_SW_A_FPGA_2v5_power_good",     BM_POL_FAULT },
    [3] = { "VMP_DTN_SW_A_FPGA_vdd_1v_power_good",  BM_POL_FAULT },
    [2] = { "VMP_DTN_SW_A_FPGA_3v3_power_good",     BM_POL_FAULT },
    [1] = { "VMP_DTN_SW_A_FPGA_1v3_power_good",     BM_POL_FAULT },
    [0] = { "VMP_DTN_SW_A_FPGA_12v_power_fault",    BM_POL_FAULT },
};
static const bm_bit_info_t BM_BITS_ICS1[16] = {
    [13] = { "VSCPU_core_power_not_ready", BM_POL_FAULT },
    [12] = { "VSCPU_core_power_fault",     BM_POL_FAULT },
    [11] = { "FCCPU_core_power_not_ready", BM_POL_FAULT },
    [10] = { "FCCPU_core_power_fault",     BM_POL_FAULT },
    [9]  = { "VSCPU_clock_foof",           BM_POL_FAULT },
    [8]  = { "VSCPU_clock_loss",           BM_POL_FAULT },
    [7]  = { "FCCPU_clock_foof",           BM_POL_FAULT },
    [6]  = { "FCCPU_clock_loss",           BM_POL_FAULT },
    [5]  = { "DTN_ES_clock_foof",          BM_POL_FAULT },
    [4]  = { "DTN_ES_clock_loss",          BM_POL_FAULT },
    [3]  = { "DTN_VSW_clock_foof",         BM_POL_FAULT },
    [2]  = { "DTN_VSW_clock_loss",         BM_POL_FAULT },
    [1]  = { "DTN_FSW_clock_foof",         BM_POL_FAULT },
    [0]  = { "DTN_FSW_clock_loss",         BM_POL_FAULT },
};
static const bm_bit_info_t BM_BITS_ICS2[16] = {
    // temp_ic_* bitleri temperature-IC fault göstergesi olarak yorumlanıyor.
    // Spec farklı ise polarity'yi düzeltmek yeterli.
    [4] = { "temp_ic_5", BM_POL_FAULT },
    [3] = { "temp_ic_4", BM_POL_FAULT },
    [2] = { "temp_ic_3", BM_POL_FAULT },
    [1] = { "temp_ic_2", BM_POL_FAULT },
    [0] = { "temp_ic_1", BM_POL_FAULT },
};

// ============================================================================
// EVENT RED bitmap — bit anlamlandırma + sadece-hatalı tablo
// ----------------------------------------------------------------------------
// Konvansiyon: bit = 0 → OK (PASS), bit = 1 → problem (FAIL).
// 10 adet red_event_flag kelimesinin her biri 16 bit. RED_EVENT_NAMES[w][b]
// = w'inci kelimenin b'inci bitinin anlamı; NULL → reserved.
// print_red_event_faults() yalnızca set (=1) olan, yani hatalı bitleri basar.
// ============================================================================
static const char *const RED_EVENT_NAMES[10][16] = {
    // red_event_flag_1 — VSCPU akım/voltaj grubu A
    {
        [0]  = "VSCPU 12V current LOW",
        [1]  = "VSCPU core imon LOW",
        [2]  = "VSCPU 3v3 rail input current LOW",
        [3]  = "VSCPU g1vdd input current LOW",
        [4]  = "VSCPU xvdd input current LOW",
        [5]  = "VSCPU svdd input current LOW",
        [6]  = "VSCPU g1vdd 1v35 rail voltage LOW",
        [8]  = "VSCPU 12V current HIGH",
        [9]  = "VSCPU core imon HIGH",
        [10] = "VSCPU 3v3 rail input current HIGH",
        [11] = "VSCPU g1vdd input current HIGH",
        [12] = "VSCPU xvdd input current HIGH",
        [13] = "VSCPU svdd input current HIGH",
        [14] = "VSCPU g1vdd 1v35 rail voltage HIGH",
    },
    // red_event_flag_2 — VSCPU voltaj grubu B
    {
        [0]  = "VSCPU 1V8 rail voltage LOW",
        [1]  = "VSCPU VCORE rail voltage LOW",
        [2]  = "VSCPU S1VDD rail voltage LOW",
        [3]  = "VSCPU S2VDD rail voltage LOW",
        [4]  = "VSCPU X1VDD rail voltage LOW",
        [5]  = "VSCPU X2VDD rail voltage LOW",
        [6]  = "VSCPU 12V main voltage LOW",
        [8]  = "VSCPU 1V8 rail voltage HIGH",
        [9]  = "VSCPU VCORE rail voltage HIGH",
        [10] = "VSCPU S1VDD rail voltage HIGH",
        [11] = "VSCPU S2VDD rail voltage HIGH",
        [12] = "VSCPU X1VDD rail voltage HIGH",
        [13] = "VSCPU X2VDD rail voltage HIGH",
        [14] = "VSCPU 12V main voltage HIGH",
    },
    // red_event_flag_3 — FCCPU akım/voltaj grubu A
    {
        [0]  = "FCCPU 12V current LOW",
        [1]  = "FCCPU core imon LOW",
        [2]  = "FCCPU 3v3 rail input current LOW",
        [3]  = "FCCPU G1VDD input current LOW",
        [4]  = "FCCPU XVDD input current LOW",
        [5]  = "FCCPU SVDD input current LOW",
        [6]  = "FCCPU G1VDD 1v35 rail voltage LOW",
        [8]  = "FCCPU 12V current HIGH",
        [9]  = "FCCPU core imon HIGH",
        [10] = "FCCPU 3v3 rail input current HIGH",
        [11] = "FCCPU G1VDD input current HIGH",
        [12] = "FCCPU XVDD input current HIGH",
        [13] = "FCCPU SVDD input current HIGH",
        [14] = "FCCPU G1VDD 1v35 rail voltage HIGH",
    },
    // red_event_flag_4 — FCCPU voltaj grubu B
    {
        [0]  = "FCCPU 1V8 rail voltage LOW",
        [1]  = "FCCPU VCORE voltage LOW",
        [2]  = "FCCPU S1VDD rail voltage LOW",
        [3]  = "FCCPU S2VDD rail voltage LOW",
        [4]  = "FCCPU X1VDD rail voltage LOW",
        [5]  = "FCCPU X2VDD rail voltage LOW",
        [6]  = "FCCPU 12V main voltage LOW",
        [8]  = "FCCPU 1V8 rail voltage HIGH",
        [9]  = "FCCPU VCORE voltage HIGH",
        [10] = "FCCPU S1VDD rail voltage HIGH",
        [11] = "FCCPU S2VDD rail voltage HIGH",
        [12] = "FCCPU X1VDD rail voltage HIGH",
        [13] = "FCCPU X2VDD rail voltage HIGH",
        [14] = "FCCPU 12V main voltage HIGH",
    },
    // red_event_flag_5 — VSCPU 1v8 + DTN-ES akım grubu
    {
        [0]  = "DTN-ES VDD input current LOW",
        [1]  = "DTN-ES 3v3 rail input current LOW",
        [2]  = "DTN-ES FO RX imon r LOW",
        [3]  = "DTN-ES 1v8 rail input current LOW",
        [4]  = "DTN-ES 1v3 rail input current LOW",
        [5]  = "DTN-ES 12V main current LOW",
        [6]  = "VSCPU 1v8 rail input current LOW",
        [8]  = "DTN-ES VDD input current HIGH",
        [9]  = "DTN-ES 3v3 rail input current HIGH",
        [10] = "DTN-ES FO RX imon r HIGH",
        [11] = "DTN-ES 1v8 rail input current HIGH",
        [12] = "DTN-ES 1v3 rail input current HIGH",
        [13] = "DTN-ES 12V main current HIGH",
        [14] = "VSCPU 1v8 rail input current HIGH",
    },
    // red_event_flag_6 — DTN-ES voltaj grubu
    {
        [0]  = "DTN-ES 1V VDD rail voltage LOW",
        [1]  = "DTN-ES 2v5 rail voltage LOW",
        [2]  = "DTN-ES 1v8 rail voltage LOW",
        [3]  = "DTN-ES 1V VDDA rail voltage LOW",
        [4]  = "DTN-ES 3v3 VDDIX rail voltage LOW",
        [5]  = "DTN-ES 2v5 rail input current LOW",
        [6]  = "DTN-ES 12V main voltage LOW",
        [8]  = "DTN-ES 1V VDD rail voltage HIGH",
        [9]  = "DTN-ES 2v5 rail voltage HIGH",
        [10] = "DTN-ES 1v8 rail voltage HIGH",
        [11] = "DTN-ES 1V VDDA rail voltage HIGH",
        [12] = "DTN-ES 3v3 VDDIX rail voltage HIGH",
        [13] = "DTN-ES 2v5 rail input current HIGH",
        [14] = "DTN-ES 12V main voltage HIGH",
    },
    // red_event_flag_7 — FCCPU 1v8 + DTN-VSW akım grubu
    {
        [0]  = "DTN-VSW VDD input current LOW",
        [1]  = "DTN-VSW 3v3 rail input current LOW",
        [3]  = "DTN-VSW 1v8 rail input current LOW",
        [4]  = "DTN-VSW 1v3 rail input current LOW",
        [5]  = "DTN-VSW 12V main current LOW",
        [6]  = "FCCPU 1v8 input current LOW",
        [8]  = "DTN-VSW VDD input current HIGH",
        [9]  = "DTN-VSW 3v3 rail input current HIGH",
        [11] = "DTN-VSW 1v8 rail input current HIGH",
        [12] = "DTN-VSW 1v3 rail input current HIGH",
        [13] = "DTN-VSW 12V main current HIGH",
        [14] = "FCCPU 1v8 input current HIGH",
    },
    // red_event_flag_8 — DTN-VSW voltaj grubu (standart-dışı yerleşim)
    {
        [0]  = "DTN-VSW 1V VDD rail voltage LOW",
        [1]  = "DTN-VSW 2v5 rail voltage LOW",
        [2]  = "DTN-VSW 1v8 rail voltage LOW",
        [3]  = "DTN-VSW 1V VDDA rail voltage LOW",
        [4]  = "DTN-VSW 2v5 rail input current LOW",
        [6]  = "DTN-VSW 12V main voltage LOW",
        [7]  = "DTN-VSW vddix 3v3 rail voltage LOW",
        [8]  = "DTN-VSW 1V VDD rail voltage HIGH",
        [9]  = "DTN-VSW 2v5 rail voltage HIGH",
        [10] = "DTN-VSW 1v8 rail voltage HIGH",
        [11] = "DTN-VSW 1V VDDA rail voltage HIGH",
        [12] = "DTN-VSW 2v5 rail input current HIGH",
        [14] = "DTN-VSW 12V main voltage HIGH",
        [15] = "DTN-VSW vddix 3v3 rail voltage HIGH",
    },
    // red_event_flag_9 — DTN-FSW akım grubu
    {
        [0]  = "DTN-FSW VDD input current LOW",
        [1]  = "DTN-FSW 3v3 rail input current LOW",
        [3]  = "DTN-FSW 1v8 rail input current LOW",
        [4]  = "DTN-FSW 1v3 rail input current LOW",
        [5]  = "DTN-FSW 12V main current LOW",
        [8]  = "DTN-FSW VDD input current HIGH",
        [9]  = "DTN-FSW 3v3 rail input current HIGH",
        [11] = "DTN-FSW 1v8 rail input current HIGH",
        [12] = "DTN-FSW 1v3 rail input current HIGH",
        [13] = "DTN-FSW 12V main current HIGH",
    },
    // red_event_flag_10 — DTN-FSW voltaj grubu
    {
        [0]  = "DTN-FSW 1V VDD rail voltage LOW",
        [1]  = "DTN-FSW 2v5 rail voltage LOW",
        [2]  = "DTN-FSW 1v8 rail voltage LOW",
        [3]  = "DTN-FSW 1V VDDA rail voltage LOW",
        [4]  = "DTN-FSW 3v3 VDDIX rail voltage LOW",
        [5]  = "DTN-FSW 2v5 rail input current LOW",
        [6]  = "DTN-FSW 12V main voltage LOW",
        [8]  = "DTN-FSW 1V VDD rail voltage HIGH",
        [9]  = "DTN-FSW 2v5 rail voltage HIGH",
        [10] = "DTN-FSW 1v8 rail voltage HIGH",
        [11] = "DTN-FSW 1V VDDA rail voltage HIGH",
        [12] = "DTN-FSW 3v3 VDDIX rail voltage HIGH",
        [13] = "DTN-FSW 2v5 rail input current HIGH",
        [14] = "DTN-FSW 12V main voltage HIGH",
    },
};

// ============================================================================
// EVENT ORANGE bitmap — bit anlamlandırma (12 kelime × 16 bit).
// Konvansiyon RED ile aynı: bit = 1 → problem (FAIL). NULL → reserved.
// Not: orange'da HIGH/LOW ayrımı yok; sinyal başına tek bit.
//      orange_event_flag_10 bit 0 henüz netleşmedi → NULL (sonra doldurulacak).
// ============================================================================
static const char *const ORANGE_EVENT_NAMES[12][16] = {
    // orange_event_flag_1 — VSCPU akım/voltaj grubu A
    {
        [0] = "VSCPU 12V current",
        [1] = "VSCPU core imon",
        [2] = "VSCPU 3v3 rail input current",
        [3] = "VSCPU g1vdd input current",
        [4] = "VSCPU xvdd input current",
        [5] = "VSCPU svdd input current",
        [6] = "VSCPU g1vdd 1v35 rail voltage",
    },
    // orange_event_flag_2 — VSCPU voltaj grubu B
    {
        [0] = "VSCPU 1V8 rail voltage",
        [1] = "VSCPU VCORE rail voltage",
        [2] = "VSCPU S1VDD rail voltage",
        [3] = "VSCPU S2VDD rail voltage",
        [4] = "VSCPU X1VDD rail voltage",
        [5] = "VSCPU X2VDD rail voltage",
        [6] = "VSCPU 12V main voltage",
    },
    // orange_event_flag_3 — FCCPU akım/voltaj grubu A
    {
        [0] = "FCCPU 12V current",
        [1] = "FCCPU core imon",
        [2] = "FCCPU 3v3 rail input current",
        [3] = "FCCPU G1VDD input current",
        [4] = "FCCPU XVDD input current",
        [5] = "FCCPU SVDD input current",
        [6] = "FCCPU G1VDD 1v35 rail voltage",
    },
    // orange_event_flag_4 — FCCPU voltaj grubu B
    {
        [0] = "FCCPU 1V8 rail voltage",
        [1] = "FCCPU VCORE voltage",
        [2] = "FCCPU S1VDD rail voltage",
        [3] = "FCCPU S2VDD rail voltage",
        [4] = "FCCPU X1VDD rail voltage",
        [5] = "FCCPU X2VDD rail voltage",
        [6] = "FCCPU 12V main voltage",
    },
    // orange_event_flag_5 — VSCPU 1v8 + DTN-ES akım grubu
    {
        [0] = "DTN-ES VDD input current",
        [1] = "DTN-ES 3v3 rail input current",
        [2] = "DTN-ES FO RX imon r",
        [3] = "DTN-ES 1v8 rail input current",
        [4] = "DTN-ES 1v3 rail input current",
        [5] = "DTN-ES 12V main current",
        [6] = "VSCPU 1v8 rail input current",
    },
    // orange_event_flag_6 — DTN-ES voltaj grubu
    {
        [0] = "DTN-ES 1V VDD rail voltage",
        [1] = "DTN-ES 2v5 rail voltage",
        [2] = "DTN-ES 1v8 rail voltage",
        [3] = "DTN-ES 1V VDDA rail voltage",
        [4] = "DTN-ES 3v3 VDDIX rail voltage",
        [5] = "DTN-ES 2v5 rail input current",
        [6] = "DTN-ES 12V main voltage",
    },
    // orange_event_flag_7 — FCCPU 1v8 + DTN-VSW akım grubu (bit 2 reserved)
    {
        [0] = "DTN-VSW VDD input current",
        [1] = "DTN-VSW 3v3 rail input current",
        [3] = "DTN-VSW 1v8 rail input current",
        [4] = "DTN-VSW 1v3 rail input current",
        [5] = "DTN-VSW 12V main current",
        [6] = "FCCPU 1v8 input current",
    },
    // orange_event_flag_8 — DTN-VSW voltaj grubu (bit 5 reserved, bit 7 dolu)
    {
        [0] = "DTN-VSW 1V VDD rail voltage",
        [1] = "DTN-VSW 2v5 rail voltage",
        [2] = "DTN-VSW 1v8 rail voltage",
        [3] = "DTN-VSW 1V VDDA rail voltage",
        [4] = "DTN-VSW 2v5 rail input current",
        [6] = "DTN-VSW 12V main voltage",
        [7] = "DTN-VSW vddix 3v3 rail voltage",
    },
    // orange_event_flag_9 — DTN-FSW akım grubu (bit 2 reserved)
    {
        [0] = "DTN-FSW VDD input current",
        [1] = "DTN-FSW 3v3 rail input current",
        [3] = "DTN-FSW 1v8 rail input current",
        [4] = "DTN-FSW 1v3 rail input current",
        [5] = "DTN-FSW 12V main current",
    },
    // orange_event_flag_10 — DTN-FSW voltaj grubu (bit 0 henüz netleşmedi → NULL)
    {
        [1] = "DTN-FSW 2v5 rail voltage",
        [2] = "DTN-FSW 1v8 rail voltage",
        [3] = "DTN-FSW 1V VDDA rail voltage",
        [4] = "DTN-FSW 3v3 VDDIX rail voltage",
        [5] = "DTN-FSW 2v5 rail input current",
        [6] = "DTN-FSW 12V main voltage",
    },
    // orange_event_flag_11 — sıcaklık + PSM event'leri (16 bit dolu)
    {
        [0]  = "PSM PWR-PRI VOLS voltage event",
        [1]  = "PSM PWR-SEC VOLS voltage event",
        [2]  = "PSM INPUT CURS current event",
        [3]  = "FCCPU_FLASH_temperature",
        [4]  = "FCCPU_RAM_temperature",
        [5]  = "FCCPU_core_remote_temperature",
        [6]  = "FCCPU_core_local_temperature",
        [7]  = "FCCPU_pwr_ic_temperature",
        [8]  = "VSCPU_FLASH_temperature",
        [9]  = "VSCPU_RAM_temperature",
        [10] = "VSCPU_core_remote_temperature",
        [11] = "VSCPU_core_local_temperature",
        [12] = "VSCPU_pwr_ic_temperature",
        [13] = "BM_FPGA_temperature",
        [14] = "PSM_TEMP",
        [15] = "Board_edge_temperature",
    },
    // orange_event_flag_12 — switch / ES transceiver+FPGA sıcaklıkları
    {
        [0] = "A664_ES_TEMPERATURE",
        [1] = "A664_TRANSCEIVER_TEMPERATURE",
        [2] = "A664_VSW_FPGA_TEMPERATURE",
        [3] = "A664_VSW_TRANSCEIVER_TEMP",
        [4] = "A664_SW_SHARED_TRANSCEIVER_TEMP",
        [5] = "A664_FSW_FPGA_TEMPERATURE",
        [6] = "A664_FSW_TRANSCEIVER_TEMP",
    },
};

// ============================================================================
// EVENT YELLOW bitmap — bit anlamlandırma (12 kelime × 16 bit).
// ORANGE ile büyük ölçüde aynı; farklar:
//   - yellow_event_flag_9 bit 6..13 henüz netleşmedi → NULL (sonra doldurulacak)
//   - yellow_event_flag_10 bit 0 = "DTN-FSW 1V VDD rail voltage" (orange'da NULL)
// Konvansiyon: bit = 1 → problem (FAIL). NULL → reserved.
// ============================================================================
static const char *const YELLOW_EVENT_NAMES[12][16] = {
    // yellow_event_flag_1 — VSCPU akım/voltaj grubu A
    {
        [0] = "VSCPU 12V current",
        [1] = "VSCPU core imon",
        [2] = "VSCPU 3v3 rail input current",
        [3] = "VSCPU g1vdd input current",
        [4] = "VSCPU xvdd input current",
        [5] = "VSCPU svdd input current",
        [6] = "VSCPU g1vdd 1v35 rail voltage",
    },
    // yellow_event_flag_2 — VSCPU voltaj grubu B
    {
        [0] = "VSCPU 1V8 rail voltage",
        [1] = "VSCPU VCORE rail voltage",
        [2] = "VSCPU S1VDD rail voltage",
        [3] = "VSCPU S2VDD rail voltage",
        [4] = "VSCPU X1VDD rail voltage",
        [5] = "VSCPU X2VDD rail voltage",
        [6] = "VSCPU 12V main voltage",
    },
    // yellow_event_flag_3 — FCCPU akım/voltaj grubu A
    {
        [0] = "FCCPU 12V current",
        [1] = "FCCPU core imon",
        [2] = "FCCPU 3v3 rail input current",
        [3] = "FCCPU G1VDD input current",
        [4] = "FCCPU XVDD input current",
        [5] = "FCCPU SVDD input current",
        [6] = "FCCPU G1VDD 1v35 rail voltage",
    },
    // yellow_event_flag_4 — FCCPU voltaj grubu B
    {
        [0] = "FCCPU 1V8 rail voltage",
        [1] = "FCCPU VCORE voltage",
        [2] = "FCCPU S1VDD rail voltage",
        [3] = "FCCPU S2VDD rail voltage",
        [4] = "FCCPU X1VDD rail voltage",
        [5] = "FCCPU X2VDD rail voltage",
        [6] = "FCCPU 12V main voltage",
    },
    // yellow_event_flag_5 — VSCPU 1v8 + DTN-ES akım grubu
    {
        [0] = "DTN-ES VDD input current",
        [1] = "DTN-ES 3v3 rail input current",
        [2] = "DTN-ES FO RX imon r",
        [3] = "DTN-ES 1v8 rail input current",
        [4] = "DTN-ES 1v3 rail input current",
        [5] = "DTN-ES 12V main current",
        [6] = "VSCPU 1v8 rail input current",
    },
    // yellow_event_flag_6 — DTN-ES voltaj grubu
    {
        [0] = "DTN-ES 1V VDD rail voltage",
        [1] = "DTN-ES 2v5 rail voltage",
        [2] = "DTN-ES 1v8 rail voltage",
        [3] = "DTN-ES 1V VDDA rail voltage",
        [4] = "DTN-ES 3v3 VDDIX rail voltage",
        [5] = "DTN-ES 2v5 rail input current",
        [6] = "DTN-ES 12V main voltage",
    },
    // yellow_event_flag_7 — FCCPU 1v8 + DTN-VSW akım grubu (bit 2 reserved)
    {
        [0] = "DTN-VSW VDD input current",
        [1] = "DTN-VSW 3v3 rail input current",
        [3] = "DTN-VSW 1v8 rail input current",
        [4] = "DTN-VSW 1v3 rail input current",
        [5] = "DTN-VSW 12V main current",
        [6] = "FCCPU 1v8 input current",
    },
    // yellow_event_flag_8 — DTN-VSW voltaj grubu (bit 5 reserved, bit 7 dolu)
    {
        [0] = "DTN-VSW 1V VDD rail voltage",
        [1] = "DTN-VSW 2v5 rail voltage",
        [2] = "DTN-VSW 1v8 rail voltage",
        [3] = "DTN-VSW 1V VDDA rail voltage",
        [4] = "DTN-VSW 2v5 rail input current",
        [6] = "DTN-VSW 12V main voltage",
        [7] = "DTN-VSW vddix 3v3 rail voltage",
    },
    // yellow_event_flag_9 — DTN-FSW akım grubu (bit 2 reserved; bit 6..13 NULL)
    {
        [0] = "DTN-FSW VDD input current",
        [1] = "DTN-FSW 3v3 rail input current",
        [3] = "DTN-FSW 1v8 rail input current",
        [4] = "DTN-FSW 1v3 rail input current",
        [5] = "DTN-FSW 12V main current",
    },
    // yellow_event_flag_10 — DTN-FSW voltaj grubu (bit 0 dolu)
    {
        [0] = "DTN-FSW 1V VDD rail voltage",
        [1] = "DTN-FSW 2v5 rail voltage",
        [2] = "DTN-FSW 1v8 rail voltage",
        [3] = "DTN-FSW 1V VDDA rail voltage",
        [4] = "DTN-FSW 3v3 VDDIX rail voltage",
        [5] = "DTN-FSW 2v5 rail input current",
        [6] = "DTN-FSW 12V main voltage",
    },
    // yellow_event_flag_11 — sıcaklık + PSM event'leri (16 bit dolu)
    {
        [0]  = "PSM PWR-PRI VOLS voltage event",
        [1]  = "PSM PWR-SEC VOLS voltage event",
        [2]  = "PSM INPUT CURS current event",
        [3]  = "FCCPU_FLASH_temperature",
        [4]  = "FCCPU_RAM_temperature",
        [5]  = "FCCPU_core_remote_temperature",
        [6]  = "FCCPU_core_local_temperature",
        [7]  = "FCCPU_pwr_ic_temperature",
        [8]  = "VSCPU_FLASH_temperature",
        [9]  = "VSCPU_RAM_temperature",
        [10] = "VSCPU_core_remote_temperature",
        [11] = "VSCPU_core_local_temperature",
        [12] = "VSCPU_pwr_ic_temperature",
        [13] = "BM_FPGA_temperature",
        [14] = "PSM_TEMP",
        [15] = "Board_edge_temperature",
    },
    // yellow_event_flag_12 — switch / ES transceiver+FPGA sıcaklıkları
    {
        [0] = "A664_ES_TEMPERATURE",
        [1] = "A664_TRANSCEIVER_TEMPERATURE",
        [2] = "A664_VSW_FPGA_TEMPERATURE",
        [3] = "A664_VSW_TRANSCEIVER_TEMP",
        [4] = "A664_SW_SHARED_TRANSCEIVER_TEMP",
        [5] = "A664_FSW_FPGA_TEMPERATURE",
        [6] = "A664_FSW_TRANSCEIVER_TEMP",
    },
};

// EVENT bitmap'ini yorumlar: yalnızca set (=1) olan (= FAIL) bitleri, flag/bit
// indeksi ve anlamıyla birlikte bir tabloda listeler. Hiçbiri set değilse
// "aktif <COLOR> event yok" satırı basar. names[word_count][16] bit isimleri;
// NULL → reserved. words = word_count adet uint16 kelime (10=RED, 12=ORANGE).
static void print_event_faults(const char *color,
                               const char *const (*names)[16],
                               const uint16_t *words, int word_count)
{
    int fault_count = 0;
    for (int w = 0; w < word_count; w++) {
        for (int b = 0; b < 16; b++) {
            if (words[w] & (1u << b)) fault_count++;
        }
    }

    char title[64];
    snprintf(title, sizeof(title), "ACTIVE %s EVENT (bit=1 => FAIL)", color);

    printf("\n+-----+-----+------------------------------------------------+\n");
    printf("| %-3s | %-3s | %-46s |\n", "FLG", "BIT", title);
    printf("+-----+-----+------------------------------------------------+\n");

    if (fault_count == 0) {
        char msg[64];
        snprintf(msg, sizeof(msg), "  aktif %s event yok (tum bitler 0 = PASS)", color);
        printf("| %-58s |\n", msg);
    } else {
        for (int w = 0; w < word_count; w++) {
            uint16_t v = words[w];
            if (v == 0) continue;
            for (int b = 0; b < 16; b++) {
                if (!(v & (1u << b))) continue;
                const char *name = names[w][b];
                printf("| %-3d | %-3d | %-46s |\n",
                       w + 1, b, name ? name : "reserved (beklenmedik set!)");
            }
        }
    }
    printf("+-----+-----+------------------------------------------------+\n");
    printf("  %s event FAIL bit sayisi: %d\n", color, fault_count);
}

void print_bm_flag_cbit_report(const bm_flag_cbit_report_t *data, const char *device_name)
{
    if (!data) return;
    const char *prefix = device_name ? device_name : "UNKNOWN";

    printf("\n========================================================================================\n");
    printf("                           [%s] BM FLAG CBIT REPORT                                     \n", prefix);
    printf("========================================================================================\n");

    printf("[ GENERAL ]\n");
    printf(" Msg ID       : %-18u | Msg Len      : %u\n",
           data->header_st.message_identifier, data->header_st.message_len);
    printf(" Timestamp    : %-18lu | LRU ID       : %u\n",
           (unsigned long)data->header_st.timestamp, data->lru_id);

    printf(" Comm Status  : %u\n", data->comm_status);

    print_bitfield_rows("BM POWER STATUS", data->bm_power_status_st.bit_u16, BM_BITS_POWER_STATUS);
    print_bitfield_rows("BM VCPU POWER GOODS", data->bm_vcpu_power_goods_st.bit_u16, BM_BITS_VCPU_PG);
    print_bitfield_rows("BM FCPU POWER GOODS", data->bm_fcpu_power_goods_st.bit_u16, BM_BITS_FCPU_PG);
    print_bitfield_rows("BM MMP DTN ES FPGA POWER GOODS", data->bm_mmp_dtn_es_fpga_power_goods_st.bit_u16, BM_BITS_MMP_ES_PG);
    print_bitfield_rows("BM VMP DTN SW B FPGA POWER GOODS", data->bm_vmp_dtn_sw_b_fpga_power_goods_st.bit_u16, BM_BITS_VSW_B_PG);
    print_bitfield_rows("BM VMP DTN SW A FPGA POWER GOODS", data->bm_vmp_dtn_sw_a_fpga_power_goods_st.bit_u16, BM_BITS_VSW_A_PG);
    print_bitfield_rows("ICS STATUS 1", data->ics_status_1_st.bit_u16, BM_BITS_ICS1);
    print_bitfield_rows("ICS STATUS 2", data->ics_status_2_st.bit_u16, BM_BITS_ICS2);

    print_bitfield_rows("PSM POWER PRIMARY FAULT", data->psm_pwr_pri_flt_st.bit_u16, (const bm_bit_info_t[16]){
        [0] = { "psm_power_primary_fault", BM_POL_FAULT },
    });
    print_bitfield_rows("PSM POWER SECONDARY FAULT", data->psm_pwr_sec_flt_st.bit_u16, (const bm_bit_info_t[16]){
        [0] = { "psm_power_secondary_fault", BM_POL_FAULT },
    });
    // psm_oring_ch: spec gereği 0 = primer güç sağlıklı (PASS), 1 = yedek
    // kanala geçilmiş (FAIL).
    print_bitfield_rows("PSM ORING CH", data->psm_oring_ch_st.bit_u16, (const bm_bit_info_t[16]){
        [0] = { "psm_oring_ch", BM_POL_FAULT },
    });
    print_bitfield_rows("PSM HOLD UP NOT OK", data->psm_hold_up_not_ok_st.bit_u16, (const bm_bit_info_t[16]){
        [0] = { "psm_hold_up_not_ok", BM_POL_FAULT },
    });

    uint16_t red[10], orange[12], yellow[12];
    memcpy(red, &data->event_red_bitmaps_st, sizeof(red));
    memcpy(orange, &data->event_orange_bitmaps_st, sizeof(orange));
    memcpy(yellow, &data->event_yellow_bitmaps_st, sizeof(yellow));
    print_event_faults("RED",    RED_EVENT_NAMES,    red,    10);
    print_event_faults("ORANGE", ORANGE_EVENT_NAMES, orange, 12);
    print_event_faults("YELLOW", YELLOW_EVENT_NAMES, yellow, 12);

    printf("========================================================================================\n");
}

// 3. DTN ES CBIT — tüm 47 monitoring alanı tek tabloda basılır
static void u64_row(const char *name, uint64_t v)
{
    printf("| %-47s| %20lu |\n", name, (unsigned long)v);
}
static void u64_hex_row(const char *name, uint64_t v)
{
    printf("| %-47s| 0x%018lx |\n", name, (unsigned long)v);
}
// Hex + PASS/FAIL: değer expected ise PASS, değilse FAIL.
static void u64_status_row(const char *name, uint64_t v, uint64_t expected_pass)
{
    const char *status = (v == expected_pass) ? "PASS" : "FAIL";
    printf("| %-47s| 0x%014lx  %-4s |\n", name, (unsigned long)v, status);
}
static void u32_row(const char *name, uint32_t v)
{
    printf("| %-47s| %20u |\n", name, v);
}
static void u16_row(const char *name, uint16_t v)
{
    printf("| %-47s| %20u |\n", name, v);
}
static void u8_row(const char *name, uint8_t v)
{
    printf("| %-47s| %20u |\n", name, v);
}
static void float_row(const char *name, float v, const char *unit)
{
    printf("| %-47s| %14.3f %-5s |\n", name, v, unit);
}
static void section_header_wide(const char *title)
{
    printf("\n+------------------------------------------------+----------------------+\n");
    printf("| %-47s| %-20s |\n", title, "VALUE");
    printf("+------------------------------------------------+----------------------+\n");
}
static void section_footer_wide(void)
{
    printf("+------------------------------------------------+----------------------+\n");
}

void print_dtn_es_cbit_report(const dtn_es_cbit_report_t *data, const char *device_name)
{
    if (!data) return;
    const char *prefix = (device_name != NULL) ? device_name : "UNKNOWN";

    printf("\n========================================================================================\n");
    printf("                           [%s] DTN ES CBIT REPORT                                      \n", prefix);
    printf("========================================================================================\n");
    printf("[ GENERAL ]\n");
    printf(" Msg ID       : %-18u | Msg Len      : %u\n",
           data->header_st.message_identifier, data->header_st.message_len);
    printf(" Timestamp    : %-18lu | LRU ID       : %u\n",
           (unsigned long)data->header_st.timestamp, data->lru_id);
    printf(" Side Type    : %-18u | Network Type : %u\n", data->side_type, data->network_type);
    printf(" Comm Status  : %u\n", data->comm_status);

    const dtn_es_monitoring_t *m = &data->dtn_es_monitoring_st;

    section_header_wide("DEVICE / CONFIG");
    printf("| %-47s| %14u.%u.%u       |\n", "A664_ES_FW_VER (major.minor.bugfix)",
           m->A664_ES_FW_VER.major, m->A664_ES_FW_VER.minor, m->A664_ES_FW_VER.bugfix);
    u64_row("A664_ES_DEV_ID",           m->A664_ES_DEV_ID);
    u64_row("A664_ES_MODE",             m->A664_ES_MODE);
    u64_row("A664_ES_CONFIG_ID",        m->A664_ES_CONFIG_ID);
    u64_hex_row("A664_ES_BIT_STATUS",   m->A664_ES_BIT_STATUS);
    // Spec: A664_ES_CONFIG_STATUS beklenen PASS değeri 0x5b; farklı gelirse FAIL.
    u64_status_row("A664_ES_CONFIG_STATUS", m->A664_ES_CONFIG_STATUS, 0x5bULL);
    u64_row("A664_ES_VENDOR_TYPE",      m->A664_ES_VENDOR_TYPE);
    u64_row("A664_BSP_VER",             m->A664_BSP_VER);
    section_footer_wide();

    section_header_wide("PTP");
    u16_row("A664_PTP_CONFIG_ID",    m->A664_PTP_CONFIG_ID);
    u8_row ("A664_PTP_DEVICE_TYPE",  m->A664_PTP_DEVICE_TYPE);
    u8_row ("A664_PTP_RC_STATUS",    m->A664_PTP_RC_STATUS);
    u8_row ("A664_PTP_PORT_A_SYNC",  m->A664_PTP_PORT_A_SYNC);
    u8_row ("A664_PTP_PORT_B_SYNC",  m->A664_PTP_PORT_B_SYNC);
    u16_row("A664_PTP_SYNC_VL_ID",   m->A664_PTP_SYNC_VL_ID);
    u16_row("A664_PTP_REQ_VL_ID",    m->A664_PTP_REQ_VL_ID);
    u16_row("A664_PTP_RES_VL_ID",    m->A664_PTP_RES_VL_ID);
    u8_row ("A664_PTP_TOD_NETWORK",  m->A664_PTP_TOD_NETWORK);
    section_footer_wide();

    section_header_wide("HARDWARE SENSORS");
    float_row("A664_ES_HW_TEMP",          m->A664_ES_HW_TEMP,          "degC");
    float_row("A664_ES_HW_VCC_INT",       m->A664_ES_HW_VCC_INT,       "V");
    float_row("A664_ES_TRANSCEIVER_TEMP", m->A664_ES_TRANSCEIVER_TEMP, "degC");
    section_footer_wide();

    section_header_wide("PORT STATUS");
    // Spec: PORT_A/PORT_B status beklenen PASS değeri 0x0; farklı gelirse FAIL.
    u64_status_row("A664_ES_PORT_A_STATUS", m->A664_ES_PORT_A_STATUS, 0x0ULL);
    u64_status_row("A664_ES_PORT_B_STATUS", m->A664_ES_PORT_B_STATUS, 0x0ULL);
    section_footer_wide();

    section_header_wide("TX COUNTERS");
    u64_row("A664_ES_TX_INCOMING_COUNT",       m->A664_ES_TX_INCOMING_COUNT);
    u64_row("A664_ES_TX_A_OUTGOING_COUNT",     m->A664_ES_TX_A_OUTGOING_COUNT);
    u64_row("A664_ES_TX_B_OUTGOING_COUNT",     m->A664_ES_TX_B_OUTGOING_COUNT);
    u64_row("A664_ES_TX_VLID_DROP_COUNT",      m->A664_ES_TX_VLID_DROP_COUNT);
    u64_row("A664_ES_TX_LMIN_LMAX_DROP_COUNT", m->A664_ES_TX_LMIN_LMAX_DROP_COUNT);
    u64_row("A664_ES_TX_MAX_JITTER_DROP_COUNT",m->A664_ES_TX_MAX_JITTER_DROP_COUNT);
    section_footer_wide();

    section_header_wide("RX A COUNTERS");
    u64_row("A664_ES_RX_A_INCOMING_COUNT",          m->A664_ES_RX_A_INCOMING_COUNT);
    u64_row("A664_ES_RX_A_VLID_DROP_COUNT",         m->A664_ES_RX_A_VLID_DROP_COUNT);
    u64_row("A664_ES_RX_A_LMIN_LMAX_DROP_COUNT",    m->A664_ES_RX_A_LMIN_LMAX_DROP_COUNT);
    u64_row("A664_ES_RX_A_NET_ERR_COUNT",           m->A664_ES_RX_A_NET_ERR_COUNT);
    u64_row("A664_ES_RX_A_SEQ_ERR_COUNT",           m->A664_ES_RX_A_SEQ_ERR_COUNT);
    u64_row("A664_ES_RX_A_CRC_ERROR_COUNT",         m->A664_ES_RX_A_CRC_ERROR_COUNT);
    u64_row("A664_ES_RX_A_IP_CHECKSUM_ERROR_COUNT", m->A664_ES_RX_A_IP_CHECKSUM_ERROR_COUNT);
    section_footer_wide();

    section_header_wide("RX B COUNTERS");
    u64_row("A664_ES_RX_B_INCOMING_COUNT",          m->A664_ES_RX_B_INCOMING_COUNT);
    u64_row("A664_ES_RX_B_VLID_DROP_COUNT",         m->A664_ES_RX_B_VLID_DROP_COUNT);
    u64_row("A664_ES_RX_B_LMIN_LMAX_DROP_COUNT",    m->A664_ES_RX_B_LMIN_LMAX_DROP_COUNT);
    u64_row("A664_ES_RX_B_SEQ_ERR_COUNT",           m->A664_ES_RX_B_SEQ_ERR_COUNT);
    u64_row("A664_ES_RX_B_NET_ERR_COUNT",           m->A664_ES_RX_B_NET_ERR_COUNT);
    u64_row("A664_ES_RX_B_CRC_ERROR_COUNT",         m->A664_ES_RX_B_CRC_ERROR_COUNT);
    u64_row("A664_ES_RX_B_IP_CHECKSUM_ERROR_COUNT", m->A664_ES_RX_B_IP_CHECKSUM_ERROR_COUNT);
    section_footer_wide();

    section_header_wide("RX TOTAL + BSP");
    u64_row("A664_ES_RX_OUTGOING_COUNT",        m->A664_ES_RX_OUTGOING_COUNT);
    u64_row("A664_BSP_TX_PACKET_COUNT",         m->A664_BSP_TX_PACKET_COUNT);
    u64_row("A664_BSP_TX_BYTE_COUNT",           m->A664_BSP_TX_BYTE_COUNT);
    u64_row("A664_BSP_TX_ERROR_COUNT",          m->A664_BSP_TX_ERROR_COUNT);
    u64_row("A664_BSP_RX_PACKET_COUNT",         m->A664_BSP_RX_PACKET_COUNT);
    u64_row("A664_BSP_RX_BYTE_COUNT",           m->A664_BSP_RX_BYTE_COUNT);
    u64_row("A664_BSP_RX_ERROR_COUNT",          m->A664_BSP_RX_ERROR_COUNT);
    u64_row("A664_BSP_RX_MISSED_FRAME_COUNT",   m->A664_BSP_RX_MISSED_FRAME_COUNT);
    u64_row("A664_ES_BSP_QUEUING_RX_VL_PORT_DROP_COUNT",
            m->A664_ES_BSP_QUEUING_RX_VL_PORT_DROP_COUNT);
    section_footer_wide();

    printf("========================================================================================\n");
}

// 4. DTN SW CBIT — tam status + 8 port (her portun 19 alanı da basılır)
static void print_dtn_sw_port_block(int idx, const dtn_sw_port_mon_t *p)
{
    char title[64];
    snprintf(title, sizeof(title), "PORT[%d]  (port_id=%u)", idx, p->A664_SW_PORT_ID);
    section_header_wide(title);
    u16_row("A664_SW_PORT_ID",                             p->A664_SW_PORT_ID);
    u8_row ("A664_SW_PORT_i_BIT_STATUS",                   p->A664_SW_PORT_i_BIT_STATUS);
    u8_row ("A664_SW_PORT_i_STATUS",                       p->A664_SW_PORT_i_STATUS);
    u64_row("A664_SW_PORT_i_CRC_ERR_COUNT",                p->A664_SW_PORT_i_CRC_ERR_COUNT);
    u64_row("A664_SW_PORT_i_MIN_VL_FRAME_ERR_COUNT",       p->A664_SW_PORT_i_MIN_VL_FRAME_ERR_COUNT);
    u64_row("A664_SW_PORT_i_MAX_VL_FRAME_ERR_COUNT",       p->A664_SW_PORT_i_MAX_VL_FRAME_ERR_COUNT);
    u64_row("A664_SW_PORT_i_TRAFFIC_POLCY_DROP_COUNT",     p->A664_SW_PORT_i_TRAFFIC_POLCY_DROP_COUNT);
    u64_row("A664_SW_PORT_i_BE_COUNT",                     p->A664_SW_PORT_i_BE_COUNT);
    u64_row("A664_SW_PORT_i_TX_COUNT",                     p->A664_SW_PORT_i_TX_COUNT);
    u64_row("A664_SW_PORT_i_RX_COUNT",                     p->A664_SW_PORT_i_RX_COUNT);
    u64_row("A664_SW_PORT_i_VL_SOURCE_ERR_COUNT",          p->A664_SW_PORT_i_VL_SOURCE_ERR_COUNT);
    u64_row("A664_SW_PORT_i_MAX_DELAY_ERR_COUNT",          p->A664_SW_PORT_i_MAX_DELAY_ERR_COUNT);
    u64_row("A664_SW_PORT_i_VLID_DROP_COUNT",              p->A664_SW_PORT_i_VLID_DROP_COUNT);
    u64_row("A664_SW_PORT_i_UNDEF_MAC_COUNT",              p->A664_SW_PORT_i_UNDEF_MAC_COUNT);
    u64_row("A664_SW_PORT_i_HIGH_PRTY_QUE_OVRFLW_COUNT",   p->A664_SW_PORT_i_HIGH_PRTY_QUE_OVRFLW_COUNT);
    u64_row("A664_SW_PORT_i_LOW_PRTY_QUE_OVRFLW_COUNT",    p->A664_SW_PORT_i_LOW_PRTY_QUE_OVRFLW_COUNT);
    u64_row("A664_SW_PORT_i_MAX_DELAY",                    p->A664_SW_PORT_i_MAX_DELAY);
    u64_row("A664_SW_PORT_i_SPEED",                        p->A664_SW_PORT_i_SPEED);
    section_footer_wide();
}

void print_dtn_sw_cbit_report(const dtn_sw_cbit_report_t *data, const char *device_name)
{
    if (!data) return;
    const char *prefix = (device_name != NULL) ? device_name : "UNKNOWN";

    printf("\n========================================================================================\n");
    printf("                            [%s] DTN SW CBIT REPORT                                      \n", prefix);
    printf("========================================================================================\n");
    printf("[ GENERAL ]\n");
    printf(" Msg ID       : %-18u | Msg Len      : %u\n",
           data->header_st.message_identifier, data->header_st.message_len);
    printf(" Timestamp    : %-18lu | LRU ID       : %u\n",
           (unsigned long)data->header_st.timestamp, data->lru_id);
    printf(" Side Type    : %-18u | Network Type : %u\n", data->side_type, data->network_type);
    printf(" Comm Status  : %u\n", data->comm_status);

    const dtn_sw_status_mon_t *sw = &data->dtn_sw_monitoring_st.status;

    // FW version'ı u64'ün düşük 3 byte'ı major.minor.bugfix olarak yorumla
    uint8_t fw_major  = (uint8_t)((sw->A664_SW_FW_VER >> 16) & 0xff);
    uint8_t fw_minor  = (uint8_t)((sw->A664_SW_FW_VER >>  8) & 0xff);
    uint8_t fw_bugfix = (uint8_t)( sw->A664_SW_FW_VER        & 0xff);
    uint8_t ef_major  = (uint8_t)((sw->A664_SW_EMBEDEED_ES_FW_VER >> 16) & 0xff);
    uint8_t ef_minor  = (uint8_t)((sw->A664_SW_EMBEDEED_ES_FW_VER >>  8) & 0xff);
    uint8_t ef_bugfix = (uint8_t)( sw->A664_SW_EMBEDEED_ES_FW_VER        & 0xff);

    section_header_wide("SWITCH STATUS (14 fields)");
    u64_row("A664_SW_TX_TOTAL_COUNT",           sw->A664_SW_TX_TOTAL_COUNT);
    u64_row("A664_SW_RX_TOTAL_COUNT",           sw->A664_SW_RX_TOTAL_COUNT);
    float_row("A664_SW_TRANSCEIVER_TEMP",       sw->A664_SW_TRANSCEIVER_TEMP,        "degC");
    float_row("A664_SW_SHARED_TRANSCEIVER_TEMP",sw->A664_SW_SHARED_TRANSCEIVER_TEMP, "degC");
    u16_row("A664_SW_DEV_ID",                   sw->A664_SW_DEV_ID);
    u8_row ("A664_SW_PORT_COUNT",               sw->A664_SW_PORT_COUNT);
    u8_row ("A664_SW_TOKEN_BUCKET",             sw->A664_SW_TOKEN_BUCKET);
    u8_row ("A664_SW_MODE",                     sw->A664_SW_MODE);
    u8_row ("A664_SW_BE_MAC_UPDATE",            sw->A664_SW_BE_MAC_UPDATE);
    u8_row ("A664_SW_BE_UPSTREAM_MODE",         sw->A664_SW_BE_UPSTREAM_MODE);
    printf("| %-47s| %14u.%u.%u       |\n", "A664_SW_FW_VER (major.minor.bugfix)",
           fw_major, fw_minor, fw_bugfix);
    printf("| %-47s| %14u.%u.%u       |\n", "A664_SW_EMBEDEED_ES_FW_VER (M.m.b)",
           ef_major, ef_minor, ef_bugfix);
    float_row("A664_SW_VOLTAGE",                sw->A664_SW_VOLTAGE,                 "V");
    float_row("A664_SW_TEMPERATURE",            sw->A664_SW_TEMPERATURE,             "degC");
    u16_row("A664_SW_CONFIGURATION_ID",         sw->A664_SW_CONFIGURATION_ID);
    section_footer_wide();

    for (int i = 0; i < 8; i++) {
        print_dtn_sw_port_block(i, &data->dtn_sw_monitoring_st.port[i]);
    }

    printf("========================================================================================\n");
}

// 5. CPU USAGE (Pcs_profile_stats)
//
// Birim notu: tüm zaman alanları NANOSANIYE (ns).
//   total_run_time  : örnekleme penceresi genişliği (ör. ~47 ms ≈ 47,000,000 ns)
//   usage           : o pencerede CPU'nun meşgul geçirdiği süre (ns)
//   percentage      ≈ (usage / total_run_time) × 100
//   latest_read_time: son örneklemenin zaman damgası (ns, monotonik)
//   Bellek alanları BYTE cinsindendir.
static inline double ns_to_ms(uint64_t ns) { return (double)ns / 1.0e6; }
static inline double ns_to_s (uint64_t ns) { return (double)ns / 1.0e9; }
static inline double b_to_kb (uint64_t b)  { return (double)b  / 1024.0; }
static inline double b_to_mb (uint64_t b)  { return (double)b  / (1024.0 * 1024.0); }

void print_pcs_profile_stats(const Pcs_profile_stats *data, const char *device_name)
{
    if (!data) return;

    const char *prefix = (device_name != NULL) ? device_name : "UNKNOWN";

    printf("\n========================================================================================\n");
    printf("                             [%s] CPU USAGE / PROFILE STATS                            \n", prefix);
    printf("========================================================================================\n");
    printf("(time fields in ns, memory fields in bytes)\n");

    printf("\n[ GENERAL ]\n");
    printf(" Sample Count     : %lu\n",                     (unsigned long)data->sample_count);
    printf(" Latest Read Time : %lu ns  (%.3f s)\n",        (unsigned long)data->latest_read_time, ns_to_s (data->latest_read_time));
    printf(" Total Run Time   : %lu ns  (%.3f ms window)\n",(unsigned long)data->total_run_time,   ns_to_ms(data->total_run_time));

    const Pcs_cpu_exec_time_type *ct = &data->cpu_exec_time;
    printf("\n[ CPU EXEC TIME ]  usage = CPU busy ns in the %.3f ms window\n", ns_to_ms(data->total_run_time));
    printf(" %-6s | %-10s | %-18s | %-12s\n", "KIND", "PERCENT", "USAGE (ns)", "USAGE (ms)");
    printf("--------|------------|--------------------|-------------\n");
    printf(" %-6s | %-8u %% | %-18lu | %10.3f\n", "min",  ct->min_exec_time.percentage,  (unsigned long)ct->min_exec_time.usage,  ns_to_ms(ct->min_exec_time.usage));
    printf(" %-6s | %-8u %% | %-18lu | %10.3f\n", "max",  ct->max_exec_time.percentage,  (unsigned long)ct->max_exec_time.usage,  ns_to_ms(ct->max_exec_time.usage));
    printf(" %-6s | %-8u %% | %-18lu | %10.3f\n", "avg",  ct->avg_exec_time.percentage,  (unsigned long)ct->avg_exec_time.usage,  ns_to_ms(ct->avg_exec_time.usage));
    printf(" %-6s | %-8u %% | %-18lu | %10.3f\n", "last", ct->last_exec_time.percentage, (unsigned long)ct->last_exec_time.usage, ns_to_ms(ct->last_exec_time.usage));

    printf("\n[ MEMORY PROFILE ]  sizes in bytes (KB = bytes/1024, MB = bytes/1048576)\n");
    printf(" %-6s | %-14s | %-14s | %-14s | %-10s | %-10s\n",
           "REGION", "TOTAL (B)", "USED (B)", "MAX USED (B)", "TOTAL(MB)", "USED(KB)");
    printf("--------|----------------|----------------|----------------|------------|-----------\n");
    printf(" %-6s | %-14lu | %-14lu | %-14lu | %10.2f | %10.2f\n", "heap",
           (unsigned long)data->heap_mem.total_size,
           (unsigned long)data->heap_mem.used_size,
           (unsigned long)data->heap_mem.max_used_size,
           b_to_mb(data->heap_mem.total_size),
           b_to_kb(data->heap_mem.used_size));
    printf(" %-6s | %-14lu | %-14lu | %-14lu | %10.2f | %10.2f\n", "stack",
           (unsigned long)data->stack_mem.total_size,
           (unsigned long)data->stack_mem.used_size,
           (unsigned long)data->stack_mem.max_used_size,
           b_to_mb(data->stack_mem.total_size),
           b_to_kb(data->stack_mem.used_size));
    printf("========================================================================================\n");
}
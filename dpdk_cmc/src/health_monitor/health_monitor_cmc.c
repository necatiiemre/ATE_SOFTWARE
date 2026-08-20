#include "health_monitor.h"
#include "Config.h"        // DEBUG_MODE

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

// ============================================================================
// ANSI Renk ve Format Kodları
// ----------------------------------------------------------------------------
// Çıktı log dosyasına yönlendirildiğinde ham escape dizileri sorun çıkardığı
// için tüm renk/format kodları devre dışı (boş string). printf çağrılarındaki
// string birleştirmeleri olduğu gibi kalır; sadece hiçbir escape basılmaz.
// ============================================================================
#define C_RESET  ""
#define C_CYAN   ""
#define C_BOLD   ""
#define C_YELLOW ""
#define C_DIM    ""
#define C_RED    ""
#define C_GREEN  ""

// Toplam tablo genişliği (Çerçeveler hariç iç alan)
#define HM_TABLE_WIDTH 112

// ============================================================================
// VL-ID → kullanıcı dostu etiket
// ============================================================================
static const char *hm_vl_label(uint16_t vl_id)
{
    switch (vl_id) {
        case 18:   return "CMSW-A";
        case 19:   return "CMSW-B";
        case 31:   return "CMSW-SM";
        case 50:   return "CMSW-LOG";
        case 2021: return "DPM-1";
        case 2042: return "DPM-2";
        case 2063: return "DPM-3";
        case 2084: return "DPM-4";
        case 2105: return "DPM-5";
        case 8009: return "DSM-A";
        case 8109: return "DSM-B";
        default:   return "VL?";
    }
}

// ============================================================================
// Sıcaklık tablosu
// ----------------------------------------------------------------------------
// Tum birimler tek bir tabloda toplanmistir. Icerigi dolu olmayan sicakliklar '-'
// ile gosterilecektir. VL id lerden alinan tum sicaklik degerlerini gorebilmek icin 
// yapilmis bir tablodur.
// ============================================================================
typedef struct {
    bool     have_es_hw;
    int32_t  es_hw_temp;            // 1/100 °C, signed
    bool     have_sw;
    int32_t  sw_temp;              // 1/100 °C, signed (int16 sign-extend)
    uint64_t sw_xcvr_temp;        // ham, 1/256 °C (düşük 16 bit signed)
    uint64_t sw_shared_xcvr_temp; // ham, 1/256 °C
} hm_temp_entry_t;

// index 0..6 → 2021,2042,2063,2084,2105,8009,8109
static hm_temp_entry_t g_temp[7];
static const uint16_t  g_temp_vlids[7] = {2021, 2042, 2063, 2084, 2105, 8009, 8109};

static int temp_index(uint16_t vl_id)
{
    switch (vl_id) {
        case 2021: return 0;
        case 2042: return 1;
        case 2063: return 2;
        case 2084: return 3;
        case 2105: return 4;
        case 8009: return 5;
        case 8109: return 6;
        default:   return -1;
    }
}

// ============================================================================
// DPM VL flow counters — DPM VL-ID'sine göre RX/TX base VL-IDX aralığı
// ----------------------------------------------------------------------------
// Her DPM 104 RX + 104 TX akış sayacı gönderir. Akış i için:
//   RX VL-IDX = rx_base + i   (ingress, paketin DPM'e girdiği VL)
//   TX VL-IDX = tx_base + i   (egress, DPM'in bir sonraki DPM'e gönderdiği VL)
// Zincir mantığı: DPM-N'in TX base'i = DPM-(N+1)'in RX base'i.
// true → bilinen DPM; false → base bilinmiyor (ham index gösterilir).
// ============================================================================
static bool dpm_vl_bases(uint16_t vl_id, uint16_t *rx_base, uint16_t *tx_base)
{
    switch (vl_id) {
        case 2021: *rx_base = 10001; *tx_base = 10105; return true; // DPM-1
        case 2042: *rx_base = 10105; *tx_base = 10209; return true; // DPM-2
        case 2063: *rx_base = 10209; *tx_base = 10313; return true; // DPM-3
        case 2084: *rx_base = 10313; *tx_base = 10417; return true; // DPM-4
        case 2105: *rx_base = 10417; *tx_base = 10521; return true; // DPM-5
        default:   *rx_base = 0;     *tx_base = 0;     return false;
    }
}

// ============================================================================
// Decode yardımcıları (her biri kendi static buffer'ında string döner;
// aynı printf'te 2 kez kullanılabilir diye 2-slot ring)
// ============================================================================
static const char *fmt_ver(uint64_t v)
{
    static char rings[2][16];
    static int idx = 0;
    char *buf = rings[idx];
    idx = (idx + 1) & 1;
    unsigned a = (unsigned)((v >> 16) & 0xFF);
    unsigned b = (unsigned)((v >>  8) & 0xFF);
    unsigned c = (unsigned)( v        & 0xFF);
    snprintf(buf, 16, "%u.%u.%u", a, b, c);
    return buf;
}

// 1/100 °C birimi, signed. Negatif sıcaklıkları doğru yorumlamak için
// çağıran taraf uint→signed cast'i (gerekirse sign-extend ile) yapmalı.
static const char *fmt_temp(int32_t v)
{
    static char buf[16];
    int abs_v = (v < 0) ? -v : v;
    snprintf(buf, sizeof(buf), "%s%d.%02d C",
             (v < 0 ? "-" : ""), abs_v / 100, abs_v % 100);
    return buf;
}

// SFP / SFF-8472: signed 16-bit ham değer, 1/256 °C birimi.
// Field uint64'e gömülmüş; düşük 16 bit'i int16 olarak yorumla.
static const char *fmt_xcvr_temp(uint64_t v)
{
    static char buf[24];
    int16_t raw = (int16_t)(v & 0xFFFF);
    int abs_raw = (raw < 0) ? -raw : raw;
    int whole   = abs_raw / 256;
    int frac    = (abs_raw % 256) * 100 / 256;   // 2 ondalık, truncation
    snprintf(buf, sizeof(buf), "%s%d.%02d C", (raw < 0 ? "-" : ""), whole, frac);
    return buf;
}

static const char *fmt_vcc(uint32_t v)
{
    static char buf[16];
    snprintf(buf, sizeof(buf), "%u.%04u V", v / 10000u, v % 10000u);
    return buf;
}

static const char *port_speed_str(uint64_t v)
{
    switch (v) {
        case 0: return "10M";
        case 1: return "100M";
        case 2: return "1G";
        default: return "?";
    }
}

// A664_SW_PORT_LINK polaritesi: 0 = LINK_UP, 1 = LINK_DOWN.
static const char *sw_port_link_str(unsigned v)   { return v ? "DOWN" : "UP"; }
static const char *sw_port_link_short(unsigned v) { return v ? "DWN"  : "UP"; }

static const char *ptp_dev_str(uint8_t v)
{
    switch (v) {
        case 0: return "slave";
        case 1: return "master";
        case 3: return "GM";
        default: return "?";
    }
}

static void banner(uint16_t vl_id, const char *title, unsigned packets)
{
    char buffer[128];
    if (packets > 1) {
        snprintf(buffer, sizeof(buffer), "[%s] %s  " C_YELLOW "(×%u packets)" C_CYAN, hm_vl_label(vl_id), title, packets);
    } else {
        snprintf(buffer, sizeof(buffer), "[%s] %s", hm_vl_label(vl_id), title);
    }

    printf("\n" C_CYAN C_BOLD);
    printf("╔════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    
    if (packets > 1) {
        char raw_buf[128];
        snprintf(raw_buf, sizeof(raw_buf), "[%s] %s  (×%u packets)", hm_vl_label(vl_id), title, packets);
        int pad = HM_TABLE_WIDTH - strlen(raw_buf);
        printf("║ %s%*s ║\n", buffer, (pad > 0 ? pad : 0), "");
    } else {
        int pad = HM_TABLE_WIDTH - (strlen(hm_vl_label(vl_id)) + strlen(title) + 3);
        printf("║ %s%*s ║\n", buffer, (pad > 0 ? pad : 0), "");
    }
    
    printf("╠════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n");
}

static void hr(void)
{
    printf("╟────────────────────────────────────────────────────────────────────────────────────────────────────────────────╢\n");
}

static void table_footer(void)
{
    printf("╚════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

// VL-ID etiketi olmayan, serbest başlıklı banner (cross-DPM özet tabloları için).
static void banner_plain(const char *title)
{
    printf("\n" C_CYAN C_BOLD);
    printf("╔════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    int pad = HM_TABLE_WIDTH - (int)strlen(title);
    printf("║ %s%*s ║\n", title, (pad > 0 ? pad : 0), "");
    printf("╠════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n");
}

// ============================================================================
// Pcs_profile_stats
// ============================================================================
void print_pcs_profile_stats(const Pcs_profile_stats *d, uint16_t vl_id, unsigned packets)
{
    if (d == NULL) return;
    
    banner(vl_id, "PCS PROFILE STATS", packets);

    // 2 + 20 + 3 + 87 = 112
    printf("║  %-20s : %-87" PRIu64 " ║\n", "Sample Count", d->sample_count);
    printf("║  %-20s : %-84" PRIu64 " ns ║\n", "Latest Read Time", d->latest_read_time);
    printf("║  %-20s : %-84" PRIu64 " ns ║\n", "Total Run Time", d->total_run_time);

    hr();
    // 2 + 16 + 3 + 20 + 3 + 68 = 112
    printf("║  " C_BOLD "%-16s" C_RESET " │ " C_BOLD "%-20s" C_RESET " │ " C_BOLD "%-68s" C_RESET " ║\n", 
           "CPU EXEC TIME", "Percentage (%)", "Usage (ns)");
    printf("║  %-16s │ %-20u │ %-68" PRIu64 " ║\n", "Minimum", 
           d->cpu_exec_time.min_exec_time.percentage, d->cpu_exec_time.min_exec_time.usage);
    printf("║  %-16s │ %-20u │ %-68" PRIu64 " ║\n", "Maximum", 
           d->cpu_exec_time.max_exec_time.percentage, d->cpu_exec_time.max_exec_time.usage);
    printf("║  %-16s │ %-20u │ %-68" PRIu64 " ║\n", "Average", 
           d->cpu_exec_time.avg_exec_time.percentage, d->cpu_exec_time.avg_exec_time.usage);
    printf("║  %-16s │ %-20u │ %-68" PRIu64 " ║\n", "Latest", 
           d->cpu_exec_time.last_exec_time.percentage, d->cpu_exec_time.last_exec_time.usage);

    hr();
    // 2 + 16 + 3 + 20 + 3 + 20 + 3 + 45 = 112
    printf("║  " C_BOLD "%-16s" C_RESET " │ " C_BOLD "%-20s" C_RESET " │ " C_BOLD "%-20s" C_RESET " │ " C_BOLD "%-45s" C_RESET " ║\n", 
           "MEMORY PROFILE", "Total (B)", "Used (B)", "Max Used (B)");
    printf("║  %-16s │ %-20zu │ %-20zu │ %-45zu ║\n", "Heap", 
           d->heap_mem.total_size, d->heap_mem.used_size, d->heap_mem.max_used_size);
    printf("║  %-16s │ %-20zu │ %-20zu │ %-45zu ║\n", "Stack", 
           d->stack_mem.total_size, d->stack_mem.used_size, d->stack_mem.max_used_size);

    table_footer();
}

// ============================================================================
// COUNTERS_DPM / COUNTERS_DSM
// ============================================================================
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
static void print_counters_body(const uint64_t *send, const uint64_t *send_fail,
                                const uint64_t *receive, const uint64_t *crc_pass,
                                const uint64_t *crc_fail, const uint64_t *drop,
                                unsigned rows, const char *row_label)
{
    printf("║  " C_BOLD "%-4s" C_RESET " │ " C_BOLD "%-15s" C_RESET " │ " C_BOLD "%-12s" C_RESET " │ " 
           C_BOLD "%-15s" C_RESET " │ " C_BOLD "%-15s" C_RESET " │ " C_BOLD "%-12s" C_RESET " │ " 
           C_BOLD "%-19s" C_RESET " ║\n",
           row_label, "Send", "Send Fail", "Receive", "CRC Pass", "CRC Fail", "Pkg Drop");
    hr();

    unsigned shown = 0;
    for (unsigned i = 0; i < rows; i++) {
        uint64_t s = send[i], sf = send_fail[i], r = receive[i];
        uint64_t cp = crc_pass[i], cf = crc_fail[i], pd = drop[i];
        
        if ((s | sf | r | cp | cf | pd) == 0) continue;
        
        printf("║  %-4u │ %-15" PRIu64 " │ %-12" PRIu64 " │ %-15" PRIu64 
               " │ %-15" PRIu64 " │ %-12" PRIu64 " │ %-19" PRIu64 " ║\n",
               i, s, sf, r, cp, cf, pd);
        shown++;
    }
    
    if (shown == 0) {
        char msg[64];
        snprintf(msg, sizeof(msg), "(Tüm %u bağlantı noktası için sayaçlar sıfır)", rows);
        printf("║  " C_DIM "%-110s" C_RESET " ║\n", msg);
    }

    table_footer();
}

void print_counters_dpm(const COUNTERS_DPM *d, uint16_t vl_id, unsigned packets)
{
    if (d == NULL) return;
    banner(vl_id, "INTER-LRM COUNTERS DPM", packets);
    print_counters_body(d->send_count, d->send_fail_count, d->receive_count,
                        d->crc_pass_count, d->crc_fail_count, d->pkg_drop_count,
                        SRC_MAX_CONN, "SRC");
}

void print_counters_inter_dpm(const COUNTERS_INTER_DPM *d, uint16_t vl_id, unsigned packets)
{
    if (d == NULL) return;
    banner(vl_id, "INTRA-LRM COUNTERS DPM", packets);
    print_counters_body(d->send_count, d->send_fail_count, d->receive_count,
                        d->crc_pass_count, d->crc_fail_count, d->pkg_drop_count,
                        SRC_DPM_MAX_CONN, "SRC");
}

void print_counters_dsm(const COUNTERS_DSM *d, uint16_t vl_id, unsigned packets)
{
    if (d == NULL) return;
    banner(vl_id, "INTER-LRM COUNTERS DSM", packets);
    print_counters_body(d->send_count, d->send_fail_count, d->receive_count,
                        d->crc_pass_count, d->crc_fail_count, d->pkg_drop_count,
                        DST_MAX_CONN, "DST");
}
#pragma GCC diagnostic pop

// ============================================================================
// DPM VL kümülatif sayaç biriktirici (accumulator)
// ----------------------------------------------------------------------------
// ÖNEMLİ: Gelen paket KÜMÜLATİF DEĞİL — her paket o ~1 sn'lik pencerede
// yapılan TX/RX'i (delta) taşır. Toplam TX/RX'i ve kümülatif kaybı
// görebilmek için bu saniyelik değerleri kendi tarafımızda biriktiriyoruz.
// Biriktirme dashboard thread'inde (tek tüketici) yapılır → kilit gerekmez.
// Bir tick'te aynı DPM'den 2+ paket gelirse hepsi toplanmalı; bu yüzden
// hm_print_dashboard drained item'ların TAMAMI üzerinde (dedup'tan önce)
// dpm_vl_accumulate() çağırır.
// ============================================================================
typedef struct {
    uint64_t rx_total[DPM_VL_PORT_COUNT];   // kümülatif RX (akış i = base+i)
    uint64_t tx_total[DPM_VL_PORT_COUNT];   // kümülatif TX
    bool     valid;                         // en az bir paket biriktirildi mi
} dpm_vl_accum_t;

static dpm_vl_accum_t g_dpm_accum[5];       // index 0..4 → DPM-1..5

static int dpm_index(uint16_t vl_id)
{
    switch (vl_id) {
        case 2021: return 0;
        case 2042: return 1;
        case 2063: return 2;
        case 2084: return 3;
        case 2105: return 4;
        default:   return -1;
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
// Bir paketin saniyelik TX/RX değerlerini ilgili DPM'in kümülatif toplamına ekler.
void dpm_vl_accumulate(uint16_t vl_id, const COUNTERS_DPM_VL *d)
{
    int idx = dpm_index(vl_id);
    if (idx < 0 || d == NULL) return;
    for (unsigned i = 0; i < (unsigned)DPM_VL_PORT_COUNT; i++) {
        g_dpm_accum[idx].rx_total[i] += d->rx_count[i];
        g_dpm_accum[idx].tx_total[i] += d->tx_count[i];
    }
    g_dpm_accum[idx].valid = true;
}
#pragma GCC diagnostic pop

// ============================================================================
// COUNTERS_DPM_VL — per-VL RX/TX akış sayaçları (kümülatif gösterim)
// ----------------------------------------------------------------------------
// 104 RX + 104 TX akış; tablo 2'li sütun (52 fiziksel satır) halinde basılır,
// böylece tüm 104 akış tek tabloda görünür. Sol sütun akış 0..51, sağ sütun
// akış 52..103. Her hücre: RX VL-IDX, RX count, TX VL-IDX, TX count.
// ============================================================================
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
void print_counters_dpm_vl(const COUNTERS_DPM_VL *d, uint16_t vl_id, unsigned packets)
{
    if (d == NULL) return;
    banner(vl_id, "DPM VL FLOW COUNTERS (RX/TX, kumulatif)", packets);

    uint16_t rx_base = 0, tx_base = 0;
    bool known = dpm_vl_bases(vl_id, &rx_base, &tx_base);
    int  idx   = dpm_index(vl_id);

    // Gösterilecek değerler KÜMÜLATİF toplamdır (paket saniyelik gelir,
    // biriktirici tutar). Bilinmeyen VL için biriktirici slotu yoksa son
    // paketin ham saniyelik değerleri gösterilir.
    uint64_t rx[DPM_VL_PORT_COUNT], tx[DPM_VL_PORT_COUNT];
    if (idx >= 0) {
        for (unsigned i = 0; i < (unsigned)DPM_VL_PORT_COUNT; i++) {
            rx[i] = g_dpm_accum[idx].rx_total[i];
            tx[i] = g_dpm_accum[idx].tx_total[i];
        }
    } else {
        for (unsigned i = 0; i < (unsigned)DPM_VL_PORT_COUNT; i++) {
            rx[i] = d->rx_count[i];
            tx[i] = d->tx_count[i];
        }
    }

    char info[128];
    if (known) {
        snprintf(info, sizeof(info),
                 "RX VL %u..%u (ingress)  TX VL %u..%u (egress)  -  kumulatif toplam (boot'tan beri)",
                 rx_base, (unsigned)(rx_base + DPM_VL_PORT_COUNT - 1),
                 tx_base, (unsigned)(tx_base + DPM_VL_PORT_COUNT - 1));
    } else {
        snprintf(info, sizeof(info),
                 "VL base araligi bilinmiyor - ham saniyelik deger, ham akis index'i (0..%u)",
                 (unsigned)(DPM_VL_PORT_COUNT - 1));
    }
    printf("║  " C_DIM "%-107s" C_RESET " ║\n", info);
    hr();

    // İki yarım sütunlu başlık (her yarı: RXvl, RX count, TXvl, TX count)
    printf("║  " C_BOLD "%-6s %-19s %-6s %-19s" C_RESET " │ "
           C_BOLD "%-6s %-19s %-6s %-19s" C_RESET " ║\n",
           "RXvl", "RX total", "TXvl", "TX total",
           "RXvl", "RX total", "TXvl", "TX total");
    hr();

    const unsigned half = DPM_VL_PORT_COUNT / 2; // 52
    for (unsigned r = 0; r < half; r++) {
        unsigned li = r;          // sol sütundaki akış
        unsigned ri = r + half;   // sağ sütundaki akış
        printf("║  %-6u %-19" PRIu64 " %-6u %-19" PRIu64 " │ %-6u %-19" PRIu64 " %-6u %-19" PRIu64 " ║\n",
               (unsigned)(rx_base + li), rx[li], (unsigned)(tx_base + li), tx[li],
               (unsigned)(rx_base + ri), rx[ri], (unsigned)(tx_base + ri), tx[ri]);
    }

    table_footer();
}
#pragma GCC diagnostic pop

// ============================================================================
// Inter-DPM VL paket kaybı (cross-DPM)
// ----------------------------------------------------------------------------
// Komşu DPM'ler zincir halinde: DPM-N'in TX VL aralığı = DPM-(N+1)'in RX VL
// aralığı ve index hizalı (DPM-N.tx ↔ DPM-(N+1).rx, aynı VL-IDX).
// Kümülatif kayıp = sender.tx_total[i] - receiver.rx_total[i] (biriktirilmiş
// toplamlar üzerinden); yalnızca loss>0 (gerçek kayıp) satırları gösterilir.
//   Eşleşmeyen uçlar (loss tablosuna girmez):
//     DPM-1.RX (10001-10104) — gönderen yok
//     DPM-5.TX (10521-10624) — alıcı yok
// ============================================================================
void print_dpm_vl_loss_table(void)
{
    // Hiç DPM VL paketi biriktirilmediyse tabloyu basma.
    bool any_valid = false;
    for (int k = 0; k < 5; k++) any_valid |= g_dpm_accum[k].valid;
    if (!any_valid) return;

    static const struct {
        uint16_t    sender_vl;   // TX tarafı (DPM-N)
        uint16_t    recv_vl;     // RX tarafı (DPM-N+1)
        const char *label;
    } links[4] = {
        {2021, 2042, "DPM-1->DPM-2"},
        {2042, 2063, "DPM-2->DPM-3"},
        {2063, 2084, "DPM-3->DPM-4"},
        {2084, 2105, "DPM-4->DPM-5"},
    };

    banner_plain("INTER-DPM VL PACKET LOSS  (loss = TX_sender - RX_receiver, sadece loss>0)");

    printf("║  " C_BOLD "%-16s │ %-10s │ %-24s │ %-24s │ %-22s" C_RESET " ║\n",
           "Link", "VL-IDX", "TX (sender)", "RX (receiver)", "Loss");
    hr();

    uint64_t grand_loss = 0;
    unsigned lossy_rows = 0;

    for (int L = 0; L < 4; L++) {
        int si = dpm_index(links[L].sender_vl);
        int ri = dpm_index(links[L].recv_vl);
        if (si < 0 || ri < 0) continue;

        if (!g_dpm_accum[si].valid || !g_dpm_accum[ri].valid) {
            char note[128];
            snprintf(note, sizeof(note), "%s : veri bekleniyor (paket gelmedi)", links[L].label);
            printf("║  " C_DIM "%-107s" C_RESET " ║\n", note);
            continue;
        }

        // Link'in paylaşılan VL-IDX base'i = sender'ın TX base'i.
        uint16_t rx_base = 0, tx_base = 0;
        dpm_vl_bases(links[L].sender_vl, &rx_base, &tx_base);

        for (unsigned i = 0; i < (unsigned)DPM_VL_PORT_COUNT; i++) {
            uint64_t tx = g_dpm_accum[si].tx_total[i];   // kümülatif gönderilen
            uint64_t rx = g_dpm_accum[ri].rx_total[i];   // kümülatif alınan
            if (tx <= rx) continue;                       // sadece loss>0
            uint64_t loss = tx - rx;
            grand_loss += loss;
            lossy_rows++;
            printf("║  %-16s │ %-10u │ %-24" PRIu64 " │ %-24" PRIu64 " │ " C_RED "%-22" PRIu64 C_RESET " ║\n",
                   links[L].label, (unsigned)(tx_base + i), tx, rx, loss);
        }
    }

    if (lossy_rows == 0) {
        printf("║  " C_GREEN "%-107s" C_RESET " ║\n",
               "Tum linkler dengeli — paket kaybi yok.");
    } else {
        char sum[128];
        snprintf(sum, sizeof(sum), "TOPLAM: %u VL'de kayip, toplam %" PRIu64 " paket kayboldu.",
                 lossy_rows, grand_loss);
        printf("║  " C_BOLD C_RED "%-107s" C_RESET " ║\n", sum);
    }

    table_footer();
}

// ============================================================================
// tA664ESMonitoring
// ============================================================================
void print_dtn_es_monitoring(const tA664ESMonitoring *d, uint16_t vl_id, unsigned packets)
{
    if (d == NULL) return;
    banner(vl_id, "DTN ES MONITORING", packets);

    // Sıcaklık özeti deposunu güncelle (en son okunan ES HW sıcaklığı).
    {
        int ti = temp_index(vl_id);
        if (ti >= 0) {
            g_temp[ti].es_hw_temp = (int32_t)d->A664_ES_HW_TEMP;
            g_temp[ti].have_es_hw = true;
        }
    }

    // Karmaşık formatlamalar (string birleştirmeler) için ve 107 limitini
    // korumak için kullanacağımız geçici tampon:
    char buf[128];

    // ========================================================================
    // [ IDENTITY ]
    // ========================================================================
    printf("║  " C_BOLD C_CYAN "%-107s" C_RESET " ║\n", "[ IDENTITY ]");
    
    snprintf(buf, sizeof(buf), "%s   (raw=%" PRIu64 ")", fmt_ver(d->A664_ES_FW_VER), d->A664_ES_FW_VER);
    printf("║  %-30s : %-74s ║\n", "FW_VER", buf);
    printf("║  %-30s : %-74" PRIu64 " ║\n", "DEV_ID", d->A664_ES_DEV_ID);
    printf("║  %-30s : %-74" PRIu64 " ║\n", "MODE", d->A664_ES_MODE);
    printf("║  %-30s : %-74" PRIu64 " ║\n", "CONFIG_ID", d->A664_ES_CONFIG_ID);
    
    snprintf(buf, sizeof(buf), "%" PRIu64 "   (%s)", d->A664_ES_BIT_STATUS, d->A664_ES_BIT_STATUS == 1 ? "PASS" : "FAIL");
    printf("║  %-30s : %-74s ║\n", "BIT_STATUS", buf);
    printf("║  %-30s : %-74" PRIu64 " ║\n", "CONFIG_STATUS", d->A664_ES_CONFIG_STATUS);
    
    snprintf(buf, sizeof(buf), "0x%02X   (ES=%c SW=%c SW-ES=%c)",
           d->A664_BSP_CONFIG_STATUS,
           (d->A664_BSP_CONFIG_STATUS & 0x1) ? 'Y' : '-',
           (d->A664_BSP_CONFIG_STATUS & 0x2) ? 'Y' : '-',
           (d->A664_BSP_CONFIG_STATUS & 0x4) ? 'Y' : '-');
    printf("║  %-30s : %-74s ║\n", "BSP_CONFIG_STATUS", buf);
    printf("║  %-30s : %-74" PRIu64 " ║\n", "VENDOR_TYPE", d->A664_ES_VENDOR_TYPE);
    snprintf(buf, sizeof(buf), "%d   (%s)", (int)d->A664_SW_ES_ENABLE,
             d->A664_SW_ES_ENABLE ? "Switch-ES" : "End System");
    printf("║  %-30s : %-74s ║\n", "SW_ES_ENABLE", buf);

    // ========================================================================
    // [ PTP ]
    // ========================================================================
    hr();
    printf("║  " C_BOLD C_CYAN "%-107s" C_RESET " ║\n", "[ PTP ]");
    printf("║  %-30s : %-74u ║\n", "CONFIG_ID", d->A664_PTP_CONFIG_ID);
    snprintf(buf, sizeof(buf), "%u   (%s)", d->A664_PTP_DEVICE_TYPE, ptp_dev_str(d->A664_PTP_DEVICE_TYPE));
    printf("║  %-30s : %-74s ║\n", "DEVICE_TYPE", buf);
    snprintf(buf, sizeof(buf), "%u   (%s)", d->A664_PTP_RC_STATUS,
             d->A664_PTP_RC_STATUS ? "enabled" : "disabled");
    printf("║  %-30s : %-74s ║\n", "RC_STATUS", buf);
    printf("║  %-30s : %-74u ║\n", "PORT_A_SYNC", d->A664_PTP_PORT_A_SYNC);
    printf("║  %-30s : %-74u ║\n", "PORT_B_SYNC", d->A664_PTP_PORT_B_SYNC);
    printf("║  %-30s : %-74u ║\n", "SYNC_VL_ID", d->A664_PTP_SYNC_VL_ID);
    printf("║  %-30s : %-74u ║\n", "REQ_VL_ID", d->A664_PTP_REQ_VL_ID);
    printf("║  %-30s : %-74u ║\n", "RES_VL_ID", d->A664_PTP_RES_VL_ID);
    printf("║  %-30s : %-74u ║\n", "TOD_NETWORK", d->A664_PTP_TOD_NETWORK);

    // ========================================================================
    // [ HW & LINK ]
    // ========================================================================
    hr();
    printf("║  " C_BOLD C_CYAN "%-107s" C_RESET " ║\n", "[ HW & LINK ]");
    snprintf(buf, sizeof(buf), "%s   (raw=%u)",
             fmt_temp((int32_t)d->A664_ES_HW_TEMP), d->A664_ES_HW_TEMP);
    printf("║  %-30s : %-74s ║\n", "HW_TEMP", buf);
    
    snprintf(buf, sizeof(buf), "%s   (raw=%u)", fmt_vcc(d->A664_ES_HW_VCC_INT), d->A664_ES_HW_VCC_INT);
    printf("║  %-30s : %-74s ║\n", "HW_VCC_INT", buf);
    
    snprintf(buf, sizeof(buf), "%" PRIu64 "   (%s)", d->A664_ES_PORT_SPEED, port_speed_str(d->A664_ES_PORT_SPEED));
    printf("║  %-30s : %-74s ║\n", "PORT_SPEED", buf);
    
    snprintf(buf, sizeof(buf), "%" PRIu64 "   (%s)", d->A664_ES_PORT_A_STATUS, d->A664_ES_PORT_A_STATUS ? "UP" : "DOWN");
    printf("║  %-30s : %-74s ║\n", "PORT_A_STATUS", buf);
    
    snprintf(buf, sizeof(buf), "%" PRIu64 "   (%s)", d->A664_ES_PORT_B_STATUS, d->A664_ES_PORT_B_STATUS ? "UP" : "DOWN");
    printf("║  %-30s : %-74s ║\n", "PORT_B_STATUS", buf);

    // ========================================================================
    // [ TX COUNTERS ]
    // ========================================================================
    hr();
    printf("║  " C_BOLD C_CYAN "%-107s" C_RESET " ║\n", "[ TX COUNTERS ]");
    printf("║  %-30s : %-74" PRIu64 " ║\n", "TX_INCOMING", d->A664_ES_TX_INCOMING_COUNT);
    printf("║  %-30s : %-74" PRIu64 " ║\n", "TX_A_OUTGOING", d->A664_ES_TX_A_OUTGOING_COUNT);
    printf("║  %-30s : %-74" PRIu64 " ║\n", "TX_B_OUTGOING", d->A664_ES_TX_B_OUTGOING_COUNT);
    printf("║  %-30s : %-74" PRIu64 " ║\n", "TX_VLID_DROP", d->A664_ES_TX_VLID_DROP_COUNT);
    printf("║  %-30s : %-74" PRIu64 " ║\n", "TX_LMIN_LMAX_DROP", d->A664_ES_TX_LMIN_LMAX_DROP_COUNT);
    printf("║  %-30s : %-74" PRIu64 " ║\n", "TX_MAX_JITTER_DROP", d->A664_ES_TX_MAX_JITTER_DROP_COUNT);

    // ========================================================================
    // [ RX COUNTERS ]
    // ========================================================================
    hr();
    printf("║  " C_BOLD C_CYAN "%-107s" C_RESET " ║\n", "[ RX COUNTERS ]");
    printf("║  %-30s : %-74" PRIu64 " ║\n", "RX_A_INCOMING", d->A664_ES_RX_A_INCOMING_COUNT);
    printf("║  %-30s : %-74" PRIu64 " ║\n", "RX_B_INCOMING", d->A664_ES_RX_B_INCOMING_COUNT);
    printf("║  %-30s : %-74" PRIu64 " ║\n", "RX_OUTGOING", d->A664_ES_RX_OUTGOING_COUNT);
    
    // --- Port A Drops (Taşmayı önlemek için 3 sütunlu ızgara yapısı) ---
    // ( 8 + 19 ) + ( 11 + 19 ) + ( 7 + 20 ) = 84 < 107 Güvenli!
    printf("║  " C_YELLOW "%-107s" C_RESET " ║\n", "Port A Drops:");
    printf("║      vlid    : %-19" PRIu64 " lmin/lmax : %-19" PRIu64 " net    : %-20" PRIu64 " ║\n",
           d->A664_ES_RX_A_VLID_DROP_COUNT, d->A664_ES_RX_A_LMIN_LMAX_DROP_COUNT, d->A664_ES_RX_A_NET_ERR_COUNT);
    printf("║      seq     : %-19" PRIu64 " crc       : %-19" PRIu64 " ipchk  : %-20" PRIu64 " ║\n",
           d->A664_ES_RX_A_SEQ_ERR_COUNT, d->A664_ES_RX_A_CRC_ERROR_COUNT, d->A664_ES_RX_A_IP_CHECKSUM_ERROR_COUNT);

    // --- Port B Drops ---
    printf("║  " C_YELLOW "%-107s" C_RESET " ║\n", "Port B Drops:");
    printf("║      vlid    : %-19" PRIu64 " lmin/lmax : %-19" PRIu64 " net    : %-20" PRIu64 " ║\n",
           d->A664_ES_RX_B_VLID_DROP_COUNT, d->A664_ES_RX_B_LMIN_LMAX_DROP_COUNT, d->A664_ES_RX_B_NET_ERR_COUNT);
    printf("║      seq     : %-19" PRIu64 " crc       : %-19" PRIu64 " ipchk  : %-20" PRIu64 " ║\n",
           d->A664_ES_RX_B_SEQ_ERR_COUNT, d->A664_ES_RX_B_CRC_ERROR_COUNT, d->A664_ES_RX_B_IP_CHECKSUM_ERROR_COUNT);

    // ========================================================================
    // [ BSP (Driver) ]
    // ========================================================================
    hr();
    printf("║  " C_BOLD C_CYAN "%-107s" C_RESET " ║\n", "[ BSP (DRIVER) ]");
    snprintf(buf, sizeof(buf), "%s   (raw=%" PRIu64 ")", fmt_ver(d->A664_BSP_VER), d->A664_BSP_VER);
    printf("║  %-30s : %-74s ║\n", "VER", buf);
    
    snprintf(buf, sizeof(buf), "pkts=%-15" PRIu64 " bytes=%-15" PRIu64 " errs=%-15" PRIu64, 
             d->A664_BSP_TX_PACKET_COUNT, d->A664_BSP_TX_BYTE_COUNT, d->A664_BSP_TX_ERROR_COUNT);
    printf("║  %-30s : %-74s ║\n", "TX", buf);
    
    snprintf(buf, sizeof(buf), "pkts=%-15" PRIu64 " bytes=%-15" PRIu64 " errs=%-15" PRIu64, 
             d->A664_BSP_RX_PACKET_COUNT, d->A664_BSP_RX_BYTE_COUNT, d->A664_BSP_RX_ERROR_COUNT);
    printf("║  %-30s : %-74s ║\n", "RX (Success)", buf);
    
    printf("║  %-30s : %-74" PRIu64 " ║\n", "RX (Missed Frames)", d->A664_BSP_RX_MISSED_FRAME_COUNT);
    
    // Değişken adı tam 30 karakter. Kalıbımıza %100 oturuyor.
    printf("║  %-30s : %-74" PRIu64 " ║\n", "ES_BSP_QUEUING_RX_VL_PORT_DROP", d->A664_ES_BSP_QUEUING_RX_VL_PORT_DROP_COUNT);

    table_footer();
}

// ============================================================================
// tA664SWMonitoring
// ============================================================================
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
void print_dtn_sw_monitoring(const tA664SWMonitoring *d, uint16_t vl_id, unsigned packets)
{
    if (d == NULL) return;
    banner(vl_id, "DTN SW MONITORING", packets);

    const a664SWMonitoringStatus *s = &d->status;
    char buf[128];

    // Sıcaklık özeti deposunu güncelle (SW HW + transceiver sıcaklıkları).
    {
        int ti = temp_index(vl_id);
        if (ti >= 0) {
            g_temp[ti].sw_temp             = (int32_t)(int16_t)s->A664_SW_TEMPERATURE;
            g_temp[ti].sw_xcvr_temp        = s->A664_SW_TRANSCEIVER_TEMP;
            g_temp[ti].sw_shared_xcvr_temp = s->A664_SW_SHARED_TRANSCEIVER_TEMP;
            g_temp[ti].have_sw             = true;
        }
    }

    // ========================================================================
    // [ STATUS ]
    // ========================================================================
    printf("║  " C_BOLD C_CYAN "%-107s" C_RESET " ║\n", "[ STATUS ]");
    
    snprintf(buf, sizeof(buf), "%s   (raw=%" PRIu64 ")", fmt_ver(s->A664_SW_VERSION), s->A664_SW_VERSION);
    printf("║  %-30s : %-74s ║\n", "SW_VERSION", buf);
    
    snprintf(buf, sizeof(buf), "%s   (raw=%" PRIu64 ")", fmt_ver(s->A664_SW_ES_VERSION), s->A664_SW_ES_VERSION);
    printf("║  %-30s : %-74s ║\n", "SW_ES_VERSION", buf);
    
    printf("║  %-30s : %-74u ║\n", "DEVICE_ID", s->A664_SW_DEVICE_ID);
    printf("║  %-30s : %-74u ║\n", "PORT_NUM", s->A664_SW_PORT_NUM);
    printf("║  %-30s : %-74u ║\n", "CONFIG_ID", s->A664_SW_CONFIG_ID);
    printf("║  %-30s : %-74u ║\n", "HEARTBEAT", s->A664_SW_HEARTBEAT);
    printf("║  %-30s : %-74u ║\n", "CURRENT_MODE", s->A664_SW_CURRENT_MODE);
    snprintf(buf, sizeof(buf), "%u   (%s)", s->A664_SW_TOKEN_BUCKET_STATUS,
             s->A664_SW_TOKEN_BUCKET_STATUS ? "active" : "inactive");
    printf("║  %-30s : %-74s ║\n", "TOKEN_BUCKET", buf);
    snprintf(buf, sizeof(buf), "%u   (%s)", s->A664_SW_AUTOMAC_UPDATE_STATUS,
             s->A664_SW_AUTOMAC_UPDATE_STATUS ? "active" : "inactive");
    printf("║  %-30s : %-74s ║\n", "AUTOMAC_UPDATE", buf);
    snprintf(buf, sizeof(buf), "%u   (%s)", s->A664_SW_UPSTREAM_MODE_STATUS,
             s->A664_SW_UPSTREAM_MODE_STATUS ? "active" : "inactive");
    printf("║  %-30s : %-74s ║\n", "UPSTREAM_MODE", buf);
    printf("║  %-30s : %-74" PRIu64 " ║\n", "VENDOR_TYPE", (uint64_t)s->A664_SW_VENDOR_TYPE);
    
    snprintf(buf, sizeof(buf), "%s   (raw=%u)",
             fmt_temp((int32_t)(int16_t)s->A664_SW_TEMPERATURE), s->A664_SW_TEMPERATURE);
    printf("║  %-30s : %-74s ║\n", "TEMPERATURE", buf);
    
    snprintf(buf, sizeof(buf), "%s   (raw=%u)", fmt_vcc((uint32_t)s->A664_SW_INTERNAL_VOLTAGE), s->A664_SW_INTERNAL_VOLTAGE);
    printf("║  %-30s : %-74s ║\n", "INTERNAL_VOLTAGE", buf);
    
    snprintf(buf, sizeof(buf), "%s   (raw=%" PRIu64 ")",
             fmt_xcvr_temp(s->A664_SW_TRANSCEIVER_TEMP), s->A664_SW_TRANSCEIVER_TEMP);
    printf("║  %-30s : %-74s ║\n", "TRANSCEIVER_TEMP", buf);

    snprintf(buf, sizeof(buf), "%s   (raw=%" PRIu64 ")",
             fmt_xcvr_temp(s->A664_SW_SHARED_TRANSCEIVER_TEMP), s->A664_SW_SHARED_TRANSCEIVER_TEMP);
    printf("║  %-30s : %-74s ║\n", "SHARED_XCVR_TEMP", buf);
    printf("║  %-30s : %-74" PRIu64 " ║\n", "TX_TOTAL", s->A664_SW_TOT_TX_DATA_NUM);
    printf("║  %-30s : %-74" PRIu64 " ║\n", "RX_TOTAL", s->A664_SW_TOT_RX_DATA_NUM);
    
    snprintf(buf, sizeof(buf), "%" PRIu64 " s + %" PRIu64 " ns", s->A664_SW_TIME_OF_DAY_S, s->A664_SW_TIME_OF_DAY_NS);
    printf("║  %-30s : %-74s ║\n", "TIME_OF_DAY", buf);

    // ========================================================================
    // [ PORTS ] (Alt Tablo)
    // ========================================================================
    hr();
#if DEBUG_MODE
    snprintf(buf, sizeof(buf), "[ PORTS ]  (DEBUG: tüm 12 port + tüm sayaçlar)");
#else
    snprintf(buf, sizeof(buf), "[ PORTS ]  (link=DOWN ve tüm sayaçları sıfır olanlar gizlendi)");
#endif
    printf("║  " C_BOLD C_CYAN "%-107s" C_RESET " ║\n", buf);

#if !DEBUG_MODE
    // Normal mod: kompakt tablo. (Debug modda her port ayrı blok basılır.)
    printf("║  " C_BOLD "%-4s" C_RESET " │ " C_BOLD "%-4s" C_RESET " │ " C_BOLD "%-3s" C_RESET " │ " C_BOLD "%-3s" C_RESET " │ "
           C_BOLD "%-20s" C_RESET " │ " C_BOLD "%-20s" C_RESET " │ " C_BOLD "%-16s" C_RESET " │ " C_BOLD "%-16s" C_RESET " ║\n",
           "Port", "ID", "Lnk", "BIT", "TX Frames", "RX Frames", "CRC Errors", "Max Delay Errs");
    printf("║ ──────┼──────┼─────┼─────┼──────────────────────┼──────────────────────┼──────────────────┼────────────────── ║\n");
#endif

    int any = 0;
    for (unsigned i = 0; i < (unsigned)A664_SW_MAX_PORT_COUNT; i++) {
        const a664SWMonitoringPort *p = &d->port[i];
        
        // Port boşta mı kontrolü (Ağ trafiği yoksa ve link down ise çizme)
        uint64_t sum = p->A664_SW_CRC_ERR_CNT | p->A664_SW_TX_FRAME_CNT |
                       p->A664_SW_RX_FRAME_CNT | p->A664_SW_MAC_ERR_CNT |
                       p->A664_SW_TOKEN_ERR_CNT | p->A664_SW_MAX_DELAY_ERR_CNT;
                       
#if !DEBUG_MODE
        // Normal mod: boş portları gizle. Debug modda TÜM portlar basılır.
        // A664_SW_PORT_LINK: 0 = LINK_UP, 1 = LINK_DOWN.
        if (p->A664_SW_PORT_LINK != 0 && sum == 0) continue;
#else
        (void)sum;
#endif
        any = 1;

#if DEBUG_MODE
        // Debug: her port ayrı BLOK. Başlık satırı port kimliğini verir, altında
        // TÜM alanlar (24 veri) tek tip "etiket : değer" olarak — tablo/detay
        // karışıklığı yok, hepsi aynı hizada, eksiksiz (sıfırlar dahil).
        snprintf(buf, sizeof(buf), "[ Port %u ]   ID=%" PRIu64 "   Link=%s   BIT=%u",
                 i, p->A664_SW_PORT_ID,
                 sw_port_link_str(p->A664_SW_PORT_LINK), p->A664_SW_BIT_STATUS);
        printf("║  " C_BOLD C_CYAN "%-107.107s" C_RESET " ║\n", buf);

        #define PORT_ROW(lbl, v) do {                                          \
            snprintf(buf, sizeof(buf), "       %-24s : %" PRIu64,              \
                     (lbl), (uint64_t)(v));                                    \
            printf("║  " C_DIM "%-107.107s" C_RESET " ║\n", buf);              \
        } while (0)
        PORT_ROW("TX Frames",           p->A664_SW_TX_FRAME_CNT);
        PORT_ROW("RX Frames",           p->A664_SW_RX_FRAME_CNT);
        PORT_ROW("CRC Errors",          p->A664_SW_CRC_ERR_CNT);
        PORT_ROW("Max Delay Errs",      p->A664_SW_MAX_DELAY_ERR_CNT);
        PORT_ROW("Alignment Err",       p->A664_SW_ALIGNMENT_ERR_CNT);
        PORT_ROW("Lmin Err",            p->A664_SW_LMIN_ERR_CNT);
        PORT_ROW("Lmax Err",            p->A664_SW_LMAX_ERR_CNT);
        PORT_ROW("VLmin Err",           p->A664_SW_VLMIN_ERR_CNT);
        PORT_ROW("VLmax Err",           p->A664_SW_VLMAX_ERR_CNT);
        PORT_ROW("MAC Err",             p->A664_SW_MAC_ERR_CNT);
        PORT_ROW("Token Err",           p->A664_SW_TOKEN_ERR_CNT);
        PORT_ROW("BE Frames",           p->A664_SW_BE_FRAME);
        PORT_ROW("VL RX Port Err",      p->A664_SW_VL_RX_PORT_ERR);
        PORT_ROW("In Port Q Overflow",  p->A664_SW_IN_PORT_Q_OVERFLOW_CNT);
        PORT_ROW("Undef VL Err",        p->A664_SW_UNDEF_VL_ERR);
        PORT_ROW("Undef BE Err",        p->A664_SW_UNDEF_BE_ERR);
        PORT_ROW("HP Q Overflow",       p->A664_SW_HP_Q_OVERFLOW);
        PORT_ROW("LP Q Overflow",       p->A664_SW_LP_Q_OVERFLOW);
        PORT_ROW("BE Q Overflow",       p->A664_SW_BE_Q_OVERFLOW);
        PORT_ROW("Conf Max Delay",      p->A664_SW_CONF_MAX_DELAY_PARAM);
        #undef PORT_ROW
        snprintf(buf, sizeof(buf), "       %-24s : %" PRIu64 "   (%s)", "Port Speed",
                 p->A664_SW_PORT_SPEED, port_speed_str(p->A664_SW_PORT_SPEED));
        printf("║  " C_DIM "%-107.107s" C_RESET " ║\n", buf);
#else
        // Normal mod: kompakt tablo satırı.
        printf("║  P%-3u │ %-4" PRIu64 " │ %-3s │ %-3u │ %-20" PRIu64 " │ %-20" PRIu64 " │ %-16" PRIu64 " │ %-16" PRIu64 " ║\n",
               i, p->A664_SW_PORT_ID, sw_port_link_short(p->A664_SW_PORT_LINK), p->A664_SW_BIT_STATUS,
               p->A664_SW_TX_FRAME_CNT, p->A664_SW_RX_FRAME_CNT,
               p->A664_SW_CRC_ERR_CNT, p->A664_SW_MAX_DELAY_ERR_CNT);
#endif
    }
    
    if (!any) {
        printf("║  " C_DIM "%-107s" C_RESET " ║\n", "(All 12 ports idle)");
    }

    table_footer();
}
#pragma GCC diagnostic pop

// ============================================================================
// Sıcaklık summary tablosu
// ============================================================================
void print_temperature_summary(void)
{
    bool any = false;
    for (int i = 0; i < 7; i++) any |= (g_temp[i].have_es_hw || g_temp[i].have_sw);
    if (!any) return;

    banner_plain("TEMPERATURE SUMMARY  (tum kaynaklar - en son okunan sicaklik degerleri)");

    printf("║  " C_BOLD "%-8s │ %-22s │ %-22s │ %-22s │ %-22s" C_RESET " ║\n",
           "Source", "ES HW Temp", "SW HW Temp", "Transceiver", "Shared Xcvr");
    hr();

    for (int i = 0; i < 7; i++) {
        const hm_temp_entry_t *t = &g_temp[i];
        if (!t->have_es_hw && !t->have_sw) continue;

        // fmt_temp / fmt_xcvr_temp tek static buffer döner; her hücreyi kendi
        // tamponuna kopyala (tek printf'te birden çok kullanım için).
        char es_s[24], sw_s[24], xc_s[24], sh_s[24];

        if (t->have_es_hw) snprintf(es_s, sizeof es_s, "%s", fmt_temp(t->es_hw_temp));
        else               snprintf(es_s, sizeof es_s, "-");

        if (t->have_sw) {
            snprintf(sw_s, sizeof sw_s, "%s", fmt_temp(t->sw_temp));
            snprintf(xc_s, sizeof xc_s, "%s", fmt_xcvr_temp(t->sw_xcvr_temp));
            snprintf(sh_s, sizeof sh_s, "%s", fmt_xcvr_temp(t->sw_shared_xcvr_temp));
        } else {
            snprintf(sw_s, sizeof sw_s, "-");
            snprintf(xc_s, sizeof xc_s, "-");
            snprintf(sh_s, sizeof sh_s, "-");
        }

        printf("║  %-8s │ %-22s │ %-22s │ %-22s │ %-22s ║\n",
               hm_vl_label(g_temp_vlids[i]), es_s, sw_s, xc_s, sh_s);
    }

    table_footer();
}

// ============================================================================
// CL CMSW status report (VL 18/19) — detaylı bölümler
// ----------------------------------------------------------------------------
// Chassis içindeki her LRM biriminin durumu ayrı alt-bölüm halinde basılır.
// Enum/bit alanları okunur değerlere çevrilir. Bitfield'lar struct'tan doğrudan
// okunur — header BE wire bit sırasını (MSB-first) LE host için ters alan
// tanımıyla zaten karşılıyor; burada yalnızca scalar'lar swap edilmiştir.
// ============================================================================
static const char *cmsw_validity (unsigned b) { return b ? "INVALID"      : "VALID"; }
static const char *cmsw_module   (unsigned b) { return b ? "LOSS"         : "ALIVE"; }
static const char *cmsw_data     (unsigned b) { return b ? "LOSS"         : "AVAILABLE"; }
static const char *cmsw_link     (unsigned b) { return b ? "BAD"          : "GOOD"; }
static const char *cmsw_sync     (unsigned b) { return b ? "NOT_SYNCHED"  : "SYNCHED"; }
static const char *cmsw_temp     (unsigned b) { return b ? "HOT"          : "OK"; }
static const char *cmsw_align    (unsigned b) { return b ? "NOT_ALIGNED"  : "ALIGNED"; }
static const char *cmsw_redun    (unsigned b) { return b ? "LOSS"         : "OK"; }
static const char *cmsw_pbit     (unsigned b) { return b ? "BAD"          : "GOOD"; }
static const char *cmsw_gen_op   (unsigned b) { return b ? "MAINTENANCE"  : "OFP"; }
static const char *cmsw_compliant(unsigned b) { return b ? "NOT_COMPLIANT": "COMPLIANT"; }

static const char *cmsw_chassis_mode(unsigned m)
{
    switch (m) { case 1: return "Standby"; case 2: return "Active"; case 3: return "Unknown"; default: return "?"; }
}
static const char *cmsw_dpm_op(unsigned m)
{
    switch (m) { case 0: return "IDLE"; case 1: return "COLD_START"; case 2: return "WARM_START"; case 3: return "NORMAL"; default: return "?"; }
}
static const char *cmsw_advb(unsigned s)
{
    switch (s) { case 0: return "AVAILABLE"; case 1: return "LOSS"; case 2: return "WARNING"; default: return "?"; }
}
// HSM 2-bit sensor durumlari (0: iyi, 1: veri kaybi, 2: turuncu esik, 3: kirmizi esik)
static const char *cmsw_temp4(unsigned s)
{
    switch (s) { case 0: return "AVAILABLE"; case 1: return "LOSS"; case 2: return "ORANGE"; case 3: return "RED"; default: return "?"; }
}
static const char *cmsw_volt4(unsigned s)
{
    switch (s) { case 0: return "GOOD"; case 1: return "LOSS"; case 2: return "ORANGE"; case 3: return "RED"; default: return "?"; }
}
static const char *cmsw_curr4(unsigned s)
{
    switch (s) { case 0: return "GOOD"; case 1: return "LOSS"; case 2: return "ORANGE"; case 3: return "RED"; default: return "?"; }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"

// LRM tiplerinin ortak başlık bloğu (Cl_cmsw_generic_lrm_status_type)
static void print_cmsw_generic(const Cl_cmsw_generic_lrm_status_type *g)
{
    char buf[128];
    snprintf(buf, sizeof buf, "slot=%u  validity=%s  op_mode=%s  module=%s",
             g->slot_id, cmsw_validity(g->ipmc_data_validity),
             cmsw_gen_op(g->operation_mode), cmsw_module(g->module_status));
    printf("║  %-24s : %-80s ║\n", "LRM Status", buf);

    snprintf(buf, sizeof buf, "%u.%u.%u   (hw_type=%u)",
             g->firmware_version.major, g->firmware_version.minor,
             g->firmware_version.patch, g->hardware_type);
    printf("║  %-24s : %-80s ║\n", "FW Version", buf);

    snprintf(buf, sizeof buf, "pcb_rev=%u  asm_id=%u  mech_rev=%u",
             g->pcb_revision_number, g->pcb_assembly_id, g->mechanical_revision_number);
    printf("║  %-24s : %-80s ║\n", "PCB", buf);

    snprintf(buf, sizeof buf, "serial=0x%016" PRIx64 "  reset_cnt=%u  power=%u",
             g->serial_id, g->reset_counter, g->power_status);
    printf("║  %-24s : %-80s ║\n", "Identity", buf);
}

static void cmsw_section(const char *title)
{
    printf("║  " C_BOLD C_CYAN "%-107s" C_RESET " ║\n", title);
}

void print_clcmsw(const Cl_cmsw_status_report_msg_type *d, uint16_t vl_id, unsigned packets)
{
    if (d == NULL) return;
    banner(vl_id, "CL CMSW STATUS REPORT", packets);

    char buf[128];
    char sect[64];

    // ---- Header (message + device id) ----
    snprintf(buf, sizeof buf, "msg_type=%u  lru_id=%u  slot_id=%u  xmc_fpga=%u",
             d->message_type, d->device_id.lru_id, d->device_id.slot_id, d->device_id.xmc_fpga);
    printf("║  %-24s : %-80s ║\n", "Device", buf);

    // ---- Computer status ----
    hr();
    cmsw_section("[ COMPUTER ]");
    {
        const Cl_cmsw_computer_status_type *c = &d->computer_status;
        snprintf(buf, sizeof buf, "validity=%s  mode=%s  temp=%s  cfg=%s",
                 cmsw_validity(c->chassis_manager_validity),
                 cmsw_chassis_mode(c->chassis_manager_mode),
                 cmsw_temp(c->temperature_status),
                 cmsw_compliant(c->lrm_configuration_status));
        printf("║  %-24s : %-80s ║\n", "Chassis Manager", buf);
        snprintf(buf, sizeof buf, "0x%08X   (bit=slot, 1=MISMATCH)",
                 c->computer_config_mismatch_slot_list);
        printf("║  %-24s : %-80s ║\n", "Cfg Mismatch Slots", buf);
    }

    // ---- PSM / HUM (generic) ----
    for (unsigned i = 0; i < PSM_COUNT_B0; i++) {
        hr(); snprintf(sect, sizeof sect, "[ PSM[%u] ]", i); cmsw_section(sect);
        print_cmsw_generic(&d->psm_status[i]);
    }
    for (unsigned i = 0; i < HUM_COUNT_B0; i++) {
        hr(); snprintf(sect, sizeof sect, "[ HUM[%u] ]", i); cmsw_section(sect);
        print_cmsw_generic(&d->hum_status[i]);
    }

    // ---- SMMM (generic) ----
    hr(); cmsw_section("[ SMMM ]");
    print_cmsw_generic(&d->smmm_status);

    // ---- IOCM ----
    hr(); cmsw_section("[ IOCM ]");
    print_cmsw_generic(&d->iocm_status.lrm_status);
    snprintf(buf, sizeof buf, "dvi=%s  voltage=%s  temperature=%s",
             cmsw_data(d->iocm_status.dvi_status),
             cmsw_data(d->iocm_status.voltage_data_status),
             cmsw_data(d->iocm_status.temperature_data_status));
    printf("║  %-24s : %-80s ║\n", "IOCM Data", buf);

    // ---- DPM[5] ----
    for (unsigned i = 0; i < DPM_COUNT_B0; i++) {
        const Cl_cmsw_dpm_status_type *p = &d->dpm_status[i];
        hr(); snprintf(sect, sizeof sect, "[ DPM[%u] ]", i); cmsw_section(sect);
        print_cmsw_generic(&p->lrm_status);

        snprintf(buf, sizeof buf, "voltage=%s  temp=%s  es_valid=%s  ptp=%s",
                 cmsw_data(p->voltage_data_status), cmsw_data(p->temperature_data_status),
                 cmsw_validity(p->dtn_es_data_validity), cmsw_sync(p->ptp_sync_status));
        printf("║  %-24s : %-80s ║\n", "Status A", buf);
        snprintf(buf, sizeof buf, "hsn_link=%s  mfa=%s  es_redun=%s  mount=%s",
                 cmsw_link(p->hsn_link_status), cmsw_align(p->major_frame_alignment_status),
                 cmsw_redun(p->dtn_es_redundancy_status), cmsw_validity(p->client_mount_validity));
        printf("║  %-24s : %-80s ║\n", "Status B", buf);
        snprintf(buf, sizeof buf, "op_mode=%s  ml_valid=%s  a653_sched=%d",
                 cmsw_dpm_op(p->operation_mode), cmsw_validity(p->ml_cmsw_msg_data_validity),
                 p->a653_schedule_id);
        printf("║  %-24s : %-80s ║\n", "Operation", buf);
        snprintf(buf, sizeof buf, "es_cfg=%u  ptp_cfg=%u  ptp_dev=%s  tod_net=%u  monolith=%u",
                 p->dtn_es_cfg_id, p->ptp_cfg_id, ptp_dev_str(p->ptp_device_type),
                 p->ptp_tod_network, p->monolith_id);
        printf("║  %-24s : %-80s ║\n", "Config / PTP", buf);
        snprintf(buf, sizeof buf, "es_fw=%u.%u.%u  timestamp=%" PRIu64,
                 p->dtn_es_fw_vers_major, p->dtn_es_fw_vers_minor, p->dtn_es_fw_vers_bugfix,
                 p->timestamp);
        printf("║  %-24s : %-80s ║\n", "ES FW / Time", buf);
        snprintf(buf, sizeof buf, "ddr=%s ifc_nand=%s serdes1_pll1=%s serdes2_pll1=%s serdes2_pll2=%s",
                 cmsw_pbit(p->t2080_dpm_bootloader_pbit.ddr_test),
                 cmsw_pbit(p->t2080_dpm_bootloader_pbit.ifc_nand_test),
                 cmsw_pbit(p->t2080_dpm_bootloader_pbit.serdes1_pll1_test),
                 cmsw_pbit(p->t2080_dpm_bootloader_pbit.serdes2_pll1_test),
                 cmsw_pbit(p->t2080_dpm_bootloader_pbit.serdes2_pll2_test));
        printf("║  %-24s : %-80s ║\n", "PBIT", buf);
    }

    // ---- DSM[2] ----
    for (unsigned i = 0; i < DSM_COUNT_B0; i++) {
        const Cl_cmsw_dsm_status_type *p = &d->dsm_status[i];
        hr(); snprintf(sect, sizeof sect, "[ DSM[%u] ]", i); cmsw_section(sect);
        print_cmsw_generic(&p->lrm_status);

        snprintf(buf, sizeof buf, "voltage=%s  temp=%s  es_valid=%s  sw_valid=%s",
                 cmsw_data(p->voltage_data_status), cmsw_data(p->temperature_data_status),
                 cmsw_validity(p->dtn_es_data_validity), cmsw_validity(p->dtn_sw_data_validity));
        printf("║  %-24s : %-80s ║\n", "Status A", buf);
        snprintf(buf, sizeof buf, "ptp=%s  mfa=%s  es_redun=%s",
                 cmsw_sync(p->ptp_sync_status), cmsw_align(p->major_frame_alignment_status),
                 cmsw_redun(p->dtn_es_redundancy_status));
        printf("║  %-24s : %-80s ║\n", "Status B", buf);
        snprintf(buf, sizeof buf, "a653_sched=%d  es_cfg=%u  sw_cfg=%u  ptp_cfg=%u  ptp_dev=%s",
                 p->a653_schedule_id, p->dtn_es_cfg_id, p->dtn_sw_cfg_id, p->ptp_cfg_id,
                 ptp_dev_str(p->ptp_device_type));
        printf("║  %-24s : %-80s ║\n", "Config / PTP", buf);
        snprintf(buf, sizeof buf, "es_fw=%u.%u.%u  sw_fw=%u.%u.%u  sw_port_link=0x%04X",
                 p->dtn_es_fw_vers_major, p->dtn_es_fw_vers_minor, p->dtn_es_fw_vers_bugfix,
                 p->dtn_sw_fw_vers_major, p->dtn_sw_fw_vers_minor, p->dtn_sw_fw_vers_bugfix,
                 p->dtn_sw_port_link_status);
        printf("║  %-24s : %-80s ║\n", "ES/SW FW / Link", buf);
        snprintf(buf, sizeof buf, "ddr=%s ifc_nand=%s serdes1_pll1=%s serdes1_pll2=%s",
                 cmsw_pbit(p->t2080_dsm_bootloader_pbit.ddr_test),
                 cmsw_pbit(p->t2080_dsm_bootloader_pbit.ifc_nand_test),
                 cmsw_pbit(p->t2080_dsm_bootloader_pbit.serdes1_pll1_test),
                 cmsw_pbit(p->t2080_dsm_bootloader_pbit.serdes1_pll2_test));
        printf("║  %-24s : %-80s ║\n", "PBIT", buf);
    }

    // ---- GPM[2] ----
    for (unsigned i = 0; i < GPM_COUNT_B0; i++) {
        const Cl_cmsw_gpm_status_type *p = &d->gpm_status[i];
        hr(); snprintf(sect, sizeof sect, "[ GPM[%u] ]", i); cmsw_section(sect);
        print_cmsw_generic(&p->lrm_status);

        snprintf(buf, sizeof buf, "advb_tx1=%s  advb_tx2=%s  advb_tx3=%s",
                 cmsw_advb(p->advb_tx_1_status), cmsw_advb(p->advb_tx_2_status),
                 cmsw_advb(p->advb_tx_3_status));
        printf("║  %-24s : %-80s ║\n", "ADVB TX", buf);
        snprintf(buf, sizeof buf, "advb_hm=%s  advb=%s  dvi=%s  voltage=%s  temp=%s",
                 cmsw_data(p->advb_hm_data_status), cmsw_data(p->advb_status),
                 cmsw_advb(p->dvi_status), cmsw_data(p->voltage_data_status),
                 cmsw_data(p->temperature_data_status));
        printf("║  %-24s : %-80s ║\n", "ADVB / Data", buf);
    }

    // ---- HSM[2] ----
    for (unsigned i = 0; i < HSM_COUNT_B0; i++) {
        const Cl_cmsw_hsm_status_type *p = &d->hsm_status[i];
        hr(); snprintf(sect, sizeof sect, "[ HSM[%u] ]", i); cmsw_section(sect);
        print_cmsw_generic(&p->lrm_status);

        snprintf(buf, sizeof buf, "hsm_valid=%s  pcie_link=%s",
                 cmsw_validity(p->hsm_data_validity), cmsw_link(p->dsm_hsm_pcie_link_status));
        printf("║  %-24s : %-80s ║\n", "HSM Data", buf);
        snprintf(buf, sizeof buf, "temp=%s  voltage=%s  current=%s",
                 cmsw_temp4(p->temperature_data_status), cmsw_volt4(p->voltage_data_status),
                 cmsw_curr4(p->current_data_status));
        printf("║  %-24s : %-80s ║\n", "HSM Sensors", buf);

        // 8 backplane HSN portu — tek satırda özet
        char ports[128];
        int off = 0;
        for (unsigned k = 0; k < 8; k++) {
            const Cl_cmsw_hsn_port_link_status_type *hp = &p->backplane_hsn_port_status_list[k];
            off += snprintf(ports + off, sizeof(ports) - (size_t)off, "P%u:%s%s ",
                            hp->port_number, cmsw_link(hp->hsn_link_status),
                            hp->data_validity ? "!" : "");
            if (off >= (int)sizeof(ports) - 12) break;
        }
        printf("║  %-24s : %-80s ║\n", "HSN Ports (link,!=inv)", ports);
    }

    table_footer();
}
#pragma GCC diagnostic pop

// ============================================================================
// SMMM monitoring (VL 2021)
// ----------------------------------------------------------------------------
// voltage / current / temperature raw 16-bit ADC kodudur; ekrana ADC dönüşüm
// formülleriyle (fiziksel birim) basılır. bit_status / alarm_status bit
// alanları ve batarya seviyesi okunur değerlere çevrilir.
// ============================================================================
static const char *smmm_batt(unsigned v)
{
    switch (v) {
        case smmm_empty:    return "EMPTY";
        case smmm_critical: return "CRITICAL";
        case smmm_low:      return "LOW";
        case smmm_medium:   return "MEDIUM";
        case smmm_high:     return "HIGH";
        default:            return "?";
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
void print_smmm(const Smmm_monitoring_data_t *s, uint16_t vl_id, unsigned packets)
{
    if (s == NULL) return;
    banner(vl_id, "SMMM MONITORING", packets);

    char buf[128];

    // ---- Sensors (ADC → fiziksel birim) ----
    printf("║  " C_BOLD C_CYAN "%-107s" C_RESET " ║\n", "[ SENSORS ]");
    snprintf(buf, sizeof buf, "%.2f C   (raw=%u)", SM_HW_TEMP_ADC_CODE(s->temperature), s->temperature);
    printf("║  %-30s : %-74s ║\n", "Temperature", buf);
    snprintf(buf, sizeof buf, "%.4f V   (raw=%u)", SM_HW_VCC_ADC_CODE(s->voltage), s->voltage);
    printf("║  %-30s : %-74s ║\n", "Voltage", buf);
    snprintf(buf, sizeof buf, "%.4f A   (raw=%u)", SM_HW_VCC_ADC_CODE(s->current), s->current);
    printf("║  %-30s : %-74s ║\n", "Current", buf);

    // ---- Identity / state ----
    hr();
    printf("║  " C_BOLD C_CYAN "%-107s" C_RESET " ║\n", "[ IDENTITY / STATE ]");
    printf("║  %-30s : %-74u ║\n", "SMMM Version", s->smmm_version);
    printf("║  %-30s : %-74u ║\n", "HW ID", s->hw_id);
    snprintf(buf, sizeof buf, "sec_ctrl=%u  ipmc=%u", s->security_controller_fw_version, s->ipmc_fw_version);
    printf("║  %-30s : %-74s ║\n", "FW Versions", buf);
    printf("║  %-30s : %-74u ║\n", "Current State", s->current_state);
    printf("║  %-30s : %-74u ║\n", "Sanitization Status", s->sanitization_status);
    snprintf(buf, sizeof buf, "%s   (0x%02X)", smmm_batt((unsigned)s->battery_level), (unsigned)s->battery_level);
    printf("║  %-30s : %-74s ║\n", "Battery Level", buf);
    printf("║  %-30s : %-74u ║\n", "Storage Lock", s->smmm_storage_lock_status);
    printf("║  %-30s : %-74u ║\n", "Pilot Auth Status", s->latest_pilot_auth_status);

    // ---- Time ----
    hr();
    printf("║  " C_BOLD C_CYAN "%-107s" C_RESET " ║\n", "[ TIME ]");
    snprintf(buf, sizeof buf, "%02u/%02u/20%02u  %02u:%02u:%02u  (dd/mm/yyyy hh:mm:ss)",
             s->smmm_time.date.day, s->smmm_time.date.month, s->smmm_time.date.year,
             s->smmm_time.time.hours, s->smmm_time.time.minutes, s->smmm_time.time.seconds);
    printf("║  %-30s : %-74s ║\n", "RTC", buf);
    snprintf(buf, sizeof buf, "%" PRIu64, s->smmm_tod);
    printf("║  %-30s : %-74s ║\n", "Time of Day (ToD)", buf);
    snprintf(buf, sizeof buf, "%" PRIu64, s->pilot_auth_status_occurance_time);
    printf("║  %-30s : %-74s ║\n", "Pilot Auth Time", buf);

    // ---- BIT status ----
    hr();
    printf("║  " C_BOLD C_CYAN "%-107s" C_RESET " ║\n", "[ BIT STATUS ]");
    snprintf(buf, sizeof buf, "backplane=%u mass_storage=%u dtn0=%u dtn1=%u uart0=%u uart1=%u",
             s->bit_status.backplane, s->bit_status.mass_storage, s->bit_status.dtn_0,
             s->bit_status.dtn_1, s->bit_status.uart_0, s->bit_status.uart_1);
    printf("║  %-30s : %-74s ║\n", "Interfaces", buf);
    snprintf(buf, sizeof buf, "f_o=%u algorithm=%u key=%u trng=%u",
             s->bit_status.f_o, s->bit_status.algorithm, s->bit_status.key, s->bit_status.trng);
    printf("║  %-30s : %-74s ║\n", "Crypto/Optic", buf);

    // ---- Alarm status ----
    hr();
    printf("║  " C_BOLD C_CYAN "%-107s" C_RESET " ║\n", "[ ALARM STATUS ]");
    snprintf(buf, sizeof buf, "int_tamper=%u ext_tamper=%u emergency_zero=%u batt_low=%u",
             s->alarm_status.internal_tamper, s->alarm_status.external_tamper,
             s->alarm_status.emergency_zeroization, s->alarm_status.battery_voltage_low);
    printf("║  %-30s : %-74s ║\n", "Tamper / Battery", buf);
    snprintf(buf, sizeof buf, "high_temp=%u low_psu=%u high_psu=%u crypto_key=%u crypto_algo=%u",
             s->alarm_status.high_module_temp, s->alarm_status.low_psu_voltage,
             s->alarm_status.high_psu_voltage, s->alarm_status.crypto_key_integrty,
             s->alarm_status.crypto_algorithm);
    printf("║  %-30s : %-74s ║\n", "Temp / PSU / Crypto", buf);

    // ---- Crypto status ----
    hr();
    printf("║  " C_BOLD C_CYAN "%-107s" C_RESET " ║\n", "[ CRYPTO STATUS ]");
    snprintf(buf, sizeof buf, "[0]=0x%016" PRIx64 "  [1]=0x%016" PRIx64,
             s->crypto_status[0], s->crypto_status[1]);
    printf("║  %-30s : %-74s ║\n", "Crypto 0..1", buf);
    snprintf(buf, sizeof buf, "[2]=0x%016" PRIx64 "  [3]=0x%016" PRIx64,
             s->crypto_status[2], s->crypto_status[3]);
    printf("║  %-30s : %-74s ║\n", "Crypto 2..3", buf);

    // ---- Counters ----
    hr();
    printf("║  " C_BOLD C_CYAN "%-107s" C_RESET " ║\n", "[ COUNTERS ]");
    snprintf(buf, sizeof buf, "rx_sectors=%" PRIu64 "  tx_sectors=%" PRIu64,
             s->nvme_rx_count, s->nvme_tx_count);
    printf("║  %-30s : %-74s ║\n", "NVMe", buf);
    snprintf(buf, sizeof buf, "rx=%" PRIu64 "  tx=%" PRIu64, s->eth_rx_count, s->eth_tx_count);
    printf("║  %-30s : %-74s ║\n", "Ethernet RX/TX", buf);
    snprintf(buf, sizeof buf, "drop=%" PRIu64 "  header_err=%" PRIu64,
             s->eth_packet_drop_count, s->eth_header_error_count);
    printf("║  %-30s : %-74s ║\n", "Ethernet Errors", buf);

    table_footer();
}
#pragma GCC diagnostic pop

// ============================================================================
// CL CMSW IPMC log (VL 50) — DPM/DSM/GPM tam board data: store + tablo
// ----------------------------------------------------------------------------
// Ham payload device_id bazında saklanır (tek thread: dashboard). Parse burada,
// descriptor tablosuyla yapılır: her board için alanlar wire SIRASIYLA listeli;
// wire cursor ile yürünür (enum=4B, value=10B [int16 + is_specified u32 +
// validity_region u32], big-endian). Board etiketi "<TİP>-<no>",
// no = (device_id % 256) / 8 (üretici ile aynı).
//   DEBUG_MODE=0 : kimlik + sıcaklıklar + kısa sağlık özeti.
//   DEBUG_MODE=1 : tüm alanlar (pgood/valid/temperature/voltage/current/other).
// ============================================================================

// --- big-endian okuyucular ---
static inline uint16_t ip_be16 (const uint8_t *p){ return (uint16_t)(((uint16_t)p[0]<<8)|p[1]); }
static inline int16_t  ip_be16s(const uint8_t *p){ return (int16_t)(((uint16_t)p[0]<<8)|p[1]); }
static inline uint32_t ip_be32 (const uint8_t *p){ return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3]; }

static const char *ipmc_lrm_name(uint32_t d)
{
    switch (d) {
        case 1: return "DSM";  case 2: return "DPM";   case 3: return "HSM";
        case 4: return "PSM";  case 5: return "HUM";   case 6: return "GPM";
        case 7: return "IO";   case 8: return "SMMM";  case 9: return "HPM";
        case 10: return "SPM"; case 11: return "GPGPM";
        default: return "LRM";
    }
}
static const char *ipmc_power_name(uint32_t p)
{
    switch (p) {
        case 1: return "ON"; case 2: return "OFF"; case 3: return "WAITING";
        case 4: return "PARTIAL"; default: return "?";
    }
}
static const char *ipmc_region_name(unsigned region)
{
    switch (region) {
        case 0: return "invalid"; case 1: return "green";
        case 2: return "orange";  case 3: return "red"; default: return "?";
    }
}
static const char *ipmc_opmode_name(uint32_t m) { return m == 0 ? "OFP" : m == 1 ? "MAINTENANCE" : "?"; }
static const char *ipmc_bus_name   (uint32_t b) { return b == 0 ? "HEALTHY" : b == 1 ? "CONN_TIMEOUT" : "?"; }

// --- alan descriptor'ları ---
typedef enum { IF_GOOD, IF_VALID, IF_TEMP, IF_RAW } ipmc_field_kind;
typedef struct { const char *name; ipmc_field_kind kind; } ipmc_field_t;
typedef struct { const char *title; const ipmc_field_t *fields; int count; } ipmc_section_t;
typedef struct { const ipmc_section_t *sections; int nsec; } ipmc_layout_t;

#define IPMC_SEC(t, arr) { (t), (arr), (int)(sizeof(arr)/sizeof((arr)[0])) }

// -------- DPM (wire sırası: pgood, valid, temp, current, voltage, other, other_valid) --------
static const ipmc_field_t dpm_pgood[] = {
    {"dtn_es_1v0_pg",IF_GOOD},{"dtn_es_1v2_pg",IF_GOOD},{"dtn_es_vdda_1v_pg",IF_GOOD},
    {"tps_pgood_dtn_es_vcc3v3",IF_GOOD},{"dtn_es_vddix_3v3_pgood",IF_GOOD},{"dtn_es_2v5_pg",IF_GOOD},
    {"dtn_es_1v8_pg",IF_GOOD},{"dtn_es_vddi_1v8_pg",IF_GOOD},{"phy_pgood_dvdd_1v",IF_GOOD},
    {"vtt_vref_pg",IF_GOOD},{"tps_pgood_cpu_s1vdd",IF_GOOD},{"tps_pgood_cpu_s2vdd",IF_GOOD},
    {"dtn_es_vddi_2v5_pg",IF_GOOD},{"bias_pgood_5v",IF_GOOD},{"tpsm_pgood_cpu_vcc1v35",IF_GOOD},
    {"tpsm_pgood_cpu_vcc1v8",IF_GOOD},{"prog_sfp_pgood",IF_GOOD},{"tpsm_pgood_cpu_vcc1v2",IF_GOOD},
};
static const ipmc_field_t dpm_valid[] = {
    {"dtn_es_1v0_pg_valid",IF_VALID},{"dtn_es_1v2_pg_valid",IF_VALID},{"dtn_es_vdda_1v_pg_valid",IF_VALID},
    {"tps_pgood_dtn_es_vcc3v3_valid",IF_VALID},{"dtn_es_vddix_3v3_pgood_valid",IF_VALID},{"dtn_es_2v5_pg_valid",IF_VALID},
    {"dtn_es_1v8_pg_valid",IF_VALID},{"dtn_es_vddi_1v8_pg_valid",IF_VALID},{"phy_pgood_dvdd_1v_valid",IF_VALID},
    {"vtt_vref_pg_valid",IF_VALID},{"tps_pgood_cpu_s1vdd_valid",IF_VALID},{"tps_pgood_cpu_s2vdd_valid",IF_VALID},
    {"dtn_es_vddi_2v5_pg_valid",IF_VALID},{"bias_pgood_5v_valid",IF_VALID},{"tpsm_pgood_cpu_vcc1v35_valid",IF_VALID},
    {"tpsm_pgood_cpu_vcc1v8_valid",IF_VALID},{"prog_sfp_pgood_valid",IF_VALID},{"tpsm_pgood_cpu_vcc1v2_valid",IF_VALID},
};
static const ipmc_field_t dpm_temp[] = {
    {"tmp451_sensor_for_cpu",IF_TEMP},{"tmp451_sensor_for_fpga",IF_TEMP},
};
static const ipmc_field_t dpm_current[] = {
    {"dtn_es_1v0_cs",IF_RAW},{"dtn_es_vddi_2v5_cs",IF_RAW},{"dtn_es_vdda_1v_cs",IF_RAW},{"dtn_es_vcc3v3_cs",IF_RAW},
    {"dtn_es_2v5_cs",IF_RAW},{"dtn_es_vdd_1v8_cs",IF_RAW},{"dtn_es_vddi_1v8_cs",IF_RAW},{"cpu_1v2_cs",IF_RAW},
    {"cpu_1v8_cs",IF_RAW},{"cpu_1v35_cs",IF_RAW},{"cpu_s1vdd_cs",IF_RAW},{"cpu_s2vdd_cs",IF_RAW},{"vcore_imon",IF_RAW},
};
static const ipmc_field_t dpm_voltage[] = {
    {"cpu_1v8",IF_RAW},{"cpu_1v35",IF_RAW},{"cpu_s1vdd",IF_RAW},{"cpu_s2vdd",IF_RAW},{"dtn_es_1v0",IF_RAW},
    {"dtn_es_vddi_2v5",IF_RAW},{"dtn_es_vdda_1v",IF_RAW},{"dtn_es_vcc3v3",IF_RAW},{"dtn_es_2v5",IF_RAW},
    {"dtn_es_vdd_1v8",IF_RAW},{"dtn_es_vddi_1v8",IF_RAW},{"cpu_1v2",IF_RAW},
};
static const ipmc_field_t dpm_other[] = {
    {"ipmbb_ready",IF_GOOD},{"ipmba_ready",IF_GOOD},{"clk_los",IF_GOOD},{"clk_gen_lol",IF_GOOD},{"mcu_asleep",IF_GOOD},
    {"vcore_rdy",IF_GOOD},{"vcore_fault",IF_GOOD},{"vcore_temp",IF_GOOD},{"ddr_test",IF_GOOD},
    {"serdes1_pll1_test",IF_GOOD},{"serdes2_pll1_test",IF_GOOD},{"serdes2_pll2_test",IF_GOOD},{"ifc_nand_test",IF_GOOD},
};
static const ipmc_field_t dpm_other_valid[] = {
    {"ipmbb_ready_valid",IF_VALID},{"ipmba_ready_valid",IF_VALID},{"clk_los_valid",IF_VALID},{"clk_gen_lol_valid",IF_VALID},
    {"mcu_asleep_valid",IF_VALID},{"vcore_rdy_valid",IF_VALID},{"vcore_fault_valid",IF_VALID},{"vcore_temp_valid",IF_VALID},
};
static const ipmc_section_t dpm_sections[] = {
    IPMC_SEC("Power Good", dpm_pgood), IPMC_SEC("Valid", dpm_valid),
    IPMC_SEC("Temperature (1/100 C)", dpm_temp), IPMC_SEC("Current (raw)", dpm_current),
    IPMC_SEC("Voltage (raw)", dpm_voltage), IPMC_SEC("Other", dpm_other),
    IPMC_SEC("Other Valid", dpm_other_valid),
};

// -------- GPM (wire sırası: pgood, valid, temp, voltage, current, other, other_valid) --------
static const ipmc_field_t gpm_pgood[] = {
    {"pg_3v3_fo",IF_GOOD},{"pg_0v9_gpu_core",IF_GOOD},{"pg_0v9_gpu_vddci",IF_GOOD},{"pg_1v35_gpu_vmeio",IF_GOOD},
    {"pg_1v8_gpu",IF_GOOD},{"pg_3v3_gpu",IF_GOOD},{"pg_0v85_fpga_vccint",IF_GOOD},{"pg_0v9_fpga_mgtavcc",IF_GOOD},
    {"pg_1v2_fpga_mgtavtt",IF_GOOD},{"pg_1v8_fpga_vcco",IF_GOOD},{"pg_1v8_fpga_mgtvccaux",IF_GOOD},
    {"pg_1v2_fpga_ddr",IF_GOOD},{"pg_3v3_fpga_vcco",IF_GOOD},{"pg_3v3_clock",IF_GOOD},{"pg_1v8_clock",IF_GOOD},
};
static const ipmc_field_t gpm_valid[] = {
    {"pg_3v3_fo_valid",IF_VALID},{"pg_0v9_gpu_core_valid",IF_VALID},{"pg_0v9_gpu_vddci_valid",IF_VALID},{"pg_1v35_gpu_vmeio_valid",IF_VALID},
    {"pg_1v8_gpu_valid",IF_VALID},{"pg_3v3_gpu_valid",IF_VALID},{"pg_0v85_fpga_vccint_valid",IF_VALID},{"pg_0v9_fpga_mgtavcc_valid",IF_VALID},
    {"pg_1v2_fpga_mgtavtt_valid",IF_VALID},{"pg_1v8_fpga_vcco_valid",IF_VALID},{"pg_1v8_fpga_mgtvccaux_valid",IF_VALID},
    {"pg_1v2_fpga_ddr_valid",IF_VALID},{"pg_3v3_fpga_vcco_valid",IF_VALID},{"pg_3v3_clock_valid",IF_VALID},{"pg_1v8_clock_valid",IF_VALID},
};
static const ipmc_field_t gpm_temp[] = {
    {"tmp451_sensor_1",IF_TEMP},{"tmp451_sensor_2",IF_TEMP},{"tmp451_sensor_3",IF_TEMP},{"tmp451_sensor_4",IF_TEMP},
};
static const ipmc_field_t gpm_voltage[] = {
    {"vols_3v3_fo",IF_RAW},{"plus_3v3_gpu",IF_RAW},{"plus_0v9_gpu_core",IF_RAW},{"plus_0v9_gpu_vddci",IF_RAW},
    {"plus_1v8_gpu",IF_RAW},{"plus_1v35_gpu_vmeio",IF_RAW},{"plus_0v85_fpga_vccint",IF_RAW},{"plus_0v9_fpga_mgtavcc",IF_RAW},
    {"plus_1v2_fpga_mgtavtt",IF_RAW},{"plus_3v3_fpga",IF_RAW},{"plus_1v2_fpga_ddr",IF_RAW},{"plus_1v8_fpga_mgtvccaux",IF_RAW},
    {"plus_1v8_fpga_vcco",IF_RAW},{"plus_3v3_clock",IF_RAW},{"plus_1v8_clock",IF_RAW},
};
static const ipmc_field_t gpm_current[] = {
    {"curs_3v3_fo",IF_RAW},{"curs_5v0",IF_RAW},{"curs_12v0",IF_RAW},{"curs_3v3_gpu",IF_RAW},{"curs_0v9_gpu_core",IF_RAW},
    {"curs_0v9_gpu_vddci",IF_RAW},{"curs_1v8_gpu",IF_RAW},{"curs_1v35_gpu_vmeio",IF_RAW},{"curs_0v85_fpga_vccint",IF_RAW},
    {"curs_0v9_fpga_mgtavcc",IF_RAW},{"curs_1v2_fpga_mgtavtt",IF_RAW},{"curs_3v3_fpga_vcco",IF_RAW},{"curs_1v2_fpga_ddr",IF_RAW},
    {"curs_1v8_fpga_mgtvccaux",IF_RAW},{"curs_1v8_fpga_vcco",IF_RAW},{"curs_3v3_clock",IF_RAW},{"curs_1v8_clock",IF_RAW},
};
static const ipmc_field_t gpm_other[] = {
    {"lol_clk_gen",IF_GOOD},{"fpga_init_3v3",IF_GOOD},{"fpga_done_3v3",IF_GOOD},{"los_clk_buf",IF_GOOD},
    {"ready_ipmba",IF_GOOD},{"ready_ipmbb",IF_GOOD},
};
static const ipmc_field_t gpm_other_valid[] = {
    {"lol_clk_gen_valid",IF_VALID},{"fpga_init_3v3_valid",IF_VALID},{"fpga_done_3v3_valid",IF_VALID},
    {"los_clk_buf_valid",IF_VALID},{"ready_ipmba_valid",IF_VALID},{"ready_ipmbb_valid",IF_VALID},
};
static const ipmc_section_t gpm_sections[] = {
    IPMC_SEC("Power Good", gpm_pgood), IPMC_SEC("Valid", gpm_valid),
    IPMC_SEC("Temperature (1/100 C)", gpm_temp), IPMC_SEC("Voltage (raw)", gpm_voltage),
    IPMC_SEC("Current (raw)", gpm_current), IPMC_SEC("Other", gpm_other),
    IPMC_SEC("Other Valid", gpm_other_valid),
};

// -------- DSM (wire sırası: pgood, valid, temp, voltage, current, other, other_valid) --------
static const ipmc_field_t dsm_pgood[] = {
    {"phy_pgood_dvdd_1v",IF_GOOD},{"bias_pgood_5v",IF_GOOD},{"refclk_vdd1v8_pg",IF_GOOD},{"refclk_vddo_pg",IF_GOOD},
    {"refclk_fod_pg",IF_GOOD},{"refclk_vdd1_3v3_pg",IF_GOOD},{"refclk_vdd2_3v3_pg",IF_GOOD},{"onepps_vdd1v8_pg",IF_GOOD},
    {"onepps_vddo_pg",IF_GOOD},{"onepps_fod_pg",IF_GOOD},{"onepps_vdd1_3v3_pg",IF_GOOD},{"onepps_vdd2_3v3_pg",IF_GOOD},
    {"dtn_es_1v2_pg",IF_GOOD},{"dtn_es_vdda_1v_pg",IF_GOOD},{"tps_pgood_dtn_es_vcc3v3",IF_GOOD},{"dtn_es_vddix_3v3_pgood",IF_GOOD},
    {"dtn_es_2v5_pg",IF_GOOD},{"dtn_es_1v8_pg",IF_GOOD},{"dtn_es_vddi_1v8_pg",IF_GOOD},{"dtn_es_1v0_pg",IF_GOOD},
    {"vtt_vref_pg",IF_GOOD},{"tps_pgood_cpu_s1vdd",IF_GOOD},{"tps_pgood_cpu_s2vdd",IF_GOOD},{"tpsm_pgood_cpu_vcc1v35",IF_GOOD},
    {"tpsm_pgood_cpu_vcc1v8",IF_GOOD},{"prog_sfp_pgood",IF_GOOD},{"tpsm_pgood_cpu_vcc1v2",IF_GOOD},{"dtn_es_vddi_2v5_pg",IF_GOOD},
};
static const ipmc_field_t dsm_valid[] = {
    {"phy_pgood_dvdd_1v_valid",IF_VALID},{"bias_pgood_5v_valid",IF_VALID},{"refclk_vdd1v8_pg_valid",IF_VALID},{"refclk_vddo_pg_valid",IF_VALID},
    {"refclk_fod_pg_valid",IF_VALID},{"refclk_vdd1_3v3_pg_valid",IF_VALID},{"refclk_vdd2_3v3_pg_valid",IF_VALID},{"onepps_vdd1v8_pg_valid",IF_VALID},
    {"onepps_vddo_pg_valid",IF_VALID},{"onepps_fod_pg_valid",IF_VALID},{"onepps_vdd1_3v3_pg_valid",IF_VALID},{"onepps_vdd2_3v3_pg_valid",IF_VALID},
    {"dtn_es_1v2_pg_valid",IF_VALID},{"dtn_es_vdda_1v_pg_valid",IF_VALID},{"tps_pgood_dtn_es_vcc3v3_valid",IF_VALID},{"dtn_es_vddix_3v3_pgood_valid",IF_VALID},
    {"dtn_es_2v5_pg_valid",IF_VALID},{"dtn_es_1v8_pg_valid",IF_VALID},{"dtn_es_vddi_1v8_pg_valid",IF_VALID},{"dtn_es_1v0_pg_valid",IF_VALID},
    {"vtt_vref_pg_valid",IF_VALID},{"tps_pgood_cpu_s1vdd_valid",IF_VALID},{"tps_pgood_cpu_s2vdd_valid",IF_VALID},{"tpsm_pgood_cpu_vcc1v35_valid",IF_VALID},
    {"tpsm_pgood_cpu_vcc1v8_valid",IF_VALID},{"prog_sfp_pgood_valid",IF_VALID},{"tpsm_pgood_cpu_vcc1v2_valid",IF_VALID},{"dtn_es_vddi_2v5_pg_valid",IF_VALID},
};
static const ipmc_field_t dsm_temp[] = {
    {"tmp451_sensor_for_cpu",IF_TEMP},{"tmp451_sensor_for_fpga",IF_TEMP},
};
static const ipmc_field_t dsm_voltage[] = {
    {"cpu_1v8",IF_RAW},{"cpu_1v35",IF_RAW},{"cpu_s1vdd",IF_RAW},{"cpu_s2vdd",IF_RAW},{"dtn_es_1v0",IF_RAW},
    {"dtn_es_vddi_2v5",IF_RAW},{"dtn_es_vdda_1v",IF_RAW},{"dtn_es_vcc3v3",IF_RAW},{"dtn_es_2v5",IF_RAW},
    {"dtn_es_vdd_1v8",IF_RAW},{"dtn_es_vddi_1v8",IF_RAW},{"cpu_1v2",IF_RAW},{"plus_1v8_1pps_vdd",IF_RAW},
    {"plus_1v8_1pps_vddo",IF_RAW},{"plus_1v8_1pps_fod",IF_RAW},{"plus_3v3_1pps_vdd1",IF_RAW},{"plus_3v3_1pps_vdd2",IF_RAW},
    {"plus_1v8_refclk_vdd",IF_RAW},{"plus_1v8_refclk_vddo",IF_RAW},{"plus_1v8_refclk_fod",IF_RAW},
    {"plus_3v3_refclk_vdd1",IF_RAW},{"plus_3v3_refclk_vdd2",IF_RAW},
};
static const ipmc_field_t dsm_current[] = {
    {"dtn_es_1v0_cs",IF_RAW},{"dtn_es_vddi_2v5_cs",IF_RAW},{"dtn_es_vdda_1v_cs",IF_RAW},{"dtn_es_vcc3v3_cs",IF_RAW},
    {"dtn_es_2v5_cs",IF_RAW},{"dtn_es_vdd_1v8_cs",IF_RAW},{"dtn_es_vddi_1v8_cs",IF_RAW},{"cpu_1v2_cs",IF_RAW},
    {"cpu_1v8_cs",IF_RAW},{"cpu_1v35_cs",IF_RAW},{"cpu_s1vdd_cs",IF_RAW},{"cpu_s2vdd_cs",IF_RAW},
    {"plus_1v8_1pps_vdd_cs",IF_RAW},{"plus_1v8_1pps_vddo_cs",IF_RAW},{"plus_1v8_1pps_fod_cs",IF_RAW},
    {"plus_3v3_1pps_vdd1_cs_cs",IF_RAW},{"plus_3v3_1pps_vdd2_cs",IF_RAW},{"plus_1v8_refclk_vdd_cs",IF_RAW},
    {"plus_1v8_refclk_vddo_cs",IF_RAW},{"plus_1v8_refclk_fod_cs",IF_RAW},{"plus_3v3_refclk_vdd1_cs",IF_RAW},{"plus_3v3_refclk_vdd2_cs",IF_RAW},
};
static const ipmc_field_t dsm_other[] = {
    {"cl_ref_lock_ind",IF_GOOD},{"cl_ref_hldovr_stat",IF_GOOD},{"cl_ref_los_ind",IF_GOOD},{"cl_1pps_lock_ind",IF_GOOD},
    {"cl_1pps_hldovr_stat",IF_GOOD},{"cl_1pps_los_ind",IF_GOOD},{"clk_los",IF_GOOD},{"clk_gen_lol",IF_GOOD},
    {"ipmbb_ready",IF_GOOD},{"ipmba_ready",IF_GOOD},{"mcu_asleep",IF_GOOD},{"ddr_test",IF_GOOD},
    {"serdes1_pll1_test",IF_GOOD},{"serdes1_pll2_test",IF_GOOD},{"ifc_nand_test",IF_GOOD},
};
static const ipmc_field_t dsm_other_valid[] = {
    {"cl_ref_lock_ind_valid",IF_VALID},{"cl_ref_hldovr_stat_valid",IF_VALID},{"cl_ref_los_ind_valid",IF_VALID},{"cl_1pps_lock_ind_valid",IF_VALID},
    {"cl_1pps_hldovr_stat_valid",IF_VALID},{"cl_1pps_los_ind_valid",IF_VALID},{"clk_los_valid",IF_VALID},{"clk_gen_lol_valid",IF_VALID},
    {"ipmbb_ready_valid",IF_VALID},{"ipmba_ready_valid",IF_VALID},{"mcu_asleep_valid",IF_VALID},
};
static const ipmc_section_t dsm_sections[] = {
    IPMC_SEC("Power Good", dsm_pgood), IPMC_SEC("Valid", dsm_valid),
    IPMC_SEC("Temperature (1/100 C)", dsm_temp), IPMC_SEC("Voltage (raw)", dsm_voltage),
    IPMC_SEC("Current (raw)", dsm_current), IPMC_SEC("Other", dsm_other),
    IPMC_SEC("Other Valid", dsm_other_valid),
};

static const ipmc_layout_t g_dpm_layout = { dpm_sections, (int)(sizeof(dpm_sections)/sizeof(dpm_sections[0])) };
static const ipmc_layout_t g_gpm_layout = { gpm_sections, (int)(sizeof(gpm_sections)/sizeof(gpm_sections[0])) };
static const ipmc_layout_t g_dsm_layout = { dsm_sections, (int)(sizeof(dsm_sections)/sizeof(dsm_sections[0])) };

static const ipmc_layout_t *ipmc_layout_for(uint32_t comp)
{
    switch (comp) {
        case IPMC_COMP_DPM: return &g_dpm_layout;
        case IPMC_COMP_DSM: return &g_dsm_layout;
        case IPMC_COMP_GPM: return &g_gpm_layout;
        default: return NULL;
    }
}

// --- store (device_id bazlı ham payload) ---
typedef struct {
    bool     valid;
    uint16_t device_id;
    uint32_t component_type;
    uint16_t len;
    uint8_t  raw[IPMC_RAW_MAX];
} ipmc_slot_t;

#define IPMC_MAX_BOARDS 16
static ipmc_slot_t g_ipmc[IPMC_MAX_BOARDS];
static int         g_ipmc_count = 0;

void ipmc_temp_store(const hm_ipmc_raw_t *src)
{
    if (src == NULL) return;
    ipmc_slot_t *e = NULL;
    for (int i = 0; i < g_ipmc_count; i++) {
        if (g_ipmc[i].device_id == src->device_id) { e = &g_ipmc[i]; break; }
    }
    if (e == NULL && g_ipmc_count < IPMC_MAX_BOARDS) e = &g_ipmc[g_ipmc_count++];
    if (e != NULL) {
        uint16_t n = src->len <= IPMC_RAW_MAX ? src->len : IPMC_RAW_MAX;
        e->valid          = true;
        e->device_id      = src->device_id;
        e->component_type = src->component_type;
        e->len            = n;
        memcpy(e->raw, src->raw, n);
    }
}

// Frame-içi tek satır (107'ye clamp) ve bölüm başlığı yardımcıları.
#define IPMC_LINE(...)  do { char _b[160]; snprintf(_b, sizeof _b, __VA_ARGS__); \
                             printf("║  %-107.107s ║\n", _b); } while (0)
#define IPMC_HEAD(s)    printf("║  " C_BOLD C_CYAN "%-107.107s" C_RESET " ║\n", (s))

static void ipmc_print_board(const ipmc_slot_t *s)
{
    const ipmc_layout_t *L = ipmc_layout_for(s->component_type);
    if (L == NULL) return;
    const uint8_t *raw = s->raw;
    uint16_t len = s->len;

    uint32_t lrm    = ip_be32(raw + IPMC_GEN_BASE + IPMC_GEN_LRM_DEV);
    uint32_t opmode = ip_be32(raw + IPMC_GEN_BASE + IPMC_GEN_OP_MODE);
    uint32_t power  = ip_be32(raw + IPMC_GEN_BASE + IPMC_GEN_POWER);
    uint32_t ipmb_a = ip_be32(raw + IPMC_GEN_BASE + IPMC_GEN_IPMB_A);
    uint32_t ipmb_b = ip_be32(raw + IPMC_GEN_BASE + IPMC_GEN_IPMB_B);
    uint16_t seq    = ip_be16(raw + IPMC_GEN_BASE + IPMC_GEN_SEQNUM);
    unsigned fw_maj = raw[IPMC_GEN_BASE + IPMC_GEN_FW_MAJOR];
    unsigned fw_min = raw[IPMC_GEN_BASE + IPMC_GEN_FW_MINOR];
    unsigned fw_pat = raw[IPMC_GEN_BASE + IPMC_GEN_FW_PATCH];
    unsigned rst    = raw[IPMC_GEN_BASE + IPMC_GEN_RESET_CNT];
    unsigned refr   = raw[IPMC_GEN_BASE + IPMC_GEN_REFRESH];
    unsigned board_no = (unsigned)((s->device_id % 256u) / 8u);

    IPMC_LINE("[ %s-%u  dev=0x%04x  comp=%u  LRM=%s ]  FW %u.%u.%u  op=%s  power=%s  ipmb A/B=%s/%s  seq=%u rst=%u refresh=%u",
              ipmc_lrm_name(lrm), board_no, s->device_id, s->component_type, ipmc_lrm_name(lrm),
              fw_maj, fw_min, fw_pat, ipmc_opmode_name(opmode), ipmc_power_name(power),
              ipmc_bus_name(ipmb_a), ipmc_bus_name(ipmb_b), seq, rst, refr);

    int tot_good = 0, bad_good = 0, tot_val = 0, bad_val = 0, bad_region = 0;
#if !DEBUG_MODE
    char tline[4][96]; int nt = 0;
#endif

    unsigned off = IPMC_OFF_BOARD;
    bool trunc = false;
    for (int si = 0; si < L->nsec && !trunc; si++) {
        const ipmc_section_t *sec = &L->sections[si];
#if DEBUG_MODE
        IPMC_HEAD(sec->title);
#endif
        for (int fi = 0; fi < sec->count; fi++) {
            const ipmc_field_t *f = &sec->fields[fi];
            if (f->kind == IF_GOOD || f->kind == IF_VALID) {
                if (off + IPMC_ENUM_STRIDE > len) { trunc = true; break; }
                uint32_t v = ip_be32(raw + off);
                off += IPMC_ENUM_STRIDE;
                if (f->kind == IF_GOOD) {
                    tot_good++; if (v != 1) bad_good++;
#if DEBUG_MODE
                    IPMC_LINE("    %-30s : %s", f->name, v == 1 ? "GOOD" : "NOT_GOOD");
#endif
                } else {
                    tot_val++; if (v != 0) bad_val++;
#if DEBUG_MODE
                    IPMC_LINE("    %-30s : %s", f->name, v == 0 ? "VALID" : "NOT_VALID");
#endif
                }
            } else { // IF_TEMP / IF_RAW — 10 bytes
                if (off + IPMC_VALUE_STRIDE > len) { trunc = true; break; }
                int16_t  sval = ip_be16s(raw + off + IPMC_VAL_OFF);
                uint32_t spec = ip_be32 (raw + off + IPMC_VAL_SPEC_OFF);
                uint32_t reg  = ip_be32 (raw + off + IPMC_VAL_REGION_OFF);
                off += IPMC_VALUE_STRIDE;
                if (spec && reg != 1) bad_region++;
                const char *lim = spec ? ipmc_region_name(reg) : "no limit set";
                if (f->kind == IF_TEMP) {
#if DEBUG_MODE
                    IPMC_LINE("    %-30s : %9s C   [%s]", f->name, fmt_temp((int32_t)sval), lim);
#else
                    if (nt < 4) { snprintf(tline[nt], sizeof tline[nt],
                                           "    %-30s : %9s C   [%s]", f->name, fmt_temp((int32_t)sval), lim); nt++; }
#endif
                } else {
#if DEBUG_MODE
                    uint16_t uval = (uint16_t)sval;
                    IPMC_LINE("    %-30s : %9u (0x%04x)   [%s]", f->name, uval, uval, lim);
#endif
                }
            }
        }
    }

#if !DEBUG_MODE
    for (int i = 0; i < nt; i++) IPMC_LINE("%s", tline[i]);
    IPMC_LINE("  Ozet: pgood %d/%d GOOD, valid %d/%d VALID, %d value region!=green%s",
              tot_good - bad_good, tot_good, tot_val - bad_val, tot_val, bad_region,
              trunc ? "  (UYARI: paket kisa, kesildi)" : "");
#else
    (void)tot_good; (void)bad_good; (void)tot_val; (void)bad_val; (void)bad_region;
    if (trunc) IPMC_LINE("  (UYARI: paket beklenenden kisa, kesildi)");
#endif
}

void print_ipmc_temperatures(void)
{
    // Görüntülenecek valid kayıtların index'leri; (component_type, device_id) artan.
    int idx[IPMC_MAX_BOARDS];
    int m = 0;
    for (int i = 0; i < g_ipmc_count; i++) if (g_ipmc[i].valid) idx[m++] = i;
    if (m == 0) return;
    for (int a = 1; a < m; a++) {
        int tmp = idx[a], b = a;
        while (b > 0 &&
               (g_ipmc[idx[b-1]].component_type > g_ipmc[tmp].component_type ||
                (g_ipmc[idx[b-1]].component_type == g_ipmc[tmp].component_type &&
                 g_ipmc[idx[b-1]].device_id > g_ipmc[tmp].device_id))) {
            idx[b] = idx[b-1]; b--;
        }
        idx[b] = tmp;
    }

#if DEBUG_MODE
    banner_plain("CL CMSW IPMC LOG  (VL50 - DPM/DSM/GPM tam board data; temp 1/100 C, limit=validity region)");
#else
    banner_plain("CL CMSW IPMC LOG  (VL50 - DPM/DSM/GPM; kimlik + sicaklik + ozet)");
#endif

    for (int k = 0; k < m; k++) {
        if (k > 0) hr();
        ipmc_print_board(&g_ipmc[idx[k]]);
    }

    table_footer();
}
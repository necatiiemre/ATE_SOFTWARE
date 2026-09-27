#ifndef CMC_OTHER_TYPES_H
#define CMC_OTHER_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SRC_MAX_CONN 20
#define DST_MAX_CONN 8
#define SRC_DPM_MAX_CONN 15

typedef struct __attribute__((packed))
{
	uint64_t send_count[SRC_MAX_CONN];
	uint64_t send_fail_count[SRC_MAX_CONN];
	uint64_t receive_count[SRC_MAX_CONN];
	uint64_t crc_pass_count[SRC_MAX_CONN];
	uint64_t crc_fail_count[SRC_MAX_CONN];
	uint64_t pkg_drop_count[SRC_MAX_CONN];
} COUNTERS_DPM;

typedef struct __attribute__((packed))
{
	uint64_t send_count[DST_MAX_CONN];
	uint64_t send_fail_count[DST_MAX_CONN];
	uint64_t receive_count[DST_MAX_CONN];
	uint64_t crc_pass_count[DST_MAX_CONN];
	uint64_t crc_fail_count[DST_MAX_CONN];
	uint64_t pkg_drop_count[DST_MAX_CONN];
} COUNTERS_DSM;

typedef struct __attribute__((packed))
{
	uint64_t send_count[SRC_DPM_MAX_CONN];
	uint64_t send_fail_count[SRC_DPM_MAX_CONN];
	uint64_t receive_count[SRC_DPM_MAX_CONN];
	uint64_t crc_pass_count[SRC_DPM_MAX_CONN];
	uint64_t crc_fail_count[SRC_DPM_MAX_CONN];
	uint64_t pkg_drop_count[SRC_DPM_MAX_CONN];
} COUNTERS_INTER_DPM;

/*inter lrm'in haberleşme countları — DPM ve DSM ayrı paketler olarak gelir*/

// ============================================================================
// DPM VL bazlı RX/TX akış sayaçları (per-VL flow counters)
// ----------------------------------------------------------------------------
// DPM-1..5 her biri 104 RX + 104 TX VL akışının paket sayacını tek pakette
// gönderir. Her sayaç 4 byte (uint32), wire'da big-endian. Layout:
//   [   0 .. 415 ] rx_count[104]  → RX VL-IDX base+i
//   [ 416 .. 831 ] tx_count[104]  → TX VL-IDX base+i
// Toplam 832 byte + 1 byte sequence trailer = 833 byte UDP payload.
// RX[i] ile TX[i] aynı mantıksal akışın giriş/çıkış VL-IDX'leridir; base
// VL-IDX'ler DPM'e göre değişir (bkz. health_monitor_cmc.c → dpm_vl_bases()).
#define DPM_VL_PORT_COUNT 104

typedef struct __attribute__((packed))
{
	uint32_t rx_count[DPM_VL_PORT_COUNT];   // 416 B — RX VL-IDX base+i
	uint32_t tx_count[DPM_VL_PORT_COUNT];   // 416 B — TX VL-IDX base+i
} COUNTERS_DPM_VL;                          // TOPLAM: 832 byte

// ============================================================================
// DPM RX/TX sayaç çiftleri (52 adet)
// ----------------------------------------------------------------------------
// DPM-1..5 (VL 2021/2042/2063/2084/2105) her biri tek pakette 52 adet
// {rx_count, tx_count} çifti gönderir. Her sayaç 4 byte (uint32), wire'da
// big-endian. Toplam 52 x 8 = 416 byte + 1 byte sequence trailer = 417 byte
// UDP payload. Paket SANİYELİK (delta) değer taşır; kümülatif toplam bizim
// tarafta biriktirilir (bkz. health_monitor_cmc.c → dpm52_accumulate()).
#define DPM_COUNTERS52_COUNT 52

typedef struct __attribute__((packed))
{
	uint32_t rx_count;
	uint32_t tx_count;
} Counters;

typedef struct __attribute__((packed))
{
	Counters msg[DPM_COUNTERS52_COUNT];     // 52 x 8 = 416 B
} COUNTERS_DPM_52;                          // TOPLAM: 416 byte

_Static_assert(sizeof(COUNTERS_DPM_52) == 416, "COUNTERS_DPM_52 size mismatch");


// NOT: Pcs_* tipleri wire formatında natural alignment kullanır (CMC firmware
// non-packed gönderiyor). Bu nedenle bu struct'lar packed DEĞİL —
// Pcs_monitor_type 16 byte (1+7+8).
//
// Pcs_mem_profile_type alanları firmware'de size_t'dir; firmware 32-bit hedefte
// derlendiği için size_t orada 4 byte'tır. Burada x86_64'te size_t 8 byte
// olacağından alanlar açıkça uint32_t tanımlanır. Böylece:
//   Pcs_profile_stats = 24 + 64 + 12 (heap) + 12 (stack) = 112 byte
//   wire paketi       = 112 + 1 byte sequence trailer   = 113 byte
typedef struct Pcs_monitor_type
{
    uint8_t   percentage;                     /*!< Percentage */
    uint64_t  usage;                          /*!< Usage amount (memory or time) */
} Pcs_monitor_type;

typedef struct Pcs_cpu_exec_time_type
{
    Pcs_monitor_type min_exec_time;             /*!< Minimum execution time in nanosec */
    Pcs_monitor_type max_exec_time;             /*!< Maximum execution time in nanosec */
    Pcs_monitor_type avg_exec_time;             /*!< Average execution time in nanosec */
    Pcs_monitor_type last_exec_time;            /*!< Latest execution time in nanosec */
} Pcs_cpu_exec_time_type;

typedef struct Pcs_mem_profile_type
{
    uint32_t total_size;          /*!< Total memory size in bytes (size_t, 32-bit firmware) */
    uint32_t used_size;           /*!< Used memory size in bytes (size_t, 32-bit firmware) */
    uint32_t max_used_size;       /*!< Maximum used memory size in bytes (size_t, 32-bit firmware) */
} Pcs_mem_profile_type;

typedef struct Pcs_profile_stats
{
    uint64_t              sample_count;       /*!< Number of samples (number of majors for ARINC, number of monitoring windows for POSIX) used for this profiling statistic. */
    uint64_t              latest_read_time;   /*!< Timestamp for the last profiling statistics calculation (in nanoseconds).  */
    uint64_t              total_run_time;     /*!< Execution time + Idle time for a major frame (in nanosec). Only valid for the last measurement (last major frame). */
    Pcs_cpu_exec_time_type  cpu_exec_time;      /*!< CPU execution time details in nanosec. */
    Pcs_mem_profile_type    heap_mem;           /*!< Heap memory information. */
    Pcs_mem_profile_type    stack_mem;          /*!< Stack memory information. */
} Pcs_profile_stats;

_Static_assert(sizeof(Pcs_profile_stats) == 112, "Pcs_profile_stats size mismatch (wire: 112 + 1 seq = 113)");

#endif /* CMC_OTHER_TYPES_H */
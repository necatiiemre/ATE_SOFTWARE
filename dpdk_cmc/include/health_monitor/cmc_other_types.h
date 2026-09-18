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
// DPM-1..5 her biri kendi 104 VL akışının paket sayacını tek pakette gönderir.
// Wire'da VL BAŞINA bir {rx, tx} çifti var — iki ayrı dizi değil, araya girmiş
// çiftler:
//   [  0 ..  7 ] VL index 0: rx_count, tx_count
//   [  8 .. 15 ] VL index 1: rx_count, tx_count
//   ...
//   [824 ..831 ] VL index 103
// Her sayaç 4 byte (uint32), big-endian. Toplam 832 byte + 1 byte sequence
// trailer = 833 byte UDP payload.
//
// NOT: bu diziliş, daha önce varsayılan [rx×104][tx×104] ayrık dizilişin
// yerini aldı. İki biçim de 832 byte olduğu için uzunluk kontrolü aradaki
// farkı yakalamaz — sayılar yanlış offsetten okunur ve sessizce saçmalar.
// Değerler beklenmedik görünüyorsa ilk şüphelenilecek yer burasıdır.
//
// rx_count = DPM'in o VL'de ALDIĞI  paket  → CMC RX (ATE → CMC yönü)
// tx_count = DPM'in o VL'de GÖNDERDİĞİ    → CMC TX (CMC → ATE yönü)
// index i'nin hangi VL-ID'ye denk geldiği DPM'in bloğuna bağlı; eşleme
// Config.h'deki CMC_DPM_BLOCKS_INIT tablosunda.
#define DPM_VL_PORT_COUNT 104

typedef struct __attribute__((packed))
{
	uint32_t rx_count;                      // CMC RX (DPM ingress)
	uint32_t tx_count;                      // CMC TX (DPM egress)
} VL_DATA_COUNT;                            // 8 byte

typedef struct __attribute__((packed))
{
	VL_DATA_COUNT vl[DPM_VL_PORT_COUNT];    // 104 × 8 B
} COUNTERS_DPM_VL;                          // TOPLAM: 832 byte


// NOT: Pcs_* tipleri wire formatında natural alignment kullanır (CMC firmware
// non-packed gönderiyor; toplam 136 byte). Bu nedenle bu struct'lar packed
// DEĞİL — Pcs_monitor_type 16 byte (1+7+8), Pcs_profile_stats 136 byte.
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
    size_t total_size;            /*!< Total memory size in bytes */
    size_t used_size;             /*!< Used memory size in bytes */
    size_t max_used_size;         /*!< Maximum used memory size in bytes */
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

#endif /* CMC_OTHER_TYPES_H */
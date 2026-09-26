/*
 * clcmsw_ipmc_types.h
 *
 * CL CMSW IPMC log payload types (VL 50) — TAM board data formatı.
 * Üç paket tipi gelir (hepsi packed, big-endian), DSM-A'dan yayınlanır:
 *   clcmsw_ipmc_dpm_data (556 B) : component_type = 56 (LOG_IPMC_DPM)
 *   clcmsw_ipmc_dsm_data (846 B) : component_type = 57 (LOG_IPMC_DSM)
 *   clcmsw_ipmc_gpm_data (586 B) : component_type = 59 (LOG_IPMC_GPM)
 *
 * Wire layout (paket başından):
 *   log_header       @0   (22 B) : device_id(u16)@0, component_type(u32)@2,
 *                                   message_type(u32)@6, timestamp(u64)@8,
 *                                   log_payload_len(u32)@16
 *   generic_ipmc     @22  (36 B) : NON-PACKED → doğal padding'li (aşağıdaki
 *                                   offset'ler wire-doğrulandı)
 *   <board>_data     @58  (...)  : pgood/valid (4B enum) + temp/voltage/current
 *                                   (10B value) + other/other_valid akışı
 *
 * KRİTİK: pcs_bool = 4 byte → Pcs_ipmc_limit_status = 8 B,
 *         Pcs_ipmc_(un)signed_value_type = 10 B (value 2 + is_specified 4 +
 *         validity_region 4). enum'lar 4 B. Tüm çok-baytlı alanlar big-endian.
 *
 * Alanların isim/sıra bilgisi descriptor tablolarında (health_monitor_cmc.c).
 * Bu header parse için gereken sabit offset ve boyutları tanımlar.
 */

#ifndef CLCMSW_IPMC_TYPES_H
#define CLCMSW_IPMC_TYPES_H

#include <stdint.h>

// component_type (log_header, offset 2) — paket tipini ayırt eder.
#define IPMC_COMP_DPM 56
#define IPMC_COMP_DSM 57
#define IPMC_COMP_GPM 59

// LRM_dev enum (generic_ipmc içinde) — teyit amaçlı.
#define IPMC_LRM_DSM 1
#define IPMC_LRM_DPM 2
#define IPMC_LRM_GPM 6

// --- log_header offset'leri (packed, 22 B) ---
#define IPMC_OFF_DEVICE_ID  0    // u16
#define IPMC_OFF_COMPONENT  2    // u32
#define IPMC_OFF_MSGTYPE    6    // u32
#define IPMC_OFF_TIMESTAMP  8    // u64
#define IPMC_OFF_PAYLEN     16   // u32
#define IPMC_HDR_LEN        22

// --- generic_ipmc offset'leri (generic base = 22'den itibaren, NON-packed) ---
#define IPMC_GEN_BASE       22
#define IPMC_GEN_LRM_DEV    0    // u32 enum
#define IPMC_GEN_IS_AUTH    4    // u8
#define IPMC_GEN_IPMB_A     8    // u32 enum (0=HEALTHY 1=CONN_TIMEOUT)
#define IPMC_GEN_IPMB_B     12   // u32 enum
#define IPMC_GEN_FW_MAJOR   16   // u8
#define IPMC_GEN_FW_MINOR   17   // u8
#define IPMC_GEN_FW_PATCH   18   // u8
#define IPMC_GEN_OP_MODE    20   // u32 enum (0=OFP 1=MAINTENANCE)
#define IPMC_GEN_SEQNUM     24   // u16
#define IPMC_GEN_POWER      28   // u32 enum (1=ON 2=OFF 3=WAIT 4=PARTIAL)
#define IPMC_GEN_RESET_CNT  32   // u8
#define IPMC_GEN_REFRESH    33   // u8
#define IPMC_GEN_LEN        36   // (padding dahil)

// --- board data başlangıcı ve value/enum stride'ları ---
#define IPMC_OFF_BOARD      58   // = IPMC_HDR_LEN + IPMC_GEN_LEN
#define IPMC_ENUM_STRIDE    4    // pgood/valid/other/other_valid (4B enum)
#define IPMC_VALUE_STRIDE   10   // temp/voltage/current (value 2 + spec 4 + region 4)
#define IPMC_VAL_OFF        0    // int16 value
#define IPMC_VAL_SPEC_OFF   2    // u32 is_specified
#define IPMC_VAL_REGION_OFF 6    // u32 validity_region

// --- tam paket boyutları (probe + wire doğrulandı) ---
#define IPMC_LEN_DPM 556
#define IPMC_LEN_GPM 586
#define IPMC_LEN_DSM 846

// Ring/store için ham payload üst sınırı (DSM wire'da +pad ile ~856).
#define IPMC_RAW_MAX 900

#endif /* CLCMSW_IPMC_TYPES_H */
/**
 * 
 * BIT ORDER ADJUSTMENT
 * The bitfield order in these structures has been manually reversed to ensure 
 * compatibility between Big-Endian (PowerPC) data transmission and 
 * Little-Endian (Intel/ARM) memory architecture.
 * 
 * - For multi-byte fields (> 8-bit): Use ntohs(), ntohl() or ntohll() to correct byte order.
 * - For bitfields (< 8-bit): The order of fields within the struct has been 
 *   manually re-arranged to match the Little-Endian bit-ordering convention.
 */

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    #define SYSTEM_LITTLE_ENDIAN 1
#elif defined(_WIN32) || defined(_WIN64)
    #define SYSTEM_LITTLE_ENDIAN 1
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    #define SYSTEM_BIG_ENDIAN 1
#else
    #error "UNKNOWN ARCHITECTURE!"
#endif


/* Auto-generated C structures from CSV */
#ifndef MMPLCMSW_TAI_EXT_INTERFACE_H
#define MMPLCMSW_TAI_EXT_INTERFACE_H
#include <stdint.h>

#define NUMBER_OF_COMPUTER 2U
#define NUMBER_OF_AVDCU 2U
#define NUMBER_OF_DTN_IRS 2U
#define NUMBER_OF_DPM 5U
#define NUMBER_OF_PSM 2U
#define NUMBER_OF_GPM 2U
#define NUMBER_OF_HSM 2U
#define NUMBER_OF_DSM 2U
#define NUMBER_OF_HUM 2U
#define NUMBER_OF_SMMM 1U
#define NUMBER_OF_IOCM 1U

/** Status of Master MMP Level CMSW */
typedef struct
{
    /** LRU ID of the CMC running as MASTER. */
    uint8_t master_cmc_id : 8;
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t reserved_2                  : 3;
    uint8_t master_backplane_slot_index : 5;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Backplane slot index of the DSM running as MASTER. */
    uint8_t master_backplane_slot_index : 5;
    uint8_t reserved_2                  : 3;
    #endif
    /** Heartbeat status of Master MMP Level CMSW running as MASTER. 0: GOOD 1: BAD */
    uint8_t master_hb_status : 8;
} __attribute__((packed)) Master_status_t;

/** Status of Backup MMP Level CMSW */
typedef struct
{
    /** LRU ID of the CMC running as BACKUP. */
    uint8_t backup_lru_id : 8;
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t reserved_3                  : 3;
    uint8_t backup_backplane_slot_index : 5;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Backplane slot index of the DSM running as BACKUP. */
    uint8_t backup_backplane_slot_index : 5;
    uint8_t reserved_3                  : 3;
    #endif
    /** Heartbeat status of Backup MMP Level CMSW running as BACKUP. 0: GOOD 1: BAD */
    uint8_t backup_hb_status : 8;
} __attribute__((packed)) Backup_status_t;

/** Unique slot identifier within the computer. */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t reserved_7               : 3;
    uint8_t lrm_backplane_slot_index : 5;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Slot index of the Backplane where the LRM is installed on. */
    uint8_t lrm_backplane_slot_index : 5;
    uint8_t reserved_7               : 3;
    #endif
} __attribute__((packed)) Slot_identifier_t;

/** Generic IPMC status of this module. */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint16_t ipmc_fw_version_patch : 6;
    uint16_t ipmc_fw_version_minor : 4;
    uint16_t ipmc_fw_version_major : 2;
    uint16_t reserved_59 : 4;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint16_t reserved_59 : 4;
    /** Indicates major change in firmware the version. Major change is a change impacting the function, performance, and interchangeability characteristics of the item. */
    uint16_t ipmc_fw_version_major : 2;
    /** Indicates minor change in firmware the version. Minor change is a change that is a modification that does not significantly impact the function, performance or interchangeability characteristics of the item, often involving small adjustments or corrections. */
    uint16_t ipmc_fw_version_minor : 4;
    /** Indicates patch change in firmware the version. It is incremented for each patch in the component. */
    uint16_t ipmc_fw_version_patch : 6;
    #endif
    /** Indivates the serial ID of the CBA. */
    uint64_t lrm_serial_id : 64;
    /** Indicates the PCB Revision Number of CBA. */
    uint32_t lrm_pcb_revision_number : 32;
    /** Indicates the PCB Assembly ID of CBA. */
    uint32_t lrm_pcb_assembly_id : 32;
    /** Indicates the Mechanical Revision Number of CBA. */
    uint32_t lrm_mechanical_revision_number : 32;
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t operating_mode : 1;
    uint8_t reserved_x : 7;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Operating mode of the IPMC firmware. 0: OFP_IPMC 1: MAINTENANCE_IPMC */
    uint8_t reserved_x : 7;
    uint8_t operating_mode : 1;
    #endif
} __attribute__((packed)) Generic_ipmc_status_t;

/** DTN End System custom design version */
typedef struct
{
    /** Indicates DTN ES Custom Design Version Major part. Major change is one that affects the requirements, external interfaces, cost, and/or schedule. */
    uint8_t dtn_es_custom_design_version_major : 8;
    /** Indicates DTN ES Custom Design Version Minor part. Minor changes are those that do not affect form, fit, function, cost, or schedule. */
    uint8_t dtn_es_custom_design_version_minor : 8;
    /** Indicates DTN ES Custom Design Version Bugfix part. */
    uint8_t dtn_es_custom_design_version_bugfix : 8;
} __attribute__((packed)) Dtn_es_custom_design_version_t;

/** Indicates the result of T2080_DPM_BOOTLOADER_PBIT. */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t reserved_73       : 3;
    uint8_t serdes2_pll2_test : 1;
    uint8_t serdes2_pll1_test : 1;
    uint8_t serdes1_pll1_test : 1;
    uint8_t ifc_nand_test : 1;
    uint8_t ddr_test : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the result of DDR test. 0: GOOD 1: BAD */
    uint8_t ddr_test : 1;
    /** Indicates the result of IFC NAND test. 0: GOOD 1: BAD */
    uint8_t ifc_nand_test : 1;
    /** Indicates the result of SERDES1 PLL1 test. 0: GOOD 1: BAD */
    uint8_t serdes1_pll1_test : 1;
    /** Indicates the result of SERDES2 PLL1 test. 0: GOOD 1: BAD */
    uint8_t serdes2_pll1_test : 1;
    /** Indicates the result of SERDES2 PLL2 test. 0: GOOD 1: BAD */
    uint8_t serdes2_pll2_test : 1;
    uint8_t reserved_73       : 3;
    #endif
} __attribute__((packed)) Mmplcmsw_t2080_dpm_bootloader_pbit_t;

/** Indicates the result of T2080_DSM_BOOTLOADER_PBIT. */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t reserved_74       : 3;
    uint8_t serdes2_pll1_test : 1;
    uint8_t serdes1_pll2_test : 1;
    uint8_t serdes1_pll1_test : 1;
    uint8_t ifc_nand_test : 1;
    uint8_t ddr_test : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the result of DDR test. 0: GOOD 1: BAD */
    uint8_t ddr_test : 1;
    /** Indicates the result of IFC NAND test. 0: GOOD 1: BAD */
    uint8_t ifc_nand_test : 1;
    /** Indicates the result of SERDES1 PLL1 test. 0: GOOD 1: BAD */
    uint8_t serdes1_pll1_test : 1;
    /** Indicates the result of SERDES1 PLL2 test. 0: GOOD 1: BAD */
    uint8_t serdes1_pll2_test : 1;
    /** Developer Note: NOT APPLICABLE for Block-0. Indicates the result of SERDES2 PLL1 test. 0: GOOD 1: BAD */
    uint8_t serdes2_pll1_test : 1;
    uint8_t reserved_74       : 3;
    #endif
} __attribute__((packed)) Mmplcmsw_t2080_dsm_bootloader_pbit_t;

/** Indicates the status of an DPM defined in the MMP Level CMSW configuration. */
typedef struct
{
    /** Unique slot identifier within the computer */
    Slot_identifier_t slot_idx;
    /** Module Level CMSW deployed on the DSM sends status report messages periodically to Computer Level CMSW deployed on DSM. This field tests whether DPM is alive or not based on messages received by Computer Level CMSW and IPMC status of the DPM. MODULE_ALIVE: CMSW heartbeat received within timeout time defined in the MMP Level CMSW configuration data and module IPMC status is GOOD. MODULE_LOSS: CMSW heartbeat not received within timeout time defined in the MMP Level CMSW configuration data or module IPMC status is BAD. */
    uint8_t module_status : 8;
    /** Generic IPMC status of this module */
    Generic_ipmc_status_t generic_ipmc_status;
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t ptp_sync_status : 1;
    uint8_t dtn_es_redundancy_status : 1;
    uint8_t dtn_es_config_status : 1;
    uint8_t dtn_es_data_validity : 1;
    uint8_t voltage_data_status : 1;
    uint8_t temperature_data_status : 1;
    uint8_t major_frame_alignment_status : 1;
    uint8_t schedule_config_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** CONFIG_MATCH: Schedule ID reported by this module matches with Schedule ID in the MMP Level CMSW configuration data for current platform configuration. CONFIG_MISMATCH: Schedule ID reported by this module does not match with Schedule ID for current platform configuration in the configuration data. 0: CONFIG_MATCH 1: CONFIG_MISMATCH */
    uint8_t schedule_config_status : 1;
    /** NOT_ALIGNED: Module has not been aligned. ALIGNED: Module has been aligned 0: ALIGNED 1: NOT_ALIGNED */
    uint8_t major_frame_alignment_status : 1;
    /** Indicates the availability of temperature data. 0: TEMP_DATA_AVAILABLE 1: TEMP_DATA_LOSS */
    uint8_t temperature_data_status : 1;
    /** Indicates the availability of voltage data. 0: VOLTAGE_DATA_AVAILABLE 1: VOLTAGE_DATA_LOSS */
    uint8_t voltage_data_status : 1;
    /** Whether following DTN ES data is valid or not. 0: VALID 1: INVALID */
    uint8_t dtn_es_data_validity : 1;
    /** CONFIG_MATCH: DTN End System Configuration ID reported by this module matches with DTN End System Configuration ID for current platform configuration in the configuration data. CONFIG_MISMATCH: DTN End System Configuration ID reported by this module does not match with DTN End System Configuration ID for current platform configuration in the configuration data. 0: CONFIG_MATCH 1: CONFIG_MISMATCH */
    uint8_t dtn_es_config_status : 1;
    /** REDUNDANCY_OK: DTN A & B link status is good REDUNDANCY_LOSS: Loss of DTN A or B link 0: REDUNDANCY_OK 1: REDUNDANCY_LOSS */
    uint8_t dtn_es_redundancy_status : 1;
    /** SYNCHED: If the module receives both PTP synch messages and PTP response messages according to the PTP configuration. NOT_SYNCHED: If the module does not recieve PTP synch messages and/or PTP response messages according to the PTP configuration. 0: SYNCHED 1: NOT_SYNCHED */
    uint8_t ptp_sync_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t reserved_8        : 7;
    uint8_t ptp_config_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** CONFIG_MATCH: PTP Configuration ID reported by this module matches with PTP Configuration ID for this module in current platform configuration in the MMP Level CMSW Configuration Data. CONFIG_MISMATCH: PTP Configuration ID reported by this module does not match with DTN ES Configuration ID for this module in current platform configuration in the MMP Level CMSW Configuration Data. 0: CONFIG_MATCH 1: CONFIG_MISMATCH */
    uint8_t ptp_config_status : 1;
    uint8_t reserved_8        : 7;
    #endif
    /** DTN End System custom design version */
    Dtn_es_custom_design_version_t dtn_es_custom_design_version;
    /** Indicates the result of T2080_dpm_BOOTLOADER_PBIT. */
    Mmplcmsw_t2080_dpm_bootloader_pbit_t t2080_dpm_bootloader_pbit;
} __attribute__((packed)) Dpm_status_list_t;

/** Incoming PSM status message that gathered from Computer Level CMSW. */
typedef struct
{
    /** Unique slot identifier within the computer */
    Slot_identifier_t slot_idx;
    /** Indicates the availability of the MCU on the LRM to chassis manager. 0: MODULE_ALIVE 1: MODULE_LOSS */
    uint8_t module_status : 8;
    /** Generic IPMC status of this module */
    Generic_ipmc_status_t generic_ipmc_status;
} __attribute__((packed)) Psm_status_list_t;

/** Indicates the result of SMMM Interface CBIT. */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t key_cbit_result : 1;
    uint8_t trng_cbit_result : 1;
    uint8_t reserved_14 : 6;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint8_t reserved_14 : 6;
    /** Indicates the result of TRNG CBIT. 0: GOOD 1: BAD */
    uint8_t trng_cbit_result : 1;
    /** Indicates the result of KEY CBIT 0: GOOD 1: BAD */
    uint8_t key_cbit_result : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t pcie_backplane_cbit_result : 1;
    uint8_t pcie_mass_storage_cbit_result : 1;
    uint8_t dtn_0_cbit_result : 1;
    uint8_t dtn_1_cbit_result : 1;
    uint8_t uart_sm_mngr_cbit_result : 1;
    uint8_t uart_sm_key_storage_cbit_result : 1;
    uint8_t fo_intf_cbit_result : 1;
    uint8_t algorithm_cbit_result : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the result of ALGORITHM CBIT. 0: GOOD 1: BAD */
    uint8_t algorithm_cbit_result : 1;
    /** Indicates the result of FO_INTF_CBIT 0: GOOD 1: BAD */
    uint8_t fo_intf_cbit_result : 1;
    /** Indicates the result of UART_SM Key Storage MCU Interface Test 0: GOOD 1: BAD */
    uint8_t uart_sm_key_storage_cbit_result : 1;
    /** Indicates the result of UART_SM Manager MCU Interface Test 0: GOOD 1: BAD */
    uint8_t uart_sm_mngr_cbit_result : 1;
    /** Indicates the result of DTN_1_CBIT 0: GOOD 1: BAD */
    uint8_t dtn_1_cbit_result : 1;
    /** Indicates the result of DTN_0_CBIT 0: GOOD 1: BAD */
    uint8_t dtn_0_cbit_result : 1;
    /** Indicates the result of PCIe for Mass Storage Interface Test. 0: GOOD 1: BAD */
    uint8_t pcie_mass_storage_cbit_result : 1;
    /** Indicates the result of PCIe for Backplane Interface Test. 0: GOOD 1: BAD */
    uint8_t pcie_backplane_cbit_result : 1;
    #endif
} __attribute__((packed)) Interface_cbit_result_t;

/** Indicates the Alarm Status of SMMM */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t crypto_algorithm_fail_alarm_status : 1;
    uint8_t reserved_15 : 7;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint8_t reserved_15 : 7;
    /** Indicates the status of Crypto algorithm failure alarm. */
    uint8_t crypto_algorithm_fail_alarm_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t emergency_zeroization_alarm_status : 1;
    uint8_t internal_tamper_detection_alarm_status : 1;
    uint8_t external_tamper_detection_alarm_status : 1;
    uint8_t battery_voltage_low_alarm : 1;
    uint8_t module_high_temp_alarm_status : 1;
    uint8_t power_supply_low_voltage_alarm_status : 1;
    uint8_t power_supply_high_voltage_alarm_status : 1;
    uint8_t crypto_key_integrity_fail_alarm_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of Crypto key integrity failure alarm. */
    uint8_t crypto_key_integrity_fail_alarm_status : 1;
    /** Indicates the status of power supply high voltage alarm. */
    uint8_t power_supply_high_voltage_alarm_status : 1;
    /** Indicates the status of power supply low voltage alarm */
    uint8_t power_supply_low_voltage_alarm_status : 1;
    /** Indicates the status of module high temperature alarm. */
    uint8_t module_high_temp_alarm_status : 1;
    /** Indicates the status of Battery voltage low alarm. */
    uint8_t battery_voltage_low_alarm : 1;
    /** Indicates the status of External tamper detection alarm. */
    uint8_t external_tamper_detection_alarm_status : 1;
    /** Indicates the status of Internal tamper detection alarm. */
    uint8_t internal_tamper_detection_alarm_status : 1;
    /** Indicates the status of Emergency zeroization alarm */
    uint8_t emergency_zeroization_alarm_status : 1;
    #endif
} __attribute__((packed)) Alarm_status_t;

/** Indicates the status of an SMMM defined in the MMP Level CMSW configuration. */
typedef struct
{
    /** Unique slot identifier within the computer */
    Slot_identifier_t slot_idx;
    /** SMMM sends status messages periodically. This field tests whether SMMM is alive or not based on received status messages and SMMM IPMC status. MODULE_ALIVE: SMMM status message received within timeout time defined in the MMP Level CMSW configuration data and module IPMC status is GOOD. MODULE_LOSS: SMMM status message not received within timeout time defined in the MMP Level CMSW configuration data or module IPMC status is BAD. 0: MODULE_ALIVE 1: MODULE_LOSS */
    uint8_t module_status : 8;
    /** Generic IPMC status of this module */
    Generic_ipmc_status_t generic_ipmc_status;
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t reserved_13         : 3;
    uint8_t voltage_data_status : 1;
    uint8_t reserved_75         : 4;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint8_t reserved_75         : 4;
    /** Indicates the availability of voltage data. 0: VOLTAGE_DATA_AVAILABLE 1: VOLTAGE_DATA_LOSS */
    uint8_t voltage_data_status : 1;
    uint8_t reserved_13         : 3;
    #endif
    /** Indicates the result of SMMM Interface CBIT */
    Interface_cbit_result_t interface_cbit_result;
    /** Indicates the Alarm Status of SMMM */
    Alarm_status_t alarm_status;
    /** Custom design version of SMMM FPGA. */
    uint16_t fpga_code_version : 16;
    /** Security Controller MCU Firmware version is sent as an unsigned integer value. */
    uint16_t security_cntrlr_mcu_fw_version : 16;
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint16_t smmm_fo_status : 1;
    uint16_t reserved_16    : 15;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint16_t reserved_16    : 15;
    /** Indicates the status of SMMM Fiber Optic. 0: SMMM_FO_OK 1: SMMM_FO_LOSS */
    uint16_t smmm_fo_status : 1;
    #endif

    uint16_t reserved_smmm_17;
} __attribute__((packed)) Smmm_status_t;

/** Indicates the status of an GPM defined in the MMP Level CMSW configuration. */
typedef struct
{
    /** Unique slot identifier within the computer */
    Slot_identifier_t slot_idx;
    /** Indicates the availability of the MCU on the LRM to chassis manager. 0: MODULE_ALIVE 1: MODULE_LOSS */
    uint8_t module_status : 8;
    /** Generic IPMC status of this module */
    Generic_ipmc_status_t generic_ipmc_status;
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t reserved_82         : 7;
    uint8_t advb_hm_data_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of ADVB HM Data. 0: ADVB_HM_DATA_AVAILABLE 1: ADVB_HM_DATA_LOSS */
    uint8_t advb_hm_data_status : 1;
    uint8_t reserved_82         : 7;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t advb_tx_1_status : 2;
    uint8_t advb_tx_2_status : 2;
    uint8_t advb_tx_3_status : 2;
    uint8_t reserved_70 : 2;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint8_t reserved_70 : 2;
    /** Indicates the availability of ADVB_TX_3 0: ADVB_TX_OK 1: ADVB_TX_LOSS 2: ADVB_TX_WARNING */
    uint8_t advb_tx_3_status : 2;
    /** Indicates the availability of ADVB_TX_2 0: ADVB_TX_OK 1: ADVB_TX_LOSS 2: ADVB_TX_WARNING */
    uint8_t advb_tx_2_status : 2;
    /** Indicates the availability of ADVB_TX_1 0: ADVB_TX_OK 1: ADVB_TX_LOSS 2: ADVB_TX_WARNING */
    uint8_t advb_tx_1_status : 2;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t reserved_69         : 2;
    uint8_t reserved_x          : 1;
    uint8_t voltage_data_status : 1;
    uint8_t temperature_data_status : 1;
    uint8_t dvi_status : 2;
    uint8_t advb_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of ADVB. 0: ADVB_OK 1: ADVB_LOSS */
    uint8_t advb_status : 1;
    /** Indicates the availability of DVI 0: DVI_AVAILABLE 1: DVI_LOSS 2: DVI_WARNING */
    uint8_t dvi_status : 2;
    /** Indicates the availability of temperature data. 0: TEMP_DATA_AVAILABLE 1: TEMP_DATA_LOSS */
    uint8_t temperature_data_status : 1;
    /** Indicates the availability of voltage data. 0: VOLTAGE_DATA_AVAILABLE 1: VOLTAGE_DATA_LOSS */
    uint8_t voltage_data_status : 1;
    uint8_t reserved_x          : 1;
    uint8_t reserved_69         : 2;
    #endif
} __attribute__((packed)) Gpm_status_list_t;

/** Indicates the loss of HSN Link. */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t hsn_link_pl_01_status : 1;
    uint8_t hsn_link_pl_02_status : 1;
    uint8_t hsn_link_pl_03_status : 1;
    uint8_t hsn_link_pl_04_status : 1;
    uint8_t hsn_link_pl_05_status : 1;
    uint8_t hsn_link_pl_06_status : 1;
    uint8_t hsn_link_pl_07_status : 1;
    uint8_t hsn_link_pl_08_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the loss of HSN Link PL 8. 0: HSN_LINK_OK 1: HSN_LINK_LOSS */
    uint8_t hsn_link_pl_08_status : 1;
    /** Indicates the loss of HSN Link PL 7. 0: HSN_LINK_OK 1: HSN_LINK_LOSS */
    uint8_t hsn_link_pl_07_status : 1;
    /** Indicates the loss of HSN Link PL 6. 0: HSN_LINK_OK 1: HSN_LINK_LOSS */
    uint8_t hsn_link_pl_06_status : 1;
    /** Indicates the loss of HSN Link PL 5. 0: HSN_LINK_OK 1: HSN_LINK_LOSS */
    uint8_t hsn_link_pl_05_status : 1;
    /** Indicates the loss of HSN Link PL 4. 0: HSN_LINK_OK 1: HSN_LINK_LOSS */
    uint8_t hsn_link_pl_04_status : 1;
    /** Indicates the loss of HSN Link PL 3. 0: HSN_LINK_OK 1: HSN_LINK_LOSS */
    uint8_t hsn_link_pl_03_status : 1;
    /** Indicates the loss of HSN Link PL 2. 0: HSN_LINK_OK 1: HSN_LINK_LOSS */
    uint8_t hsn_link_pl_02_status : 1;
    /** Indicates the loss of HSN Link PL 1. 0: HSN_LINK_OK 1: HSN_LINK_LOSS */
    uint8_t hsn_link_pl_01_status : 1;
    #endif
} __attribute__((packed)) Hsn_link_pl_01_08_status_t;

/** Indicates the status of an HSM defined in the MMP Level CMSW configuration. */
typedef struct
{
    /** Unique slot identifier within the computer */
    Slot_identifier_t slot_idx;
    /** Availability status of the MCU on the LRM to chassis manager. 0: MODULE_ALIVE 1: MODULE_LOSS */
    uint8_t module_status : 8;
    /** Generic IPMC status of this module */
    Generic_ipmc_status_t generic_ipmc_status;
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t hsm_data_validity : 4;
    uint8_t hsm_current_status: 2;
    uint8_t hsm_voltage_status: 2;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of HSM Voltage Data. 0: VOLTAGE_GOOD 1: VOLTAGE_DATA_LOSS 2: VOLTAGE_ORANGE 3: VOLTAGE_RED*/
    uint8_t hsm_voltage_status: 2;
    /** Indicates the status of HSM Current Data. 0: CURRENT_GOOD 1: CURRENT_DATA_LOSS 2: CURRENT_ORANGE 3: CURRENT_RED*/
    uint8_t hsm_current_status: 2;
    /** Whether following HSM status are valid or not. 0: VALID 1: INVALID */
    uint8_t hsm_data_validity : 4;
    #endif
    /** Indicates the loss of HSN Link */
    Hsn_link_pl_01_08_status_t hsn_link_pl_01_08_status;
    /** Indicates the status of HSM Temperature Data. 0: TEMP_GOOD 1: TEMP_DATA_LOSS 2: TEMP_ORANGE 3: TEMP_RED*/
    uint8_t hsm_temp_status : 8;
} __attribute__((packed)) Hsm_status_list_t;

/** Indicates whether the LRM in the Slot is the same defined in the configuration data. */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint16_t backplane_slot_16_config_status : 1;
    uint16_t backplane_slot_17_config_status : 1;
    uint16_t reserved_25 : 14;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint16_t reserved_25 : 14;
    /** Indicates the status of BACKPLANE_SLOT_17_CONFIG 0: CONFIGURATION_OK 1: CONFIGURATION_MISMATCH */
    uint16_t backplane_slot_17_config_status : 1;
    /** Indicates the status of BACKPLANE_SLOT_16_CONFIG 0: CONFIGURATION_OK 1: CONFIGURATION_MISMATCH */
    uint16_t backplane_slot_16_config_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t backplane_slot_8_config_status : 1;
    uint8_t backplane_slot_9_config_status : 1;
    uint8_t backplane_slot_10_config_status : 1;
    uint8_t backplane_slot_11_config_status : 1;
    uint8_t backplane_slot_12_config_status : 1;
    uint8_t backplane_slot_13_config_status : 1;
    uint8_t backplane_slot_14_config_status : 1;
    uint8_t backplane_slot_15_config_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of BACKPLANE_SLOT_15_CONFIG 0: CONFIGURATION_OK 1: CONFIGURATION_MISMATCH */
    uint8_t backplane_slot_15_config_status : 1;
    /** Indicates the status of BACKPLANE_SLOT_14_CONFIG 0: CONFIGURATION_OK 1: CONFIGURATION_MISMATCH */
    uint8_t backplane_slot_14_config_status : 1;
    /** Indicates the status of BACKPLANE_SLOT_13_CONFIG 0: CONFIGURATION_OK 1: CONFIGURATION_MISMATCH */
    uint8_t backplane_slot_13_config_status : 1;
    /** Indicates the status of BACKPLANE_SLOT_12_CONFIG 0: CONFIGURATION_OK 1: CONFIGURATION_MISMATCH */
    uint8_t backplane_slot_12_config_status : 1;
    /** Indicates the status of BACKPLANE_SLOT_11_CONFIG 0: CONFIGURATION_OK 1: CONFIGURATION_MISMATCH */
    uint8_t backplane_slot_11_config_status : 1;
    /** Indicates the status of BACKPLANE_SLOT_10_CONFIG 0: CONFIGURATION_OK 1: CONFIGURATION_MISMATCH */
    uint8_t backplane_slot_10_config_status : 1;
    /** Indicates the status of BACKPLANE_SLOT_9_CONFIG 0: CONFIGURATION_OK 1: CONFIGURATION_MISMATCH */
    uint8_t backplane_slot_9_config_status : 1;
    /** Indicates the status of BACKPLANE_SLOT_8_CONFIG 0: CONFIGURATION_OK 1: CONFIGURATION_MISMATCH */
    uint8_t backplane_slot_8_config_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t reserved_24                    : 1;
    uint8_t backplane_slot_1_config_status : 1;
    uint8_t backplane_slot_2_config_status : 1;
    uint8_t backplane_slot_3_config_status : 1;
    uint8_t backplane_slot_4_config_status : 1;
    uint8_t backplane_slot_5_config_status : 1;
    uint8_t backplane_slot_6_config_status : 1;
    uint8_t backplane_slot_7_config_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of BACKPLANE_SLOT_7_CONFIG 0: CONFIGURATION_OK 1: CONFIGURATION_MISMATCH */
    uint8_t backplane_slot_7_config_status : 1;
    /** Indicates the status of BACKPLANE_SLOT_6_CONFIG 0: CONFIGURATION_OK 1: CONFIGURATION_MISMATCH */
    uint8_t backplane_slot_6_config_status : 1;
    /** Indicates the status of BACKPLANE_SLOT_5_CONFIG 0: CONFIGURATION_OK 1: CONFIGURATION_MISMATCH */
    uint8_t backplane_slot_5_config_status : 1;
    /** Indicates the status of BACKPLANE_SLOT_4_CONFIG 0: CONFIGURATION_OK 1: CONFIGURATION_MISMATCH */
    uint8_t backplane_slot_4_config_status : 1;
    /** Indicates the status of BACKPLANE_SLOT_3_CONFIG 0: CONFIGURATION_OK 1: CONFIGURATION_MISMATCH */
    uint8_t backplane_slot_3_config_status : 1;
    /** Indicates the status of BACKPLANE_SLOT_2_CONFIG 0: CONFIGURATION_OK 1: CONFIGURATION_MISMATCH */
    uint8_t backplane_slot_2_config_status : 1;
    /** Indicates the status of BACKPLANE_SLOT_1_CONFIG 0: CONFIGURATION_OK 1: CONFIGURATION_MISMATCH */
    uint8_t backplane_slot_1_config_status : 1;
    uint8_t reserved_24                    : 1;
    #endif
} __attribute__((packed)) Computer_config_mismatch_slot_list_t;

/** Indicates the Data-link status of DTN switch ports. */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t fo_1_port_link_status : 1;
    uint8_t fo_2_port_link_status : 1;
    uint8_t cross_dsm_link_status : 1;
    uint8_t dsm_es_link_status : 1;
    uint8_t reserved_26 : 4;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint8_t reserved_26 : 4;
    /** Indicates the status of DSM_ES_LINK 0: GOOD_LINK 1: BAD_LINK */
    uint8_t dsm_es_link_status : 1;
    /** Indicates the status of CROSS_DSM_LINK 0: GOOD_LINK 1: BAD_LINK */
    uint8_t cross_dsm_link_status : 1;
    /** Indicates the status of FO_2_PORT_LINK 0: GOOD_LINK 1: BAD_LINK */
    uint8_t fo_2_port_link_status : 1;
    /** Indicates the status of FO_1_PORT_LINK 0: GOOD_LINK 1: BAD_LINK */
    uint8_t fo_1_port_link_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t payload_1_link_status : 1;
    uint8_t payload_2_link_status : 1;
    uint8_t payload_3_link_status : 1;
    uint8_t payload_4_link_status : 1;
    uint8_t payload_5_link_status : 1;
    uint8_t payload_6_link_status : 1;
    uint8_t payload_7_link_status : 1;
    uint8_t payload_8_link_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of PAYLOAD_8_LINK 0: GOOD_LINK 1: BAD_LINK */
    uint8_t payload_8_link_status : 1;
    /** Indicates the status of PAYLOAD_7_LINK 0: GOOD_LINK 1: BAD_LINK */
    uint8_t payload_7_link_status : 1;
    /** Indicates the status of PAYLOAD_6_LINK 0: GOOD_LINK 1: BAD_LINK */
    uint8_t payload_6_link_status : 1;
    /** Indicates the status of PAYLOAD_5_LINK 0: GOOD_LINK 1: BAD_LINK */
    uint8_t payload_5_link_status : 1;
    /** Indicates the status of PAYLOAD_4_LINK 0: GOOD_LINK 1: BAD_LINK */
    uint8_t payload_4_link_status : 1;
    /** Indicates the status of PAYLOAD_3_LINK 0: GOOD_LINK 1: BAD_LINK */
    uint8_t payload_3_link_status : 1;
    /** Indicates the status of PAYLOAD_2_LINK 0: GOOD_LINK 1: BAD_LINK */
    uint8_t payload_2_link_status : 1;
    /** Indicates the status of PAYLOAD_1_LINK 0: GOOD_LINK 1: BAD_LINK */
    uint8_t payload_1_link_status : 1;
    #endif
} __attribute__((packed)) Dtn_switch_port_link_status_t;

/** Indicates whether the DSM in the Slot is the same defined in the configuration data. */
typedef struct
{
    /** Unique slot identifier within the computer */
    Slot_identifier_t slot_idx;
    /** Computer Level CMSW deployed on the DSM sends status report messages periodically. This field tests whether DSM is alive or not based on messages received by MMP Level CMSW and IPMC status of the DSM. 0: MODULE_ALIVE 1: MODULE_LOSS */
    uint8_t module_status : 8;
    /** Generic IPMC status of this module */
    Generic_ipmc_status_t generic_ipmc_status;
    /** Indicates whether the LRM in the Slot is the same defined in the configuration data */
    Computer_config_mismatch_slot_list_t computer_config_mismatch_slot_list;
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t dtn_es_redundancy_status : 1;
    uint8_t ptp_config_status : 1;
    uint8_t ptp_sync_status : 1;
    uint8_t dtn_switch_config_status : 1;
    uint8_t dtn_es_config_status : 1;
    uint8_t dtn_sw_data_validity : 1;
    uint8_t dtn_es_data_validity : 1;
    uint8_t module_major_frame_alignment_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** NOT_ALIGNED: Module has not been aligned yet. ALIGNED: Module has been aligned 0: ALIGNED 1: NOT_ALIGNED */
    uint8_t module_major_frame_alignment_status : 1;
    /** Whether following DTN ES Data is valid or not. 0: VALID 1: INVALID */
    uint8_t dtn_es_data_validity : 1;
    /** Whether following DTN SW Data is valid or not. 0: VALID 1: INVALID */
    uint8_t dtn_sw_data_validity : 1;
    /** CONFIG_MATCH: DTN End System Configuration ID reported by this module matches with DTN End System Configuration ID for current platform configuration in the configuration data. CONFIG_MISMATCH: DTN End System Configuration ID reported by this module does not match with DTN End System Configuration ID for current platform configuration in the configuration data. 0: CONFIG_MATCH 1: CONFIG_MISMATCH */
    uint8_t dtn_es_config_status : 1;
    /** CONFIG_MATCH: DTN Switch Configuration ID reported by this module matches with DTN Switch Configuration ID for current platform configuration in the configuration data. CONFIG_MISMATCH: DTN Switch Configuration ID reported by this module does not match with DTN Switch Configuration ID for current platform configuration in the configuration data. 0: CONFIG_MATCH 1: CONFIG_MISMATCH */
    uint8_t dtn_switch_config_status : 1;
    /** SYNCHED: If the module receives both PTP synch messages and PTP response messages according to the PTP configuration. NOT_SYNCHED: If the module does not recieve PTP synch messages and/or PTP response messages according to the PTP configuration. 0: SYNCHED 1: NOT_SYNCHED */
    uint8_t ptp_sync_status : 1;
    /** CONFIG_MATCH: PTP Configuration ID reported by this module matches with PTP Configuration ID for this module in current platform configuration in the MMP Level CMSW Configuration Data. CONFIG_MISMATCH: PTP Configuration ID reported by this module does not match with DTN ES Configuration ID for this module in current platform configuration in the MMP Level CMSW Configuration Data. 0: CONFIG_MATCH 1: CONFIG_MISMATCH */
    uint8_t ptp_config_status : 1;
    /** REDUNDANCY_OK: DTN A & B link status is good REDUNDANCY_LOSS: Loss of DTN A or B link 0: REDUNDANCY_OK 1: REDUNDANCY_LOSS */
    uint8_t dtn_es_redundancy_status : 1;
    #endif
    /** Indicates the Data-link status of DTN switch ports */
    Dtn_switch_port_link_status_t dtn_switch_port_link_status;
    /** Indicates DTN Switch Custom Design Version Major part. Major change is one that affects the requirements, external interfaces, cost, and/or schedule. */
    uint8_t dtn_switch_custom_design_version_major : 8;
    /** Indicates DTN Switch Custom Design Version Minor part. Minor changes are those that do not affect form, fit, function, cost, or schedule. */
    uint8_t dtn_switch_custom_design_version_minor : 8;
    /** Indicates DTN Switch Custom Design Version Bugfix part. */
    uint8_t dtn_switch_custom_design_version_bugfix : 8; /** Indicates the availability of temperature data. 0: TEMP_DATA_AVAILABLE 1: TEMP_DATA_LOSS */
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t reserved_27         : 6;
    uint8_t voltage_data_status : 1;
    uint8_t temperature_data_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint8_t temperature_data_status                 : 1;
    /** Indicates the availability of voltage data. 0: VOLTAGE_DATA_AVAILABLE 1: VOLTAGE_DATA_LOSS */
    uint8_t voltage_data_status : 1;
    uint8_t reserved_27         : 6;
    #endif
    /** Indicates the result of T2080_dsm_BOOTLOADER_PBIT */
    Mmplcmsw_t2080_dsm_bootloader_pbit_t t2080_dsm_bootloader_pbit;
} __attribute__((packed)) Dsm_status_list_t;

/** Indicates whether the IOCM in the Slot is the same defined in the configuration data. */
typedef struct
{
    /** Unique slot identifier within the computer */
    Slot_identifier_t slot_idx;
    /** Availability status of the MCU on the LRM to chassis manager. 0: MODULE_ALIVE 1: MODULE_LOSS */
    uint8_t module_status : 8;
    /** Generic IPMC status of this module */
    Generic_ipmc_status_t generic_ipmc_status;
   #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t reserved_84     : 5;
    uint8_t iocm_dvi_status : 1;
    uint8_t voltage_data_status : 1;
    uint8_t temperature_data_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the availability of temperature data. 0: TEMP_DATA_AVAILABLE 1: TEMP_DATA_LOSS */
    uint8_t temperature_data_status : 1;
    /** Indicates the availability of voltage data. 0: VOLTAGE_DATA_AVAILABLE 1: VOLTAGE_DATA_LOSS */
    uint8_t voltage_data_status : 1;
    /** There are undetected loss of dvi conditions. Therefore, dvi loss may also occur for other reasons. 0: DVI_UNKNOWN 1: DVI_LOSS */
    uint8_t iocm_dvi_status : 1;
    uint8_t reserved_84     : 5;
    #endif
} __attribute__((packed)) Iocm_status_t;

/** Indicates whether the HUM in the Slot is the same defined in the configuration data. */
typedef struct
{
    /** Unique slot identifier within the computer */
    Slot_identifier_t slot_idx;
    /** Availability status of the MCU on the LRM to chassis manager. 0: MODULE_ALIVE 1: MODULE_LOSS */
    uint8_t module_status : 8;
    /** Generic IPMC status of this module */
    Generic_ipmc_status_t generic_ipmc_status;
} __attribute__((packed)) Hum_status_list_t;

/** List of computer status in the platform */
typedef struct
{
    /** Unique ID of computer in the platform */
    uint8_t computer_lru_id : 8;
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t reserved_5                        : 5;
    uint8_t computer_chassis_manager_validity : 1;
    uint8_t computer_status                   : 2;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Status of LRU */
    /** 0: LRU_ALIVE 1: LRU_LOSS 2: LRU_HOT */
    uint8_t computer_status : 2;
    /** Chassis manager validity status of an MMP Computer. Chassis Manager Validity Status is used for determining
     * whether LRM IPMC Status fields in the MMP Status Report Message is valid or not. */
    /** 0: VALID 1: INVALID */
    uint8_t computer_chassis_manager_validity : 1;
    uint8_t reserved_5                        : 5;
    #endif
    uint8_t reserved_6                        : 8;
    /** An array of DPM Status */
    Dpm_status_list_t dpm_status_list[5];
    uint8_t           reserved_9 : 8;
    /** An array of PSM Status */
    Psm_status_list_t psm_status_list[2];
    uint8_t           reserved_11 : 8;
    /** Indicates the status of an SMMM defined in the MMP Level CMSW configuration */
    Smmm_status_t smmm_status;
    uint8_t       reserved_17 : 8;
    /** An array of GPM Status */
    Gpm_status_list_t gpm_status_list[2];
    uint8_t           reserved_20 : 8;
    /** An array of HSM Status */
    Hsm_status_list_t hsm_status_list[2];
    uint8_t           reserved_22 : 8;
    /** An array of DSM Status */
    Dsm_status_list_t dsm_status_list[2];
    uint8_t           reserved_28 : 8;
    /** IOCM Status */
    Iocm_status_t iocm_status;
    uint8_t       reserved_31 : 8;
    /** An array of HUM Status */
    Hum_status_list_t hum_status_list[2];
} __attribute__((packed)) Computer_status_list_t;

/** Indicates the Config Status of DCU */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t m1553_lut_0x111_cfg_match_status : 1;
    uint8_t m1553_lut_0x110_cfg_match_status : 1;
    uint8_t m1553_inst_list_0x111_cfg_match_status : 1;
    uint8_t m1553_inst_list_0x110_cfg_match_status : 1;
    uint8_t dcu_vl_channel_config_match_status : 1;
    uint8_t dcu_channel_config_match_status : 1;
    uint8_t m1553_0x111_bus_start_receive_status : 1;
    uint8_t m1553_0x110_bus_start_receive_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates whether M1553 Bus Start command received for channel 0x110 by the DCU. 0: RECEIVED 1: NOT_RECEIVED */
    uint8_t m1553_0x110_bus_start_receive_status : 1;
    /** Indicates whether M1553 Bus Start command received for channel 0x111 by the DCU. 0: RECEIVED 1: NOT_RECEIVED */
    uint8_t m1553_0x111_bus_start_receive_status : 1;
    /** Indicates whether DCU Channel Configuration ID reported by the DCU matches with expected configuration ID in the MMP Level CMSW configuration data or not. 0: CONFIG_MATCH 1: CONFIG_MISMATCH */
    uint8_t dcu_channel_config_match_status : 1;
    /** Indicates whether DCU VL Channel Configuration ID reported by the DCU matches with expected configuration ID in the MMP Level CMSW configuration data or not. 0: CONFIG_MATCH 1: CONFIG_MISMATCH */
    uint8_t dcu_vl_channel_config_match_status : 1;
    /** Indicates whether M1553 Instruction List Channel 0x110 Configuration ID reported by the DCU matches with expected configuration ID in the MMP Level CMSW configuration data or not. 0: CONFIG_MATCH 1: CONFIG_MISMATCH */
    uint8_t m1553_inst_list_0x110_cfg_match_status : 1;
    /** Indicates whether M1553 Instruction List Channel 0x111 Configuration ID reported by the DCU matches with expected configuration ID in the MMP Level CMSW configuration data or not. 0: CONFIG_MATCH 1: CONFIG_MISMATCH */
    uint8_t m1553_inst_list_0x111_cfg_match_status : 1;
    /** Indicates whether M1553 LUT Channel 0x110 Configuration ID reported by the DCU matches with expected configuration ID in the MMP Level CMSW configuration data or not. 0: CONFIG_MATCH 1: CONFIG_MISMATCH */
    uint8_t m1553_lut_0x110_cfg_match_status : 1;
    /** Indicates whether M1553 LUT Channel 0x111 Configuration ID reported by the DCU matches with expected configuration ID in the MMP Level CMSW configuration data or not. 0: CONFIG_MATCH 1: CONFIG_MISMATCH */
    uint8_t m1553_lut_0x111_cfg_match_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t heartbeat_resp : 1;
    uint8_t major_frame_sync_resp : 1;
    uint8_t m1553_lut_rcv_status_0x111_resp : 1;
    uint8_t m1553_lut_rcv_status_0x110_resp : 1;
    uint8_t m1553_inst_list_status_0x111_resp : 1;
    uint8_t m1553_inst_list_status_0x110_resp : 1;
    uint8_t dcu_vl_channel_config_status_resp : 1;
    uint8_t dcu_channel_config_status_resp : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of DCU Channel configuration status response reported by DCU. This field tests for some potential errors that may happen during DCU channel configuration. DCU channel configuration is composed of 2 DTN packets. GOOD: Both DCU channel configuration packets received by the DCU have the same configuration ID and Structure version of both DCU Channel Configuration packets received by the DCU are compatible with DCU firmware version. BAD: DCU Channel configuration failed. 0: GOOD 1: BAD */
    uint8_t dcu_channel_config_status_resp : 1;
    /** Indicates the status of DCU VL - Channel configuration status response reported by DCU. This field tests for some potential errors that may happen during DCU VL - Channel configuration. DCU VL - Channel configuration is composed of 2 DTN packets. GOOD: Both DCU VL - Channel configuration packets received by the DCU have the same configuration ID. BAD: DCU VL -Channel configuration failed. 0: GOOD 1: BAD */
    uint8_t dcu_vl_channel_config_status_resp : 1;
    /** Indicates the receive status of M1553 Instruction List Status Channel 0x110 Configuration reported by DCU. GOOD: DCU received MIL-STD 1553 Instruction List Configuration for Channel 0x110. BAD: DCU has not received MIL-STD 1553 Instruction List Configuration for Channel 0x110. This is a configuration failure. 0: GOOD 1: BAD */
    uint8_t m1553_inst_list_status_0x110_resp : 1;
    /** Indicates the receive status of M1553 Instruction List Status Channel 0x111 Configuration reported by DCU. GOOD: DCU received MIL-STD 1553 Instruction List Configuration for Channel 0x111. BAD: DCU has not received MIL-STD 1553 Instruction List Configuration for Channel 0x111. This is a configuration failure. 0: GOOD 1: BAD */
    uint8_t m1553_inst_list_status_0x111_resp : 1;
    /** Indicates the receive status of M1553 LUT Channel 0x110 Configuration reported by DCU. GOOD: DCU received MIL-STD 1553 LUT Configuration for Channel 0x110. BAD: DCU has not received MIL-STD 1553 LUT Configuration for Channel 0x110. This is a configuration failure. 0: GOOD 1: BAD */
    uint8_t m1553_lut_rcv_status_0x110_resp : 1;
    /** Indicates the receive status of M1553 LUT Channel 0x111 Configuration reported by DCU. GOOD: DCU received MIL-STD 1553 LUT Configuration for Channel 0x111. BAD: DCU has not received MIL-STD 1553 LUT Configuration for Channel 0x111. This is a configuration failure. 0: GOOD 1: BAD */
    uint8_t m1553_lut_rcv_status_0x111_resp : 1;
    /** Indicates the status of Major Frame Sync Response reported by DCU. GOOD: DCU received Major Frame Synchronization message. BAD: Major Frame synchronization has failed or not started. 0: GOOD 1: BAD */
    uint8_t major_frame_sync_resp : 1;
    /** Indicates the Computer Heartbeat Response reported by DCU. GOOD: DCU received Computer Hearbeat within timeout time defined in DCU Channel Configuration. BAD: DCU has not received Computer Hearbeat within timeout time defined in DCU Channel Configuration. 0: GOOD 1: BAD */
    uint8_t heartbeat_resp : 1;
    #endif
} __attribute__((packed)) Dcu_config_status_t;

/** Indicates the status of DOGO Channels */
typedef struct
{
   #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t dogo_ch24_status : 1;
    uint8_t dogo_ch25_status : 1;
    uint8_t dogo_ch26_status : 1;
    uint8_t dogo_ch27_status : 1;
    uint8_t dogo_ch28_status : 1;
    uint8_t dogo_ch29_status : 1;
    uint8_t dogo_ch30_status : 1;
    uint8_t dogo_ch31_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of DOGO_CH31 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch31_status : 1;
    /** Indicates the status of DOGO_CH30 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch30_status : 1;
    /** Indicates the status of DOGO_CH29 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch29_status : 1;
    /** Indicates the status of DOGO_CH28 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch28_status : 1;
    /** Indicates the status of DOGO_CH27 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch27_status : 1;
    /** Indicates the status of DOGO_CH26 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch26_status : 1;
    /** Indicates the status of DOGO_CH25 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch25_status : 1;
    /** Indicates the status of DOGO_CH24 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch24_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t dogo_ch16_status : 1;
    uint8_t dogo_ch17_status : 1;
    uint8_t dogo_ch18_status : 1;
    uint8_t dogo_ch19_status : 1;
    uint8_t dogo_ch20_status : 1;
    uint8_t dogo_ch21_status : 1;
    uint8_t dogo_ch22_status : 1;
    uint8_t dogo_ch23_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of DOGO_CH23 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch23_status : 1;
    /** Indicates the status of DOGO_CH22 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch22_status : 1;
    /** Indicates the status of DOGO_CH21 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch21_status : 1;
    /** Indicates the status of DOGO_CH20 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch20_status : 1;
    /** Indicates the status of DOGO_CH19 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch19_status : 1;
    /** Indicates the status of DOGO_CH18 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch18_status : 1;
    /** Indicates the status of DOGO_CH17 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch17_status : 1;
    /** Indicates the status of DOGO_CH16 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch16_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t dogo_ch08_status : 1;
    uint8_t dogo_ch09_status : 1;
    uint8_t dogo_ch10_status : 1;
    uint8_t dogo_ch11_status : 1;
    uint8_t dogo_ch12_status : 1;
    uint8_t dogo_ch13_status : 1;
    uint8_t dogo_ch14_status : 1;
    uint8_t dogo_ch15_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of DOGO_CH15 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch15_status : 1;
    /** Indicates the status of DOGO_CH14 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch14_status : 1;
    /** Indicates the status of DOGO_CH13 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch13_status : 1;
    /** Indicates the status of DOGO_CH12 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch12_status : 1;
    /** Indicates the status of DOGO_CH11 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch11_status : 1;
    /** Indicates the status of DOGO_CH10 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch10_status : 1;
    /** Indicates the status of DOGO_CH09 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch09_status : 1;
    /** Indicates the status of DOGO_CH08 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch08_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t dogo_ch00_status : 1;
    uint8_t dogo_ch01_status : 1;
    uint8_t dogo_ch02_status : 1;
    uint8_t dogo_ch03_status : 1;
    uint8_t dogo_ch04_status : 1;
    uint8_t dogo_ch05_status : 1;
    uint8_t dogo_ch06_status : 1;
    uint8_t dogo_ch07_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of DOGO_CH07 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch07_status : 1;
    /** Indicates the status of DOGO_CH06 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch06_status : 1;
    /** Indicates the status of DOGO_CH05 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch05_status : 1;
    /** Indicates the status of DOGO_CH04 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch04_status : 1;
    /** Indicates the status of DOGO_CH03 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch03_status : 1;
    /** Indicates the status of DOGO_CH02 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch02_status : 1;
    /** Indicates the status of DOGO_CH01 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch01_status : 1;
    /** Indicates the status of DOGO_CH00 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t dogo_ch00_status : 1;
    #endif
} __attribute__((packed)) Channel_group_00_status_t;

/** Indicates the status of DOGO Channels. */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint32_t dogo_ch32_status : 1;
    uint32_t dogo_ch33_status : 1;
    uint32_t dogo_ch34_status : 1;
    uint32_t dogo_ch35_status : 1;
    uint32_t dogo_ch36_status : 1;
    uint32_t dogo_ch37_status : 1;
    uint32_t dogo_ch38_status : 1;
    uint32_t dogo_ch39_status : 1;
    uint32_t dogo_ch40_status : 1;
    uint32_t dogo_ch41_status : 1;
    uint32_t reserved_34 : 22;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint32_t reserved_34 : 22;
    /** Indicates the status of DOGO_CH41 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t dogo_ch41_status : 1;
    /** Indicates the status of DOGO_CH40 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t dogo_ch40_status : 1;
    /** Indicates the status of DOGO_CH39 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t dogo_ch39_status : 1;
    /** Indicates the status of DOGO_CH38 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t dogo_ch38_status : 1;
    /** Indicates the status of DOGO_CH37 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t dogo_ch37_status : 1;
    /** Indicates the status of DOGO_CH36 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t dogo_ch36_status : 1;
    /** Indicates the status of DOGO_CH35 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t dogo_ch35_status : 1;
    /** Indicates the status of DOGO_CH34 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t dogo_ch34_status : 1;
    /** Indicates the status of DOGO_CH33 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t dogo_ch33_status : 1;
    /** Indicates the status of DOGO_CH32 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t dogo_ch32_status : 1;
    #endif
} __attribute__((packed)) Channel_group_01_status_t;

/** Indicates the status of DOOS Channels. */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint32_t doos_ch00_status : 1;
    uint32_t doos_ch01_status : 1;
    uint32_t reserved_35 : 30;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint32_t reserved_35 : 30;
    /** Indicates the status of DOOS_CH01. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t doos_ch01_status : 1;
    /** Indicates the status of DOOS_CH00. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t doos_ch00_status : 1;
    #endif
} __attribute__((packed)) Channel_group_02_status_t;

/** Indicates the status of DOSO Channels. */
typedef struct
{
   #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t doso_ch24_status : 1;
    uint8_t doso_ch25_status : 1;
    uint8_t reserved_37 : 1;
    uint8_t reserved_36 : 5;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint8_t reserved_36 : 5;
    uint8_t reserved_37 : 1;
    /** Indicates the status of DOSO_CH25. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch25_status : 1;
    /** Indicates the status of DOSO_CH24. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch24_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t doso_ch16_status : 1;
    uint8_t doso_ch17_status : 1;
    uint8_t doso_ch18_status : 1;
    uint8_t doso_ch19_status : 1;
    uint8_t doso_ch20_status : 1;
    uint8_t doso_ch21_status : 1;
    uint8_t doso_ch22_status : 1;
    uint8_t doso_ch23_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of DOSO_CH23. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch23_status : 1;
    /** Indicates the status of DOSO_CH22. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch22_status : 1;
    /** Indicates the status of DOSO_CH21. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch21_status : 1;
    /** Indicates the status of DOSO_CH20. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch20_status : 1;
    /** Indicates the status of DOSO_CH19. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch19_status : 1;
    /** Indicates the status of DOSO_CH18. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch18_status : 1;
    /** Indicates the status of DOSO_CH17. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch17_status : 1;
    /** Indicates the status of DOSO_CH16. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch16_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t doso_ch08_status : 1;
    uint8_t doso_ch09_status : 1;
    uint8_t doso_ch10_status : 1;
    uint8_t doso_ch11_status : 1;
    uint8_t doso_ch12_status : 1;
    uint8_t doso_ch13_status : 1;
    uint8_t doso_ch14_status : 1;
    uint8_t doso_ch15_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of DOSO_CH15. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch15_status : 1;
    /** Indicates the status of DOSO_CH14. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch14_status : 1;
    /** Indicates the status of DOSO_CH13. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch13_status : 1;
    /** Indicates the status of DOSO_CH12. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch12_status : 1;
    /** Indicates the status of DOSO_CH11. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch11_status : 1;
    /** Indicates the status of DOSO_CH10. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch10_status : 1;
    /** Indicates the status of DOSO_CH09. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch09_status : 1;
    /** Indicates the status of DOSO_CH08. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch08_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t doso_ch00_status : 1;
    uint8_t doso_ch01_status : 1;
    uint8_t doso_ch02_status : 1;
    uint8_t doso_ch03_status : 1;
    uint8_t doso_ch04_status : 1;
    uint8_t doso_ch05_status : 1;
    uint8_t doso_ch06_status : 1;
    uint8_t doso_ch07_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of DOSO_CH07. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch07_status : 1;
    /** Indicates the status of DOSO_CH06. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch06_status : 1;
    /** Indicates the status of DOSO_CH05. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch05_status : 1;
    /** Indicates the status of DOSO_CH04. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch04_status : 1;
    /** Indicates the status of DOSO_CH03. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch03_status : 1;
    /** Indicates the status of DOSO_CH02. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch02_status : 1;
    /** Indicates the status of DOSO_CH01. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch01_status : 1;
    /** Indicates the status of DOSO_CH00. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint8_t doso_ch00_status : 1;
    #endif
} __attribute__((packed)) Channel_group_03_status_t;

/** Indicates the status of  DIGO Channels (CAS, ICNI). */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t cas_digo_ch21_status : 1;
    uint8_t cas_digo_ch22_status : 1;
    uint8_t cas_digo_ch23_status : 1;
    uint8_t cas_digo_ch24_status : 1;
    uint8_t cas_digo_ch25_status : 1;
    uint8_t cas_digo_ch26_status : 1;
    uint8_t cas_digo_ch27_status : 1;
    uint8_t cas_digo_ch28_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of CAS_DIGO_CH28. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch28_status : 1;
    /** Indicates the status of CAS_DIGO_CH27. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch27_status : 1;
    /** Indicates the status of CAS_DIGO_CH26. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch26_status : 1;
    /** Indicates the status of CAS_DIGO_CH25. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch25_status : 1;
    /** Indicates the status of CAS_DIGO_CH24. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch24_status : 1;
    /** Indicates the status of CAS_DIGO_CH23. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch23_status : 1;
    /** Indicates the status of CAS_DIGO_CH22. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch22_status : 1;
    /** Indicates the status of CAS_DIGO_CH21. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch21_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t cas_digo_ch13_status : 1;
    uint8_t cas_digo_ch14_status : 1;
    uint8_t cas_digo_ch15_status : 1;
    uint8_t cas_digo_ch16_status : 1;
    uint8_t cas_digo_ch17_status : 1;
    uint8_t cas_digo_ch18_status : 1;
    uint8_t cas_digo_ch19_status : 1;
    uint8_t cas_digo_ch20_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of CAS_DIGO_CH20. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch20_status : 1;
    /** Indicates the status of CAS_DIGO_CH19. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch19_status : 1;
    /** Indicates the status of CAS_DIGO_CH18. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch18_status : 1;
    /** Indicates the status of CAS_DIGO_CH17. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch17_status : 1;
    /** Indicates the status of CAS_DIGO_CH16. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch16_status : 1;
    /** Indicates the status of CAS_DIGO_CH15. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch15_status : 1;
    /** Indicates the status of CAS_DIGO_CH14. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch14_status : 1;
    /** Indicates the status of CAS_DIGO_CH13. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch13_status : 1;
    #endif
   #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t cas_digo_ch05_status : 1;
    uint8_t cas_digo_ch06_status : 1;
    uint8_t cas_digo_ch07_status : 1;
    uint8_t cas_digo_ch08_status : 1;
    uint8_t cas_digo_ch09_status : 1;
    uint8_t cas_digo_ch10_status : 1;
    uint8_t cas_digo_ch11_status : 1;
    uint8_t cas_digo_ch12_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of CAS_DIGO_CH12. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch12_status : 1;
    /** Indicates the status of CAS_DIGO_CH11. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch11_status : 1;
    /** Indicates the status of CAS_DIGO_CH10. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch10_status : 1;
    /** Indicates the status of CAS_DIGO_CH09. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch09_status : 1;
    /** Indicates the status of CAS_DIGO_CH08. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch08_status : 1;
    /** Indicates the status of DIGO_CH07. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch07_status : 1;
    /** Indicates the status of CAS_DIGO_CH06. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch06_status : 1;
    /** Indicates the status of CAS_DIGO_CH05. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch05_status : 1;
    /** Indicates the status of CAS_DIGO_CH04. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch04_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t icni_digo_ch00_status : 1;
    uint8_t icni_digo_ch01_status : 1;
    uint8_t icni_digo_ch02_status : 1;
    uint8_t cas_digo_ch00_status : 1;
    uint8_t cas_digo_ch01_status : 1;
    uint8_t cas_digo_ch02_status : 1;
    uint8_t cas_digo_ch03_status : 1;
    uint8_t cas_digo_ch04_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of CAS_DIGO_CH04. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch04_status : 1;
    /** Indicates the status of CAS_DIGO_CH03. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch03_status : 1;
    /** Indicates the status of CAS_DIGO_CH02. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch02_status : 1;
    /** Indicates the status of CAS_DIGO_CH01. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch01_status : 1;
    /** Indicates the status of CAS_DIGO_CH00. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch00_status : 1;
    /** Indicates the status of ICNI_DIGO_CH02. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t icni_digo_ch02_status : 1;
    /** Indicates the status of ICNI_DIGO_CH01. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t icni_digo_ch01_status : 1;
    /** Indicates the status of ICNI_DIGO_CH00. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t icni_digo_ch00_status : 1;
    #endif
} __attribute__((packed)) Channel_group_04_status_t;

/** Indicates the status of DIGO Channels (CAS). */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t cas_digo_ch53_status : 1;
    uint8_t cas_digo_ch54_status : 1;
    uint8_t cas_digo_ch55_status : 1;
    uint8_t cas_digo_ch56_status : 1;
    uint8_t cas_digo_ch57_status : 1;
    uint8_t cas_digo_ch58_status : 1;
    uint8_t cas_digo_ch59_status : 1;
    uint8_t cas_digo_ch60_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of CAS_DIGO_CH60. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch60_status : 1;
    /** Indicates the status of CAS_DIGO_CH59. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch59_status : 1;
    /** Indicates the status of CAS_DIGO_CH58. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch58_status : 1;
    /** Indicates the status of CAS_DIGO_CH57. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch57_status : 1;
    /** Indicates the status of CAS_DIGO_CH56. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch56_status : 1;
    /** Indicates the status of CAS_DIGO_CH55. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch55_status : 1;
    /** Indicates the status of CAS_DIGO_CH54. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch54_status : 1;
    /** Indicates the status of CAS_DIGO_CH53. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch53_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t cas_digo_ch45_status : 1;
    uint8_t cas_digo_ch46_status : 1;
    uint8_t cas_digo_ch47_status : 1;
    uint8_t cas_digo_ch48_status : 1;
    uint8_t cas_digo_ch49_status : 1;
    uint8_t cas_digo_ch50_status : 1;
    uint8_t cas_digo_ch51_status : 1;
    uint8_t cas_digo_ch52_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of CAS_DIGO_CH52. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch52_status : 1;
    /** Indicates the status of CAS_DIGO_CH51. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch51_status : 1;
    /** Indicates the status of CAS_DIGO_CH50. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch50_status : 1;
    /** Indicates the status of CAS_DIGO_CH49. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch49_status : 1;
    /** Indicates the status of CAS_DIGO_CH48. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch48_status : 1;
    /** Indicates the status of CAS_DIGO_CH47. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch47_status : 1;
    /** Indicates the status of CAS_DIGO_CH46. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch46_status : 1;
    /** Indicates the status of CAS_DIGO_CH45. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch45_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t cas_digo_ch37_status : 1;
    uint8_t cas_digo_ch38_status : 1;
    uint8_t cas_digo_ch39_status : 1;
    uint8_t cas_digo_ch40_status : 1;
    uint8_t cas_digo_ch41_status : 1;
    uint8_t cas_digo_ch42_status : 1;
    uint8_t cas_digo_ch43_status : 1;
    uint8_t cas_digo_ch44_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of CAS_DIGO_CH44. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch44_status : 1;
    /** Indicates the status of CAS_DIGO_CH43. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch43_status : 1;
    /** Indicates the status of CAS_DIGO_CH42. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch42_status : 1;
    /** Indicates the status of CAS_DIGO_CH41. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch41_status : 1;
    /** Indicates the status of CAS_DIGO_CH40. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch40_status : 1;
    /** Indicates the status of CAS_DIGO_CH39. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch39_status : 1;
    /** Indicates the status of CAS_DIGO_CH38. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch38_status : 1;
    /** Indicates the status of CAS_DIGO_CH37. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch37_status : 1;
    #endif
   #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t cas_digo_ch29_status : 1;
    uint8_t cas_digo_ch30_status : 1;
    uint8_t cas_digo_ch31_status : 1;
    uint8_t cas_digo_ch32_status : 1;
    uint8_t cas_digo_ch33_status : 1;
    uint8_t cas_digo_ch34_status : 1;
    uint8_t cas_digo_ch35_status : 1;
    uint8_t cas_digo_ch36_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of CAS_DIGO_CH36. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch36_status : 1;
    /** Indicates the status of CAS_DIGO_CH35. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch35_status : 1;
    /** Indicates the status of CAS_DIGO_CH34. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch34_status : 1;
    /** Indicates the status of CAS_DIGO_CH33. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch33_status : 1;
    /** Indicates the status of CAS_DIGO_CH32. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch32_status : 1;
    /** Indicates the status of CAS_DIGO_CH31. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch31_status : 1;
    /** Indicates the status of CAS_DIGO_CH30. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch30_status : 1;
    /** Indicates the status of CAS_DIGO_CH29. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch29_status : 1;
    #endif
} __attribute__((packed)) Channel_group_05_status_t;

/** Indicates the status of DIGO Channels (CAS, IEOS). */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t ieos_digo_ch04_status : 1;
    uint8_t ieos_digo_ch05_status : 1;
    uint8_t ieos_digo_ch06_status : 1;
    uint8_t ieos_digo_ch07_status : 1;
    uint8_t ieos_digo_ch08_status : 1;
    uint8_t ieos_digo_ch09_status : 1;
    uint8_t ieos_digo_ch10_status : 1;
    uint8_t ieos_digo_ch11_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of IEOS_DIGO_CH11. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t ieos_digo_ch11_status : 1;
    /** Indicates the status of IEOS_DIGO_CH10. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t ieos_digo_ch10_status : 1;
    /** Indicates the status of IEOS_DIGO_CH09. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t ieos_digo_ch09_status : 1;
    /** Indicates the status of IEOS_DIGO_CH08. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t ieos_digo_ch08_status : 1;
    /** Indicates the status of IEOS_DIGO_CH07. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t ieos_digo_ch07_status : 1;
    /** Indicates the status of IEOS_DIGO_CH06. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t ieos_digo_ch06_status : 1;
    /** Indicates the status of IEOS_DIGO_CH05. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t ieos_digo_ch05_status : 1;
    /** Indicates the status of IEOS_DIGO_CH04. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t ieos_digo_ch04_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t cas_digo_ch77_status : 1;
    uint8_t cas_digo_ch78_status : 1;
    uint8_t cas_digo_ch79_status : 1;
    uint8_t cas_digo_ch80_status : 1;
    uint8_t ieos_digo_ch00_status : 1;
    uint8_t ieos_digo_ch01_status : 1;
    uint8_t ieos_digo_ch02_status : 1;
    uint8_t ieos_digo_ch03_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of IEOS_DIGO_CH03. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t ieos_digo_ch03_status : 1;
    /** Indicates the status of IEOS_DIGO_CH02. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t ieos_digo_ch02_status : 1;
    /** Indicates the status of IEOS_DIGO_CH01. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t ieos_digo_ch01_status : 1;
    /** Indicates the status of IEOS_IEOS_DIGO_CH00. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t ieos_digo_ch00_status : 1;
    /** Indicates the status of CAS_DIGO_CH80. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch80_status : 1;
    /** Indicates the status of CAS_DIGO_CH79. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch79_status : 1;
    /** Indicates the status of CAS_DIGO_CH78. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch78_status : 1;
    /** Indicates the status of CAS_DIGO_CH77. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch77_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t cas_digo_ch69_status : 1;
    uint8_t cas_digo_ch70_status : 1;
    uint8_t cas_digo_ch71_status : 1;
    uint8_t cas_digo_ch72_status : 1;
    uint8_t cas_digo_ch73_status : 1;
    uint8_t cas_digo_ch74_status : 1;
    uint8_t cas_digo_ch75_status : 1;
    uint8_t cas_digo_ch76_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of CAS_DIGO_CH76. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch76_status : 1;
    /** Indicates the status of CAS_DIGO_CH75. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch75_status : 1;
    /** Indicates the status of CAS_DIGO_CH74. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch74_status : 1;
    /** Indicates the status of CAS_DIGO_CH73. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch73_status : 1;
    /** Indicates the status of CAS_DIGO_CH72. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch72_status : 1;
    /** Indicates the status of CAS_DIGO_CH71. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch71_status : 1;
    /** Indicates the status of CAS_DIGO_CH70. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch70_status : 1;
    /** Indicates the status of CAS_DIGO_CH69. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch69_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t cas_digo_ch61_status : 1;
    uint8_t cas_digo_ch62_status : 1;
    uint8_t cas_digo_ch63_status : 1;
    uint8_t cas_digo_ch64_status : 1;
    uint8_t cas_digo_ch65_status : 1;
    uint8_t cas_digo_ch66_status : 1;
    uint8_t cas_digo_ch67_status : 1;
    uint8_t cas_digo_ch68_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of CAS_DIGO_CH68. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch68_status : 1;
    /** Indicates the status of CAS_DIGO_CH67. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch67_status : 1;
    /** Indicates the status of CAS_DIGO_CH66. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch66_status : 1;
    /** Indicates the status of CAS_DIGO_CH65. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch65_status : 1;
    /** Indicates the status of CAS_DIGO_CH64. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch64_status : 1;
    /** Indicates the status of CAS_DIGO_CH63. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch63_status : 1;
    /** Indicates the status of CAS_DIGO_CH62. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch62_status : 1;
    /** Indicates the status of CAS_DIGO_CH61. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_digo_ch61_status : 1;
    #endif
} __attribute__((packed)) Channel_group_06_status_t;

/** Indicates the status of DISO Channels (WDS, IRFS, ICNI, IEOS, CAS). */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint32_t cas_diso_ch00_status : 1;
    uint32_t cas_diso_ch01_status : 1;
    uint32_t ieos_diso_ch00_status : 1;
    uint32_t icni_diso_ch00_status : 1;
    uint32_t icni_diso_ch01_status : 1;
    uint32_t irfs_diso_ch00_status : 1;
    uint32_t irfs_diso_ch01_status : 1;
    uint32_t irfs_diso_ch02_status : 1;
    uint32_t irfs_diso_ch03_status : 1;
    uint32_t wds_diso_ch00_status : 1;
    uint32_t wds_diso_ch01_status : 1;
    uint32_t wds_diso_ch02_status : 1;
    uint32_t wds_diso_ch03_status : 1;
    uint32_t wds_diso_ch04_status : 1;
    uint32_t wds_diso_ch05_status : 1;
    uint32_t reserved_38 : 17;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint32_t reserved_38 : 17;
    /** Indicates the status of WDS_DISO_CH05. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t wds_diso_ch05_status : 1;
    /** Indicates the status of WDS_DISO_CH04. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t wds_diso_ch04_status : 1;
    /** Indicates the status of WDS_DISO_CH03. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t wds_diso_ch03_status : 1;
    /** Indicates the status of WDS_DISO_CH02. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t wds_diso_ch02_status : 1;
    /** Indicates the status of WDS_DISO_CH01. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t wds_diso_ch01_status : 1;
    /** Indicates the status of WDS_DISO_CH00. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t wds_diso_ch00_status : 1;
    /** Indicates the status of IRFS_DISO_CH03. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t irfs_diso_ch03_status : 1;
    /** Indicates the status of IRFS_DISO_CH02. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t irfs_diso_ch02_status : 1;
    /** Indicates the status of IRFS_DISO_CH01. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t irfs_diso_ch01_status : 1;
    /** Indicates the status of IRFS_DISO_CH00. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t irfs_diso_ch00_status : 1;
    /** Indicates the status of ICNI_DISO_CH01. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t icni_diso_ch01_status : 1;
    /** Indicates the status of ICNI_DISO_CH00. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t icni_diso_ch00_status : 1;
    /** Indicates the status of IEOS_DISO_CH00. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t ieos_diso_ch00_status : 1;
    /** Indicates the status of CAS_DISO_CH01. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t cas_diso_ch01_status : 1;
    /** Indicates the status of CAS_DISO_CH00. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t cas_diso_ch00_status : 1;
    #endif
} __attribute__((packed)) Channel_group_07_status_t;

/** Indicates the status of AVI Channels (CAS). */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint32_t cas_avi_ch00_status : 1;
    uint32_t cas_avi_ch01_status : 1;
    uint32_t cas_avi_ch02_status : 1;
    uint32_t cas_avi_ch03_status : 1;
    uint32_t reserved_39 : 28;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint32_t reserved_39 : 28;
    /** Indicates the status of CAS_AVI_CH03. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t cas_avi_ch03_status : 1;
    /** Indicates the status of CAS_AVI_CH02. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t cas_avi_ch02_status : 1;
    /** Indicates the status of CAS_AVI_CH01. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t cas_avi_ch01_status : 1;
    /** Indicates the status of CAS_AVI_CH00. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t cas_avi_ch00_status : 1;
    #endif
} __attribute__((packed)) Channel_group_08_status_t;

/** Indicates the status of RS422 Channels (IEOS, ICNI, CAS). */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint16_t ieos_rs422_ch00_status : 1;
    uint16_t ieos_rs422_ch01_status : 1;
    uint16_t ieos_rs422_ch02_status : 1;
    uint16_t ieos_rs422_ch03_status : 1;
    uint16_t ieos_rs422_ch04_status : 1;
    uint16_t reserved_40 : 11;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint16_t reserved_40 : 11;
    /** Indicates the status of IEOS_RS422_CH04. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint16_t ieos_rs422_ch04_status : 1;
    /** Indicates the status of IEOS_RS422_CH03. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint16_t ieos_rs422_ch03_status : 1;
    /** Indicates the status of IEOS_RS422_CH02. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint16_t ieos_rs422_ch02_status : 1;
    /** Indicates the status of IEOS_RS422_CH01. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint16_t ieos_rs422_ch01_status : 1;
    /** Indicates the status of IEOS_RS422_CH00. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint16_t ieos_rs422_ch00_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t icni_rs422_ch01_status : 1;
    uint8_t icni_rs422_ch02_status : 1;
    uint8_t icni_rs422_ch03_status : 1;
    uint8_t icni_rs422_ch04_status : 1;
    uint8_t icni_rs422_ch05_status : 1;
    uint8_t irfs_rs422_ch00_status : 1;
    uint8_t irfs_rs422_ch01_status : 1;
    uint8_t irfs_rs422_ch02_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of IRFS_RS422_CH02. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t irfs_rs422_ch02_status : 1;
    /** Indicates the status of IRFS_RS422_CH01. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t irfs_rs422_ch01_status : 1;
    /** Indicates the status of IRFS_RS422_CH00. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t irfs_rs422_ch00_status : 1;
    /** Indicates the status of ICNI_RS422_CH05. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t icni_rs422_ch05_status : 1;
    /** Indicates the status of ICNI_RS422_CH04. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t icni_rs422_ch04_status : 1;
    /** Indicates the status of ICNI_RS422_CH03. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t icni_rs422_ch03_status : 1;
    /** Indicates the status of ICNI_RS422_CH02. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t icni_rs422_ch02_status : 1;
    /** Indicates the status of ICNI_RS422_CH01. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t icni_rs422_ch01_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t cas_rs422_ch00_status : 1;
    uint8_t cas_rs422_ch01_status : 1;
    uint8_t cas_rs422_ch02_status : 1;
    uint8_t cas_rs422_ch03_status : 1;
    uint8_t cas_rs422_ch04_status : 1;
    uint8_t cas_rs422_ch05_status : 1;
    uint8_t cas_rs422_ch06_status : 1;
    uint8_t icni_rs422_ch00_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of ICNI_RS422_CH00. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t icni_rs422_ch00_status : 1;
    /** Indicates the status of CAS_RS422_CH06. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_rs422_ch06_status : 1;
    /** Indicates the status of CAS_RS422_CH05. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_rs422_ch05_status : 1;
    /** Indicates the status of CAS_RS422_CH04. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_rs422_ch04_status : 1;
    /** Indicates the status of CAS_RS422_CH03. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_rs422_ch03_status : 1;
    /** Indicates the status of CAS_RS422_CH02. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_rs422_ch02_status : 1;
    /** Indicates the status of CAS_RS422_CH01. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_rs422_ch01_status : 1;
    /** Indicates the status of CAS_RS422_CH00. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint8_t cas_rs422_ch00_status : 1;
    #endif
} __attribute__((packed)) Channel_group_09_status_t;

/** Indicates the status of A429 Channels (ICNI). */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint32_t icni_a429_ch00_status : 1;
    uint32_t icni_a429_ch01_status : 1;
    uint32_t icni_a429_ch02_status : 1;
    uint32_t icni_a429_ch03_status : 1;
    uint32_t icni_a429_ch04_status : 1;
    uint32_t icni_a429_ch05_status : 1;
    uint32_t icni_a429_ch06_status : 1;
    uint32_t icni_a429_ch07_status : 1;
    uint32_t icni_a429_ch08_status : 1;
    uint32_t reserved_44 : 23;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint32_t reserved_44 : 23;
    /** Indicates the status of ICNI_A429_CH08. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t icni_a429_ch08_status : 1;
    /** Indicates the status of ICNI_A429_CH07. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t icni_a429_ch07_status : 1;
    /** Indicates the status of ICNI_A429_CH06. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t icni_a429_ch06_status : 1;
    /** Indicates the status of ICNI_A429_CH05. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t icni_a429_ch05_status : 1;
    /** Indicates the status of ICNI_A429_CH04. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t icni_a429_ch04_status : 1;
    /** Indicates the status of ICNI_A429_CH03. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t icni_a429_ch03_status : 1;
    /** Indicates the status of ICNI_A429_CH02. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t icni_a429_ch02_status : 1;
    /** Indicates the status of ICNI_A429_CH01. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t icni_a429_ch01_status : 1;
    /** Indicates the status of ICNI_A429_CH00. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t icni_a429_ch00_status : 1;
    #endif
} __attribute__((packed)) Channel_group_10_status_t;

/** Indicates the status of Ethernet Channels. */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint32_t ethernet_ch00_status : 1;
    uint32_t ethernet_ch01_status : 1;
    uint32_t reserved_45 : 30;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint32_t reserved_45 : 30;
    /** Indicates the status of Ethernet_CH01. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t ethernet_ch01_status : 1;
    /** Indicates the status of Ethernet_CH00. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t ethernet_ch00_status : 1;
    #endif
} __attribute__((packed)) Channel_group_11_status_t;

/** Indicates the status of M1553 Channels. */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint32_t m1553_ch00_status : 1;
    uint32_t m1553_ch01_status : 1;
    uint32_t reserved_46 : 30;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint32_t reserved_46 : 30;
    /** Indicates the status of M1553_CH01. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t m1553_ch01_status : 1;
    /** Indicates the status of M1553_CH00. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t m1553_ch00_status : 1;
    #endif
} __attribute__((packed)) Channel_group_12_status_t;

/** Indicates the status of FLASH Channels. */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint32_t io_vl_config_flash_ch00_status : 1;
    uint32_t hm_flash_ch01_status : 1;
    uint32_t reserved_47 : 30;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint32_t reserved_47 : 30;
    /** Indicates the status of HM_FLASH_CH01_STATUS. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t hm_flash_ch01_status : 1;
    /** Indicates the status of IO_VL_CONFIG_FLASH_CH00_STATUS. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t io_vl_config_flash_ch00_status : 1;
    #endif
} __attribute__((packed)) Channel_group_13_status_t;

/** Indicates the status of SNTZ_BLOCK. */
typedef struct
{
   #ifdef SYSTEM_LITTLE_ENDIAN
    uint32_t adcu_x_sntz_doos_ch02_erroneous_status : 1;
    uint32_t adcu_x_sntz_doos_ch01_erroneous_status : 1;
    uint32_t adcu_x_sntz_doos_ch00_erroneous_status : 1;
    uint32_t adcu_x_sntz_doso_ch00_erroneous_status : 1;
    uint32_t adcu_x_sntz_dogo_ch03_erroneous_status : 1;
    uint32_t adcu_x_sntz_dogo_ch02_erroneous_status : 1;
    uint32_t adcu_x_sntz_dogo_ch01_erroneous_status : 1;
    uint32_t adcu_x_sntz_dogo_ch00_erroneous_status : 1;
    uint32_t adcu_x_sntz_cas_diso_ch00_loss_status : 1;
    uint32_t adcu_x_sntz_util_digo_ch00_loss_status : 1;
    uint32_t adcu_x_sntz_ces_digo_ch02_loss_status : 1;
    uint32_t adcu_x_sntz_ces_digo_ch01_loss_status : 1;
    uint32_t adcu_x_sntz_ces_digo_ch00_loss_status : 1;
    uint32_t reserved_67 : 19;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint32_t reserved_67 : 19;
    /** Indicates the status of ADCU_X_SNTZ_CES_DIGO_CH00_LOSS. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t adcu_x_sntz_ces_digo_ch00_loss_status : 1;
    /** Indicates the status of ADCU_X_SNTZ_CES_DIGO_CH01_LOSS. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t adcu_x_sntz_ces_digo_ch01_loss_status : 1;
    /** Indicates the status of ADCU_X_SNTZ_CES_DIGO_CH02_LOSS. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t adcu_x_sntz_ces_digo_ch02_loss_status : 1;
    /** Indicates the status of ADCU_X_SNTZ_UTIL_DIGO_CH00_LOSS. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t adcu_x_sntz_util_digo_ch00_loss_status : 1;
    /** Indicates the status of ADCU_X_SNTZ_CAS_DISO_CH00_LOSS. 0: CHANNEL_OK 1: CHANNEL_LOSS */
    uint32_t adcu_x_sntz_cas_diso_ch00_loss_status : 1;
    /** Indicates the status of ADCU_X_SNTZ_DOGO_CH00_ERRONEOUS. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t adcu_x_sntz_dogo_ch00_erroneous_status : 1;
    /** Indicates the status of ADCU_X_SNTZ_DOGO_CH01_ERRONEOUS. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t adcu_x_sntz_dogo_ch01_erroneous_status : 1;
    /** Indicates the status of ADCU_X_SNTZ_DOGO_CH02_ERRONEOUS. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t adcu_x_sntz_dogo_ch02_erroneous_status : 1;
    /** Indicates the status of ADCU_X_SNTZ_DOGO_CH03_ERRONEOUS. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t adcu_x_sntz_dogo_ch03_erroneous_status : 1;
    /** Indicates the status of ADCU_X_SNTZ_DOSO_CH00_ERRONEOUS. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t adcu_x_sntz_doso_ch00_erroneous_status : 1;
    /** Indicates the status of ADCU_X_SNTZ_DOOS_CH00_ERRONEOUS. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t adcu_x_sntz_doos_ch00_erroneous_status : 1;
    /** Indicates the status of ADCU_X_SNTZ_DOOS_CH01_ERRONEOUS. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t adcu_x_sntz_doos_ch01_erroneous_status : 1;
    /** Indicates the status of ADCU_X_SNTZ_DOOS_CH02_ERRONEOUS. 0: CHANNEL_OK 1: CHANNEL_ERRONEOUS */
    uint32_t adcu_x_sntz_doos_ch02_erroneous_status : 1;
    #endif
} __attribute__((packed)) Sntz_block_status_t;

/** Indicates the status of Block Loss or Erroneous. */
typedef struct
{
   #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t adcu_x_wds_block_loss_status : 1;
    uint8_t adcu_x_irfs_block_loss_status : 1;
    uint8_t adcu_x_ieos_block_loss_status : 1;
    uint8_t adcu_x_icni_block_loss_status : 1;
    uint8_t adcu_x_cas_block_loss_status : 1;
    uint8_t adcu_x_do_block_erroneous_status : 1;
    uint8_t reserved_65 : 2;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint8_t reserved_65 : 2;
    /** Indicates the status of ADCU_X_DO_BLOCK. 0: BLOCK_OK 1: BLOCK_ERRONEOUS */
    uint8_t adcu_x_do_block_erroneous_status : 1;
    /** Indicates the status of ADCU_X_CAS_BLOCK. 0: BLOCK_OK 1: BLOCK_LOSS */
    uint8_t adcu_x_cas_block_loss_status : 1;
    /** Indicates the status of ADCU_X_ICNI_BLOCK. 0: BLOCK_OK 1: BLOCK_LOSS */
    uint8_t adcu_x_icni_block_loss_status : 1;
    /** Indicates the status of ADCU_X_IEOS_BLOCK. 0: BLOCK_OK 1: BLOCK_LOSS */
    uint8_t adcu_x_ieos_block_loss_status : 1;
    /** Indicates the status of ADCU_X_IRFS_BLOCK. 0: BLOCK_OK 1: BLOCK_LOSS */
    uint8_t adcu_x_irfs_block_loss_status : 1;
    /** Indicates the status of ADCU_X_WDS_BLOCK. 0: BLOCK_OK 1: BLOCK_LOSS */
    uint8_t adcu_x_wds_block_loss_status : 1;
    #endif
} __attribute__((packed)) Block_status_t;

/** Indicates the status of BLOCK_VOLTAGE_HM_DATA. */
typedef struct
{
   #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t adcu_x_sntz_voltage_hm_data_loss_status : 1;
    uint8_t adcu_x_wds_voltage_hm_data_loss_status : 1;
    uint8_t adcu_x_irfs_voltage_hm_data_loss_status : 1;
    uint8_t adcu_x_ieos_voltage_hm_data_loss_status : 1;
    uint8_t adcu_x_icni_voltage_hm_data_loss_status : 1;
    uint8_t adcu_x_cas_voltage_hm_data_loss_status : 1;
    uint8_t reserved_66 : 2;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint8_t reserved_66 : 2;
    /** Indicates the status of ADCU_X_CAS_VOLTAGE_HM_DATA_LOSS. 0: BLOCK_VOLTAGE_OK 1: BLOCK_VOLTAGE_LOSS */
    uint8_t adcu_x_cas_voltage_hm_data_loss_status : 1;
    /** Indicates the status of ADCU_X_ICNI_VOLTAGE_HM_DATA_LOSS. 0: BLOCK_VOLTAGE_OK 1: BLOCK_VOLTAGE_LOSS */
    uint8_t adcu_x_icni_voltage_hm_data_loss_status : 1;
    /** Indicates the status of ADCU_X_IEOS_VOLTAGE_HM_DATA_LOSS. 0: BLOCK_VOLTAGE_OK 1: BLOCK_VOLTAGE_LOSS */
    uint8_t adcu_x_ieos_voltage_hm_data_loss_status : 1;
    /** Indicates the status of ADCU_X_IRFS_VOLTAGE_HM_DATA_LOSS. 0: BLOCK_VOLTAGE_OK 1: BLOCK_VOLTAGE_LOSS */
    uint8_t adcu_x_irfs_voltage_hm_data_loss_status : 1;
    /** Indicates the status of ADCU_X_WDS_VOLTAGE_HM_DATA_LOSS. 0: BLOCK_VOLTAGE_OK 1: BLOCK_VOLTAGE_LOSS */
    uint8_t adcu_x_wds_voltage_hm_data_loss_status : 1;
    /** Indicates the status of ADCU_X_SNTZ_VOLTAGE_HM_DATA_LOSS. 0: BLOCK_VOLTAGE_OK 1: BLOCK_VOLTAGE_LOSS */
    uint8_t adcu_x_sntz_voltage_hm_data_loss_status : 1;
    #endif
} __attribute__((packed)) Block_voltage_hm_data_status_t;

/** Indicates DCU Channel 0x0100 Ethernet Status. */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t link_status : 1;
    uint8_t ethernet_speed : 1;
    uint8_t ethernet_mode : 1;
    uint8_t identifier_error : 1;
    uint8_t reserved_x : 4;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint8_t reserved_x : 4;
    /** Indicates Error on Ethernet. Applicable when DCU_STATUS is LRU_ALIVE. 0: NO_ERROR 1: ERROR */
    uint8_t identifier_error : 1;
    /** Indicates Ethernet Mode of DCU Channel. Applicable when DCU_STATUS is LRU_ALIVE. 0: HALF_DUPLEX 1: FULL_DUPLEX */
    uint8_t ethernet_mode : 1;
    /** Indicates Ethernet Speed of DCU Channel. Applicable when DCU_STATUS is LRU_ALIVE. 0: 10Mbit 1: 100Mbit */
    uint8_t ethernet_speed : 1;
    /** Indicates Link Status of DCU Channel. Applicable when DCU_STATUS is LRU_ALIVE. 0: GOOD_LINK 1: BAD_LINK */
    uint8_t link_status : 1;
    #endif
} __attribute__((packed)) Dcu_ethernet_status_ch_0_x0100_t;

/** Indicates DCU Channel 0x0101 Ethernet Status. */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t link_status : 1;
    uint8_t ethernet_speed : 1;
    uint8_t ethernet_mode : 1;
    uint8_t identifier_error : 1;
    uint8_t reserved_x : 4;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint8_t reserved_x : 4;
    /** Indicates Error on Ethernet. Applicable when DCU_STATUS is LRU_ALIVE. 0: NO_ERROR 1: ERROR */
    uint8_t identifier_error : 1;
    /** Indicates Ethernet Mode of DCU Channel. Applicable when DCU_STATUS is LRU_ALIVE. 0: HALF_DUPLEX 1: FULL_DUPLEX */
    uint8_t ethernet_mode : 1;
    /** Indicates Ethernet Speed of DCU Channel. Applicable when DCU_STATUS is LRU_ALIVE. 0: 10Mbit 1: 100Mbit */
    uint8_t ethernet_speed : 1;
    /** Indicates Link Status of DCU Channel. Applicable when DCU_STATUS is LRU_ALIVE. 0: GOOD_LINK 1: BAD_LINK */
    uint8_t link_status : 1;
    #endif
} __attribute__((packed)) Dcu_ethernet_status_ch_0_x0101_t;

/** Indicates ADCU Ethernet Status. */
typedef struct
{
    /** Indicates DCU Channel 0x0100 Ethernet Status */
    Dcu_ethernet_status_ch_0_x0100_t dcu_ethernet_status_ch_0_x0100;
    /** Indicates DCU Channel 0x0101 Ethernet Status */
    Dcu_ethernet_status_ch_0_x0101_t dcu_ethernet_status_ch_0_x0101;
} __attribute__((packed)) Dcu_ethernet_status_t;

/** Indicates the status of an ADCU defined in the MMP Level CMSW configuration. */
typedef struct
{
    /** Uniquely identifies the DCU within the whole IPP. */
    uint8_t dcu_lru_id : 8;
    /** DCU sends CBIT messages periodically. This field tests whether DCU is alive or not based on received status messages. LRU_ALIVE: CBIT message received within timeout time in the MMP Level CMSW configuration data. LRU_LOSS: CBIT message not received within timeout time in the MMP Level CMSW configuration data. Remark: MMP level CMSW reports Current Platform Configuration ID in the MMP Status Report Message periodically. */
    uint8_t dcu_status : 8;
    /** Indicates the status of Hold Up. */
    uint8_t hold_up_status : 8;
    /** Indicates the status of Voltage Health Monitoring Data. */
    uint8_t voltage_hm_data_status : 8;
    /** Indicates the Config Status of DCU */
    Dcu_config_status_t dcu_config_status;
    /** Indicates the DCU Custom Design Version Major part. Major change is one that affects the requirements, external interfaces, cost, and/or schedule. */
    uint8_t dcu_custom_design_version_major : 8;
    /** Indicates the DCU Custom Design Version Minor part. Minor changes are those that do not affect form, fit, function, cost, or schedule. */
    uint8_t dcu_custom_design_version_minor : 8;
    /** Indicates the DCU Custom Design Version Bugfix part. */
    uint8_t dcu_custom_design_version_bugfix : 8;
    /** Indicates data validity of following DTN and PTP related fields. */
    uint8_t dcu_dtn_es_data_validity : 8;
    /** Indicates whether DTN End System configuration ID reported by the DCU matches with expected DTN End System Configuration ID in the MMP Level CMSW configuration data or not. */
    uint8_t dtn_es_config_status : 8;
    /** Indicates whether DCU DTN ES A and B link is up or not on the network. DCU_DTN_ES_REDUNDANCY_OK: DTN A and B links are active DCU_DTN_ES_REDUNDANCY_FAIL: When DTN A or B link is down. */
    uint8_t dcu_dtn_redundancy_status : 8;
    /** Indicates whether PTP configuration ID reported by the DCU matches with expected PTP Configuration ID in the MMP Level CMSW configuration data or not. */
    uint8_t ptp_config_status : 8;
    /** NOT_SYNCHED: DCU is not receiving synch messages or PTP response messages from PTP Master or PTP Grand Master. SYNCHED: DCU is receiving both PTP synch messages and PTP response messages from PTP Master or PTP Grand Master. */
    uint8_t ptp_sync_status : 8;
    /** Indicates the status of TEMP_DATA */
    uint8_t temp_data_status : 8;
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t m1553_watchdog_status_ch_0x111 : 2;
    uint8_t m1553_watchdog_status_ch_0x110 : 2;
    uint8_t reserved_yz : 4;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint8_t reserved_yz : 4;
    /** Indicates M1553 bus status for channel 0x110. 0: NOT_FAULTY, 1:FAULTY */
    uint8_t m1553_watchdog_status_ch_0x110 : 2;
    /** Indicates M1553 bus status for channel 0x111. 0: NOT_FAULTY, 1:FAULTY */
    uint8_t m1553_watchdog_status_ch_0x111 : 2;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t m1553_bus_type_ch_0x111 : 4;
    uint8_t m1553_bus_type_ch_0x110 : 4;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates M1553 Bus Type Channel 0x110 0: NONE 1: BC_MODE 2: BM_MODE */
    uint8_t m1553_bus_type_ch_0x110 : 4;
    /** Indicates M1553 Bus Type Channel 0x111 0: NONE 1: BC_MODE 2: BM_MODE */
    uint8_t m1553_bus_type_ch_0x111 : 4;
    #endif
    /** Indicates the status of DOGO Channels */
    Channel_group_00_status_t channel_group_00_status;
    /** Indicates the status of DOGO Channels */
    Channel_group_01_status_t channel_group_01_status;
    /** Indicates the status of DOOS Channels */
    Channel_group_02_status_t channel_group_02_status;
    /** Indicates the status of DOSO Channels */
    Channel_group_03_status_t channel_group_03_status;
    /** Indicates the status of  DIGO Channels (CAS, ICNI) */
    Channel_group_04_status_t channel_group_04_status;
    /** Indicates the status of DIGO Channels (CAS) */
    Channel_group_05_status_t channel_group_05_status;
    /** Indicates the status of DIGO Channels (CAS, IEOS) */
    Channel_group_06_status_t channel_group_06_status;
    /** Indicates the status of DISO Channels (WDS, IRFS, ICNI, IEOS, CAS) */
    Channel_group_07_status_t channel_group_07_status;
    /** Indicates the status of AVI Channels (CAS) */
    Channel_group_08_status_t channel_group_08_status;
    /** Indicates the status of RS422 Channels (IEOS, ICNI, CAS) */
    Channel_group_09_status_t channel_group_09_status;
    /** Indicates the status of A429 Channels (ICNI) */
    Channel_group_10_status_t channel_group_10_status;
    /** Indicates the status of Ethernet Channels */
    Channel_group_11_status_t channel_group_11_status;
    /** Indicates the status of M1553 Channels */
    Channel_group_12_status_t channel_group_12_status;
    /** Indicates the status of FLASH Channels */
    Channel_group_13_status_t channel_group_13_status;
    /** Indicates the status of SNTZ_BLOCK */
    Sntz_block_status_t sntz_block_status;
    /** Indicates the status of Block Loss or Erroneous */
    Block_status_t block_status;
    /** Indicates the status of BLOCK_VOLTAGE_HM_DATA */
    Block_voltage_hm_data_status_t block_voltage_hm_data_status;
    /** Indicates ADCU Ethernet Status */
    Dcu_ethernet_status_t dcu_ethernet_status;
} __attribute__((packed)) Avionics_dcu_status_list_t;

/** DTN Interrack Switch Port link status */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint32_t port_32_link_status : 1;
    uint32_t port_33_link_status : 1;
    uint32_t reserved_50 : 30;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint32_t reserved_50 : 30;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint32_t port_33_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint32_t port_32_link_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t port_24_link_status : 1;
    uint8_t port_25_link_status : 1;
    uint8_t port_26_link_status : 1;
    uint8_t port_27_link_status : 1;
    uint8_t port_28_link_status : 1;
    uint8_t port_29_link_status : 1;
    uint8_t port_30_link_status : 1;
    uint8_t port_31_link_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_31_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_30_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_29_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_28_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_27_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_26_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_25_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_24_link_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t port_16_link_status : 1;
    uint8_t port_17_link_status : 1;
    uint8_t port_18_link_status : 1;
    uint8_t port_19_link_status : 1;
    uint8_t port_20_link_status : 1;
    uint8_t port_21_link_status : 1;
    uint8_t port_22_link_status : 1;
    uint8_t port_23_link_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_23_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_22_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_21_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_20_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_19_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_18_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_17_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_16_link_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t port_8_link_status : 1;
    uint8_t port_9_link_status : 1;
    uint8_t port_10_link_status : 1;
    uint8_t port_11_link_status : 1;
    uint8_t port_12_link_status : 1;
    uint8_t port_13_link_status : 1;
    uint8_t port_14_link_status : 1;
    uint8_t port_15_link_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_15_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_14_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_13_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_12_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_11_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_10_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_9_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_8_link_status : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t port_0_link_status : 1;
    uint8_t port_1_link_status : 1;
    uint8_t port_2_link_status : 1;
    uint8_t port_3_link_status : 1;
    uint8_t port_4_link_status : 1;
    uint8_t port_5_link_status : 1;
    uint8_t port_6_link_status : 1;
    uint8_t port_7_link_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_7_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_6_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_5_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_4_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_3_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_2_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_1_link_status : 1;
    /** DTN IR SW Port Status 0: PORT_LINK_GOOD 1: PORT_LINK_LOSS */
    uint8_t port_0_link_status : 1;
    #endif
} __attribute__((packed)) Dtn_ir_sw_port_link_status_t;

/** Indicates the status of DTN_IR_SW_FPGA */
typedef struct
{
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t fpga_0_status : 1;
    uint8_t fpga_1_status : 1;
    uint8_t reserved_51 : 6;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint8_t reserved_51 : 6;
    /** Indicates the status of FPGA_1. 0: FPGA_ALIVE 1: FPGA_LOSS */
    uint8_t fpga_1_status : 1;
    /** Indicates the status of FPGA_0. 0: FPGA_ALIVE 1: FPGA_LOSS */
    uint8_t fpga_0_status : 1;
    #endif
} __attribute__((packed)) Dtn_ir_sw_fpga_status_t;

/** Indicates the status of an DTN IRS defined in the MMP Level CMSW configuration. */
typedef struct
{
    /** Uniquely identifies the DTN Inter-rack SW within the whole IPP. */
    uint8_t lru_id : 8;
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t reserved_85      : 7;
    uint8_t dtn_ir_sw_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** DTN Inter-rack Switch sends status messages periodically. This field tests whether DTN Inter-rack Switch is alive or not based on received status messages. LRU_ALIVE: Status message received within timeout time in the MMP Level CMSW configuration data. LRU_LOSS:Status message not received within timeout time in the MMP Level CMSW configuration data. Remark: MMP level CMSW reports Current Platform Configuration ID in the MMP Status Report Message periodically. 0: LRU_ALIVE 1: LRU_LOSS */
    uint8_t dtn_ir_sw_status : 1;
    uint8_t reserved_85      : 7;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t dtn_ir_sw_dtn_es_data_validity : 1;
    uint8_t dtn_ir_sw_assistant_group2_data_validity : 1;
    uint8_t dtn_ir_sw_assistant_group1_data_validity : 1;
    uint8_t dtn_ir_sw_mcu_data_validity : 1;
    uint8_t dtn_ir_sw_manager_group3_data_validity : 1;
    uint8_t dtn_ir_sw_manager_group2_data_validity : 1;
    uint8_t dtn_ir_sw_manager_group1_data_validity : 1;
    uint8_t reserved_81 : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    uint8_t reserved_81      : 1;
    /** Indicates data validity of some of the following fields. Fields affected by this data validity reference this object by it's object text in its' definition. 0: VALID 1: INVALID */
    uint8_t dtn_ir_sw_manager_group1_data_validity : 1;
    /** Indicates data validity of some of the following fields. Fields affected by this data validity reference this object by it's object text in its' definition. 0: VALID 1: INVALID */
    uint8_t dtn_ir_sw_manager_group2_data_validity : 1;
    /** Indicates data validity of some of the following fields. Fields affected by this data validity reference this object by it's object text in its' definition. 0: VALID 1: INVALID */
    uint8_t dtn_ir_sw_manager_group3_data_validity : 1;
    /** Indicates data validity of some of the following fields. Fields affected by this data validity reference this object by it's object text in its' definition. 0: VALID 1: INVALID */
    uint8_t dtn_ir_sw_mcu_data_validity : 1;
    /** Indicates data validity of some of the following fields. Fields affected by this data validity reference this object by it's object text in its' definition. 0: VALID 1: INVALID */
    uint8_t dtn_ir_sw_assistant_group1_data_validity : 1;
    /** Indicates data validity of some of the following fields. Fields affected by this data validity reference this object by it's object text in its' definition. 0: VALID 1: INVALID */
    uint8_t dtn_ir_sw_assistant_group2_data_validity : 1;
    /** Indicates data validity of some of the following fields. Fields affected by this data validity reference this object by it's object text in its' definition. 0: VALID 1: INVALID */
    uint8_t dtn_ir_sw_dtn_es_data_validity : 1;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t reserved_49                 : 4;
    uint8_t dtn_ir_sw_cd_version_status : 1;
    uint8_t ptp_config_status : 1;
    uint8_t ptp_sync_status : 1;
    uint8_t dtn_ir_sw_config_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** CONFIG_MATCH: DTN Switch Configuration ID reported by this module matches with DTN Switch Configuration ID for current platform configuration in the configuration data. CONFIG_MISMATCH: DTN Switch Configuration ID reported by this module does not match with DTN Switch Configuration ID for current platform configuration in the configuration data. 0: CONFIG_MATCH 1: CONFIG_MISMATCH */
    uint8_t dtn_ir_sw_config_status : 1;
    /** SYNCHED: PTP Synhcronisation is ok for this component. NOT_SYNCHED: PTP Synhcronisation is failed for this component. 0: SYNCHED 1: NOT_SYNCHED */
    uint8_t ptp_sync_status : 1;
    /** GOOD: PTP Configuration ID reported by this module matches with PTP Configuration ID for this module in current platform configuration in the MMP Level CMSW Configuration Data. BAD: PTP Configuration ID reported by this module does not match with DTN ES Configuration ID for this module in current platform configuration in the MMP Level CMSW Configuration Data. 0: GOOD 1: BAD */
    uint8_t ptp_config_status : 1;
    /** There are two FPGAs in the DTN Inter-Rack Switch. This field is used to verify both FPGAs are initialized with the same custom design version. VERSION_MATCH: Both FPGAs in the DTN Inter-Rack Switch report the same version number. VERSION_MISMATCH: FPGAs in the DTN Inter-Rack Switch report different version number. 0: VERSION_MATCH 1: VERSION_MISMATCH */
    uint8_t dtn_ir_sw_cd_version_status : 1;
    uint8_t reserved_49                 : 4;
    #endif
    /** DTN Interrack Switch Port link status */
    Dtn_ir_sw_port_link_status_t dtn_ir_sw_port_link_status;
    /** Indicates the status of DTN_IR_SW_FPGA */
    Dtn_ir_sw_fpga_status_t dtn_ir_sw_fpga_status;
    /** Indicates DTN IR Switch Custom Design Version Major part. Major change is one that affects the requirements, external interfaces, cost, and/or schedule. */
    uint16_t dtn_ir_sw_custom_design_version_major : 16;
    /** Indicates DTN IR Switch Custom Design Version Minor part. Minor changes are those that do not affect form, fit, function, cost, or schedule. */
    uint16_t dtn_ir_sw_custom_design_version_minor : 16;
    /** Indicates DTN IR Custom Design Version Bugfix part. */
    uint16_t dtn_ir_sw_custom_design_version_bugfix : 16;
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t reserved_86                       : 7;
    uint8_t dtn_ir_sw_connection_csfdr_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the connnection status of CSFDR. 0: OK 1: LOSS */
    uint8_t dtn_ir_sw_connection_csfdr_status : 1;
    uint8_t reserved_86                       : 7;
    #endif
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t mcu_firmware_version_minor : 4;
    uint8_t mcu_firmware_version_major : 4;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates MCU Firmware Design  Version Major part. Major change is one that affects the requirements, external interfaces, cost, and/or schedule. */
    uint8_t mcu_firmware_version_major : 4;
    /** Indicates MCU Firmware Design  Version Minor part. Minor changes are those that do not affect form, fit, function, cost, or schedule. */
    uint8_t mcu_firmware_version_minor : 4;
    #endif
    /** Indicates MCU Firmware Design Version Bugfix part. */
    uint8_t mcu_firmware_version_bugfix : 8;
    #ifdef SYSTEM_LITTLE_ENDIAN
    uint8_t reserved_87                  : 7;
    uint8_t dtn_ir_sw_mcu_hm_data_status : 1;
    #elif defined(SYSTEM_BIG_ENDIAN)
    /** Indicates the status of DTN Interrack Switch MCU Health Monitoring Data. 0: HM_DATA_AVAILABLE 1: HM_DATA_LOSS */
    uint8_t dtn_ir_sw_mcu_hm_data_status : 1;
    uint8_t reserved_87                  : 7;
    #endif
} __attribute__((packed)) Dtn_ir_sw_status_list_t;

/** MMPL CMSW Status Report Message that periodically sent Hosted Applciation */
typedef struct
{
    /** Message type identifier 62: MMP_STATUS_REPORT */
    uint8_t  mmp_status_report : 8;
    uint16_t reserved_1        : 16;
    /** Timestamp of when the message is generated. */
    uint64_t timestamp : 64;
    /** Status of Master MMP Level CMSW */
    Master_status_t master_status;
    /** Status of Backup MMP Level CMSW */
    Backup_status_t backup_status;
    /** Behaviour of MMP Level CMSW changes depending on the MMP State. 0: INITIALIZATION 1: OPERATIONAL 2: RECONFIGURATION */
    uint8_t mmp_state : 8;
    /** Identifier of current platform configuration. */
    uint8_t current_platform_config_id : 8;
    uint8_t reserved_4                 : 8;
    /** List of computer status in the platform */
    Computer_status_list_t computer_status_list[2];
    uint8_t                reserved_33 : 8;
    /** An array of Avionics DCU Status */
    Avionics_dcu_status_list_t avionics_dcu_status_list[2];
    uint8_t                    reserved_48 : 8;
    /** List of DTN Inter-rack Switch Status */
    Dtn_ir_sw_status_list_t dtn_ir_sw_status_list[2];
} __attribute__((packed)) Mmplcmsw_tai_icd_status_t;

#endif /* MMPLCMSW_TAI_EXT_INTERFACE_H */

static int mmpl_cmsw_mmp_status_endiannes_converter(Mmplcmsw_tai_icd_status_t *mmplcmsw_tai_icd_status) {
    mmplcmsw_tai_icd_status->timestamp = ntohll(mmplcmsw_tai_icd_status->timestamp);
    // Computer_status_list_t computer_status_list[2];
    for (int i = 0; i < 2; ++i) {
        // Dpm_status_list_t dpm_status_list[5];
        for (int j = 0; j < 5; ++j) {
            mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].generic_ipmc_status.lrm_serial_id = ntohll(mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].generic_ipmc_status.lrm_serial_id);
            mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].generic_ipmc_status.lrm_pcb_revision_number = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].generic_ipmc_status.lrm_pcb_revision_number);
            mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].generic_ipmc_status.lrm_pcb_assembly_id = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].generic_ipmc_status.lrm_pcb_assembly_id);
            mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].generic_ipmc_status.lrm_mechanical_revision_number = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].generic_ipmc_status.lrm_mechanical_revision_number);
        }
        // Psm_status_list_t psm_status_list[2];
        for (int j = 0; j < 2; ++j) {
            mmplcmsw_tai_icd_status->computer_status_list[i].psm_status_list[j].generic_ipmc_status.lrm_serial_id = ntohll(mmplcmsw_tai_icd_status->computer_status_list[i].psm_status_list[j].generic_ipmc_status.lrm_serial_id);
            mmplcmsw_tai_icd_status->computer_status_list[i].psm_status_list[j].generic_ipmc_status.lrm_pcb_revision_number = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].psm_status_list[j].generic_ipmc_status.lrm_pcb_revision_number);
            mmplcmsw_tai_icd_status->computer_status_list[i].psm_status_list[j].generic_ipmc_status.lrm_pcb_assembly_id = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].psm_status_list[j].generic_ipmc_status.lrm_pcb_assembly_id);
            mmplcmsw_tai_icd_status->computer_status_list[i].psm_status_list[j].generic_ipmc_status.lrm_mechanical_revision_number = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].psm_status_list[j].generic_ipmc_status.lrm_mechanical_revision_number);
        }
        // Smmm_status_t smmm_status;
        mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.generic_ipmc_status.lrm_serial_id = ntohll(mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.generic_ipmc_status.lrm_serial_id);
        mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.generic_ipmc_status.lrm_pcb_revision_number = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.generic_ipmc_status.lrm_pcb_revision_number);
        mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.generic_ipmc_status.lrm_pcb_assembly_id = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.generic_ipmc_status.lrm_pcb_assembly_id);
        mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.generic_ipmc_status.lrm_mechanical_revision_number = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.generic_ipmc_status.lrm_mechanical_revision_number);
        mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.fpga_code_version = ntohs(mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.fpga_code_version);
        mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.security_cntrlr_mcu_fw_version = ntohs(mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.security_cntrlr_mcu_fw_version);
        // Gpm_status_list_t gpm_status_list[2];
        for (int j = 0; j < 2; ++j) {
            mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].generic_ipmc_status.lrm_serial_id = ntohll(mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].generic_ipmc_status.lrm_serial_id);
            mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].generic_ipmc_status.lrm_pcb_revision_number = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].generic_ipmc_status.lrm_pcb_revision_number);
            mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].generic_ipmc_status.lrm_pcb_assembly_id = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].generic_ipmc_status.lrm_pcb_assembly_id);
            mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].generic_ipmc_status.lrm_mechanical_revision_number = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].generic_ipmc_status.lrm_mechanical_revision_number);
        }
        // Hsm_status_list_t hsm_status_list[2];
        for (int j = 0; j < 2; ++j) {
            mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].generic_ipmc_status.lrm_serial_id = ntohll(mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].generic_ipmc_status.lrm_serial_id);
            mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].generic_ipmc_status.lrm_pcb_revision_number = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].generic_ipmc_status.lrm_pcb_revision_number);
            mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].generic_ipmc_status.lrm_pcb_assembly_id = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].generic_ipmc_status.lrm_pcb_assembly_id);
            mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].generic_ipmc_status.lrm_mechanical_revision_number = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].generic_ipmc_status.lrm_mechanical_revision_number);
        }
        // Dsm_status_list_t dsm_status_list[2];
        for (int j = 0; j < 2; ++j) {
            mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].generic_ipmc_status.lrm_serial_id = ntohll(mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].generic_ipmc_status.lrm_serial_id);
            mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].generic_ipmc_status.lrm_pcb_revision_number = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].generic_ipmc_status.lrm_pcb_revision_number);
            mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].generic_ipmc_status.lrm_pcb_assembly_id = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].generic_ipmc_status.lrm_pcb_assembly_id);
            mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].generic_ipmc_status.lrm_mechanical_revision_number = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].generic_ipmc_status.lrm_mechanical_revision_number);
        }
        // Iocm_status_t iocm_status;
        mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.generic_ipmc_status.lrm_serial_id = ntohll(mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.generic_ipmc_status.lrm_serial_id);
        mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.generic_ipmc_status.lrm_pcb_revision_number = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.generic_ipmc_status.lrm_pcb_revision_number);
        mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.generic_ipmc_status.lrm_pcb_assembly_id = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.generic_ipmc_status.lrm_pcb_assembly_id);
        mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.generic_ipmc_status.lrm_mechanical_revision_number = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.generic_ipmc_status.lrm_mechanical_revision_number);
        // Hum_status_list_t hum_status_list[2];
        for (int j = 0; j < 2; ++j) {
            mmplcmsw_tai_icd_status->computer_status_list[i].hum_status_list[j].generic_ipmc_status.lrm_serial_id = ntohll(mmplcmsw_tai_icd_status->computer_status_list[i].hum_status_list[j].generic_ipmc_status.lrm_serial_id);
            mmplcmsw_tai_icd_status->computer_status_list[i].hum_status_list[j].generic_ipmc_status.lrm_pcb_revision_number = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].hum_status_list[j].generic_ipmc_status.lrm_pcb_revision_number);
            mmplcmsw_tai_icd_status->computer_status_list[i].hum_status_list[j].generic_ipmc_status.lrm_pcb_assembly_id = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].hum_status_list[j].generic_ipmc_status.lrm_pcb_assembly_id);
            mmplcmsw_tai_icd_status->computer_status_list[i].hum_status_list[j].generic_ipmc_status.lrm_mechanical_revision_number = ntohl(mmplcmsw_tai_icd_status->computer_status_list[i].hum_status_list[j].generic_ipmc_status.lrm_mechanical_revision_number);
        }
    }
    // Dtn_ir_sw_status_list_t dtn_ir_sw_status_list[2];
    for (int i = 0; i < 2; ++i) {
        mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[i].dtn_ir_sw_custom_design_version_major = ntohs(mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[i].dtn_ir_sw_custom_design_version_major);
        mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[i].dtn_ir_sw_custom_design_version_minor = ntohs(mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[i].dtn_ir_sw_custom_design_version_minor);
        mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[i].dtn_ir_sw_custom_design_version_bugfix = ntohs(mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[i].dtn_ir_sw_custom_design_version_bugfix);
    }
}

static int mmpl_cmsw_mmp_status_header_printer(FILE *fp) {

    fprintf(fp, "DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP,LOG PAYLOAD LENGTH,");
    
    fprintf(fp, "mmp_status_report,");
    fprintf(fp, "timestamp,");

    // Master_status_t master_status
    fprintf(fp, "master_cmc_id,");
    fprintf(fp, "master_backplane_slot_index,");
    fprintf(fp, "master_hb_status,");

    // Backup_status_t backup_status;
    fprintf(fp, "backup_lru_id,");
    fprintf(fp, "backup_backplane_slot_index,");
    fprintf(fp, "backup_hb_status,");

    fprintf(fp, "mmp_state,");
    fprintf(fp, "current_platform_config_id,");

    // Computer_status_list_t computer_status_list[2];
    for (int i = 0; i < 2; ++i) {
        fprintf(fp, "computer_status_list[%d].computer_lru_id,", i);
        fprintf(fp, "computer_status_list[%d].computer_status,", i);
        fprintf(fp, "computer_status_list[%d].computer_chassis_manager_validity,", i);

        // Dpm_status_list_t dpm_status_list[5];
        for (int j = 0; j < 5; ++j) {
            // Slot_identifier_t slot_idx;
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].slot_idx.lrm_backplane_slot_index,", i, j);

            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].module_status,", i, j);

            //Generic_ipmc_status_t generic_ipmc_status;
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].generic_ipmc_status.ipmc_fw_version_major,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].generic_ipmc_status.ipmc_fw_version_minor,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].generic_ipmc_status.ipmc_fw_version_patch,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].generic_ipmc_status.lrm_serial_id,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].generic_ipmc_status.lrm_pcb_revision_number,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].generic_ipmc_status.lrm_pcb_assembly_id,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].generic_ipmc_status.lrm_mechanical_revision_number,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].generic_ipmc_status.operating_mode,", i, j);

            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].schedule_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].major_frame_alignment_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].temperature_data_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].voltage_data_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].dtn_es_data_validity,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].dtn_es_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].dtn_es_redundancy_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].ptp_sync_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].ptp_config_status,", i, j);

            // Dtn_es_custom_design_version_t dtn_es_custom_design_version;
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].dtn_es_custom_design_version.dtn_es_custom_design_version_major,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].dtn_es_custom_design_version.dtn_es_custom_design_version_minor,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].dtn_es_custom_design_version.dtn_es_custom_design_version_bugfix,", i, j);

            // Mmplcmsw_t2080_dpm_bootloader_pbit_t t2080_dpm_bootloader_pbit;
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].t2080_dpm_bootloader_pbit.ddr_test,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].t2080_dpm_bootloader_pbit.ifc_nand_test,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].t2080_dpm_bootloader_pbit.serdes1_pll1_test,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].t2080_dpm_bootloader_pbit.serdes2_pll1_test,", i, j);
            fprintf(fp, "computer_status_list[%d].dpm_status_list[%d].t2080_dpm_bootloader_pbit.serdes2_pll2_test,", i, j);
        }

        // Psm_status_list_t psm_status_list[2];
        for (int j = 0; j < 2; ++j) {
            // Slot_identifier_t slot_idx;
            fprintf(fp, "computer_status_list[%d].psm_status_list[%d].slot_idx.lrm_backplane_slot_index,", i, j);

            fprintf(fp, "computer_status_list[%d].psm_status_list[%d].module_status,", i, j);

            // Generic_ipmc_status_t generic_ipmc_status;
            fprintf(fp, "computer_status_list[%d].psm_status_list[%d].generic_ipmc_status.ipmc_fw_version_major,", i, j);
            fprintf(fp, "computer_status_list[%d].psm_status_list[%d].generic_ipmc_status.ipmc_fw_version_minor,", i, j);
            fprintf(fp, "computer_status_list[%d].psm_status_list[%d].generic_ipmc_status.ipmc_fw_version_patch,", i, j);
            fprintf(fp, "computer_status_list[%d].psm_status_list[%d].generic_ipmc_status.lrm_serial_id,", i, j);
            fprintf(fp, "computer_status_list[%d].psm_status_list[%d].generic_ipmc_status.lrm_pcb_revision_number,", i, j);
            fprintf(fp, "computer_status_list[%d].psm_status_list[%d].generic_ipmc_status.lrm_pcb_assembly_id,", i, j);
            fprintf(fp, "computer_status_list[%d].psm_status_list[%d].generic_ipmc_status.lrm_mechanical_revision_number,", i, j);
            fprintf(fp, "computer_status_list[%d].psm_status_list[%d].generic_ipmc_status.operating_mode,", i, j);
        }

        // Smmm_status_t smmm_status;
        // Slot_identifier_t slot_idx;
        fprintf(fp, "computer_status_list[%d].smmm_status.slot_idx.lrm_backplane_slot_index,", i);

        fprintf(fp, "computer_status_list[%d].smmm_status.module_status,", i);

        // Generic_ipmc_status_t generic_ipmc_status;
        fprintf(fp, "computer_status_list[%d].smmm_status.generic_ipmc_status.ipmc_fw_version_major,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.generic_ipmc_status.ipmc_fw_version_minor,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.generic_ipmc_status.ipmc_fw_version_patch,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.generic_ipmc_status.lrm_serial_id,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.generic_ipmc_status.lrm_pcb_revision_number,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.generic_ipmc_status.lrm_pcb_assembly_id,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.generic_ipmc_status.lrm_mechanical_revision_number,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.generic_ipmc_status.operating_mode,", i);

        fprintf(fp, "computer_status_list[%d].smmm_status.voltage_data_status,", i);

        // Interface_cbit_result_t interface_cbit_result;
        fprintf(fp, "computer_status_list[%d].smmm_status.interface_cbit_result.trng_cbit_result,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.interface_cbit_result.key_cbit_result,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.interface_cbit_result.algorithm_cbit_result,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.interface_cbit_result.fo_intf_cbit_result,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.interface_cbit_result.uart_sm_key_storage_cbit_result,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.interface_cbit_result.uart_sm_mngr_cbit_result,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.interface_cbit_result.dtn_1_cbit_result,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.interface_cbit_result.dtn_0_cbit_result,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.interface_cbit_result.pcie_mass_storage_cbit_result,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.interface_cbit_result.pcie_backplane_cbit_result,", i);

        // Alarm_status_t alarm_status;
        fprintf(fp, "computer_status_list[%d].smmm_status.alarm_status.crypto_algorithm_fail_alarm_status,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.alarm_status.crypto_key_integrity_fail_alarm_status,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.alarm_status.power_supply_high_voltage_alarm_status,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.alarm_status.power_supply_low_voltage_alarm_status,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.alarm_status.module_high_temp_alarm_status,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.alarm_status.battery_voltage_low_alarm,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.alarm_status.external_tamper_detection_alarm_status,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.alarm_status.internal_tamper_detection_alarm_status,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.alarm_status.emergency_zeroization_alarm_status,", i);

        fprintf(fp, "computer_status_list[%d].smmm_status.fpga_code_version,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.security_cntrlr_mcu_fw_version,", i);
        fprintf(fp, "computer_status_list[%d].smmm_status.smmm_fo_status,", i);

        // Gpm_status_list_t gpm_status_list[2];
        for (int j = 0; j < 2; ++j) {
            // Slot_identifier_t slot_idx;
            fprintf(fp, "computer_status_list[%d].gpm_status_list[%d].slot_idx.lrm_backplane_slot_index,", i, j);

            fprintf(fp, "computer_status_list[%d].gpm_status_list[%d].module_status,", i, j);

            // Generic_ipmc_status_t generic_ipmc_status;
            fprintf(fp, "computer_status_list[%d].gpm_status_list[%d].generic_ipmc_status.ipmc_fw_version_major,", i, j);
            fprintf(fp, "computer_status_list[%d].gpm_status_list[%d].generic_ipmc_status.ipmc_fw_version_minor,", i, j);
            fprintf(fp, "computer_status_list[%d].gpm_status_list[%d].generic_ipmc_status.ipmc_fw_version_patch,", i, j);
            fprintf(fp, "computer_status_list[%d].gpm_status_list[%d].generic_ipmc_status.lrm_serial_id,", i, j);
            fprintf(fp, "computer_status_list[%d].gpm_status_list[%d].generic_ipmc_status.lrm_pcb_revision_number,", i, j);
            fprintf(fp, "computer_status_list[%d].gpm_status_list[%d].generic_ipmc_status.lrm_pcb_assembly_id,", i, j);
            fprintf(fp, "computer_status_list[%d].gpm_status_list[%d].generic_ipmc_status.lrm_mechanical_revision_number,", i, j);
            fprintf(fp, "computer_status_list[%d].gpm_status_list[%d].generic_ipmc_status.operating_mode,", i, j);

            fprintf(fp, "computer_status_list[%d].gpm_status_list[%d].advb_hm_data_status,", i, j);
            fprintf(fp, "computer_status_list[%d].gpm_status_list[%d].advb_tx_3_status,", i, j);
            fprintf(fp, "computer_status_list[%d].gpm_status_list[%d].advb_tx_2_status,", i, j);
            fprintf(fp, "computer_status_list[%d].gpm_status_list[%d].advb_tx_1_status,", i, j);
            fprintf(fp, "computer_status_list[%d].gpm_status_list[%d].advb_status,", i, j);
            fprintf(fp, "computer_status_list[%d].gpm_status_list[%d].dvi_status,", i, j);
            fprintf(fp, "computer_status_list[%d].gpm_status_list[%d].temperature_data_status,", i, j);
            fprintf(fp, "computer_status_list[%d].gpm_status_list[%d].voltage_data_status,", i, j);
        }


        // Hsm_status_list_t hsm_status_list[2];
        for (int j = 0; j < 2; ++j) {
            // Slot_identifier_t slot_idx;
            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].slot_idx.lrm_backplane_slot_index,", i, j);

            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].module_status,", i, j);

            // Generic_ipmc_status_t generic_ipmc_status;
            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].generic_ipmc_status.ipmc_fw_version_major,", i, j);
            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].generic_ipmc_status.ipmc_fw_version_minor,", i, j);
            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].generic_ipmc_status.ipmc_fw_version_patch,", i, j);
            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].generic_ipmc_status.lrm_serial_id,", i, j);
            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].generic_ipmc_status.lrm_pcb_revision_number,", i, j);
            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].generic_ipmc_status.lrm_pcb_assembly_id,", i, j);
            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].generic_ipmc_status.lrm_mechanical_revision_number,", i, j);
            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].generic_ipmc_status.operating_mode,", i, j);

            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].hsm_voltage_status,", i, j);
            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].hsm_current_status,", i, j);
            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].hsm_data_validity,", i, j);

            // Hsn_link_pl_01_08_status_t hsn_link_pl_01_08_status;
            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].hsn_link_pl_01_08_status.hsn_link_pl_01_status,", i, j);
            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].hsn_link_pl_01_08_status.hsn_link_pl_02_status,", i, j);
            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].hsn_link_pl_01_08_status.hsn_link_pl_03_status,", i, j);
            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].hsn_link_pl_01_08_status.hsn_link_pl_04_status,", i, j);
            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].hsn_link_pl_01_08_status.hsn_link_pl_05_status,", i, j);
            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].hsn_link_pl_01_08_status.hsn_link_pl_06_status,", i, j);
            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].hsn_link_pl_01_08_status.hsn_link_pl_07_status,", i, j);
            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].hsn_link_pl_01_08_status.hsn_link_pl_08_status,", i, j);

            fprintf(fp, "computer_status_list[%d].hsm_status_list[%d].hsm_temp_status,", i, j);
        }


        // Dsm_status_list_t dsm_status_list[2];
        for (int j = 0; j < 2; ++j) {
            // Slot_identifier_t slot_idx;
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].slot_idx.lrm_backplane_slot_index,", i, j);

            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].module_status,", i, j);

            // Generic_ipmc_status_t generic_ipmc_status;
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].generic_ipmc_status.ipmc_fw_version_major,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].generic_ipmc_status.ipmc_fw_version_minor,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].generic_ipmc_status.ipmc_fw_version_patch,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].generic_ipmc_status.lrm_serial_id,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].generic_ipmc_status.lrm_pcb_revision_number,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].generic_ipmc_status.lrm_pcb_assembly_id,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].generic_ipmc_status.lrm_mechanical_revision_number,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].generic_ipmc_status.operating_mode,", i, j);

            // Computer_config_mismatch_slot_list_t computer_config_mismatch_slot_list;
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].computer_config_mismatch_slot_list.backplane_slot_17_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].computer_config_mismatch_slot_list.backplane_slot_16_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].computer_config_mismatch_slot_list.backplane_slot_15_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].computer_config_mismatch_slot_list.backplane_slot_14_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].computer_config_mismatch_slot_list.backplane_slot_13_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].computer_config_mismatch_slot_list.backplane_slot_12_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].computer_config_mismatch_slot_list.backplane_slot_11_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].computer_config_mismatch_slot_list.backplane_slot_10_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].computer_config_mismatch_slot_list.backplane_slot_9_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].computer_config_mismatch_slot_list.backplane_slot_8_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].computer_config_mismatch_slot_list.backplane_slot_7_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].computer_config_mismatch_slot_list.backplane_slot_6_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].computer_config_mismatch_slot_list.backplane_slot_5_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].computer_config_mismatch_slot_list.backplane_slot_4_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].computer_config_mismatch_slot_list.backplane_slot_3_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].computer_config_mismatch_slot_list.backplane_slot_2_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].computer_config_mismatch_slot_list.backplane_slot_1_config_status,", i, j);

            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].module_major_frame_alignment_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_es_data_validity,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_sw_data_validity,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_es_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_switch_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].ptp_sync_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].ptp_config_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_es_redundancy_status,", i, j);

            // Dtn_switch_port_link_status_t dtn_switch_port_link_status;
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_switch_port_link_status.dsm_es_link_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_switch_port_link_status.cross_dsm_link_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_switch_port_link_status.fo_2_port_link_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_switch_port_link_status.fo_1_port_link_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_switch_port_link_status.payload_8_link_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_switch_port_link_status.payload_7_link_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_switch_port_link_status.payload_6_link_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_switch_port_link_status.payload_5_link_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_switch_port_link_status.payload_4_link_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_switch_port_link_status.payload_3_link_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_switch_port_link_status.payload_2_link_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_switch_port_link_status.payload_1_link_status,", i, j);

            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_switch_custom_design_version_major,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_switch_custom_design_version_minor,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].dtn_switch_custom_design_version_bugfix,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].temperature_data_status,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].voltage_data_status,", i, j);

            // Mmplcmsw_t2080_dsm_bootloader_pbit_t t2080_dsm_bootloader_pbit;
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].t2080_dsm_bootloader_pbit.ddr_test,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].t2080_dsm_bootloader_pbit.ifc_nand_test,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].t2080_dsm_bootloader_pbit.serdes1_pll1_test,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].t2080_dsm_bootloader_pbit.serdes1_pll2_test,", i, j);
            fprintf(fp, "computer_status_list[%d].dsm_status_list[%d].t2080_dsm_bootloader_pbit.serdes2_pll1_test,", i, j);
        }


        // Iocm_status_t iocm_status;
        // Slot_identifier_t slot_idx;
        fprintf(fp, "computer_status_list[%d].iocm_status.slot_idx.lrm_backplane_slot_index,", i);

        fprintf(fp, "computer_status_list[%d].iocm_status.module_status,", i);

        // Generic_ipmc_status_t generic_ipmc_status;
        fprintf(fp, "computer_status_list[%d].iocm_status.generic_ipmc_status.ipmc_fw_version_major,", i);
        fprintf(fp, "computer_status_list[%d].iocm_status.generic_ipmc_status.ipmc_fw_version_minor,", i);
        fprintf(fp, "computer_status_list[%d].iocm_status.generic_ipmc_status.ipmc_fw_version_patch,", i);
        fprintf(fp, "computer_status_list[%d].iocm_status.generic_ipmc_status.lrm_serial_id,", i);
        fprintf(fp, "computer_status_list[%d].iocm_status.generic_ipmc_status.lrm_pcb_revision_number,", i);
        fprintf(fp, "computer_status_list[%d].iocm_status.generic_ipmc_status.lrm_pcb_assembly_id,", i);
        fprintf(fp, "computer_status_list[%d].iocm_status.generic_ipmc_status.lrm_mechanical_revision_number,", i);
        fprintf(fp, "computer_status_list[%d].iocm_status.generic_ipmc_status.operating_mode,", i);

        fprintf(fp, "computer_status_list[%d].iocm_status.temperature_data_status,", i);
        fprintf(fp, "computer_status_list[%d].iocm_status.voltage_data_status,", i);
        fprintf(fp, "computer_status_list[%d].iocm_status.iocm_dvi_status,", i);


        // Hum_status_list_t hum_status_list[2];
        for (int j = 0; j < 2; ++j) {
            // Slot_identifier_t slot_idx;
            fprintf(fp, "computer_status_list[%d].hum_status_list[%d].slot_idx.lrm_backplane_slot_index,", i, j);

            fprintf(fp, "computer_status_list[%d].hum_status_list[%d].module_status,", i, j);

            // Generic_ipmc_status_t generic_ipmc_status;
            fprintf(fp, "computer_status_list[%d].hum_status_list[%d].generic_ipmc_status.ipmc_fw_version_major,", i, j);
            fprintf(fp, "computer_status_list[%d].hum_status_list[%d].generic_ipmc_status.ipmc_fw_version_minor,", i, j);
            fprintf(fp, "computer_status_list[%d].hum_status_list[%d].generic_ipmc_status.ipmc_fw_version_patch,", i, j);
            fprintf(fp, "computer_status_list[%d].hum_status_list[%d].generic_ipmc_status.lrm_serial_id,", i, j);
            fprintf(fp, "computer_status_list[%d].hum_status_list[%d].generic_ipmc_status.lrm_pcb_revision_number,", i, j);
            fprintf(fp, "computer_status_list[%d].hum_status_list[%d].generic_ipmc_status.lrm_pcb_assembly_id,", i, j);
            fprintf(fp, "computer_status_list[%d].hum_status_list[%d].generic_ipmc_status.lrm_mechanical_revision_number,", i, j);
            fprintf(fp, "computer_status_list[%d].hum_status_list[%d].generic_ipmc_status.operating_mode,", i, j);
        }
    }
    
    // Avionics_dcu_status_list_t avionics_dcu_status_list[2];
    for (int j = 0; j < 2; ++j) {
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_lru_id,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].hold_up_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].voltage_hm_data_status,", j);

        // Dcu_config_status_t dcu_config_status;
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_config_status.m1553_0x110_bus_start_receive_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_config_status.m1553_0x111_bus_start_receive_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_config_status.dcu_channel_config_match_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_config_status.dcu_vl_channel_config_match_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_config_status.m1553_inst_list_0x110_cfg_match_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_config_status.m1553_inst_list_0x111_cfg_match_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_config_status.m1553_lut_0x110_cfg_match_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_config_status.m1553_lut_0x111_cfg_match_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_config_status.dcu_channel_config_status_resp,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_config_status.dcu_vl_channel_config_status_resp,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_config_status.m1553_inst_list_status_0x110_resp,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_config_status.m1553_inst_list_status_0x111_resp,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_config_status.m1553_lut_rcv_status_0x110_resp,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_config_status.m1553_lut_rcv_status_0x111_resp,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_config_status.major_frame_sync_resp,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_config_status.heartbeat_resp,", j);

        fprintf(fp, "avionics_dcu_status_list[%d].dcu_custom_design_version_major,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_custom_design_version_minor,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_custom_design_version_bugfix,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_dtn_es_data_validity,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dtn_es_config_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_dtn_redundancy_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].ptp_config_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].ptp_sync_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].temp_data_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].m1553_watchdog_status_ch_0x110,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].m1553_watchdog_status_ch_0x111,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].m1553_bus_type_ch_0x110,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].m1553_bus_type_ch_0x111,", j);

        // Channel_group_00_status_t channel_group_00_status;
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch31_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch30_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch29_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch28_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch27_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch26_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch25_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch24_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch23_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch22_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch21_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch20_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch19_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch18_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch17_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch16_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch15_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch14_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch13_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch12_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch11_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch10_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch09_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch08_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch07_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch06_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch05_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch04_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch03_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch02_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch01_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_00_status.dogo_ch00_status,", j);

        // Channel_group_01_status_t channel_group_01_status;
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_01_status.dogo_ch41_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_01_status.dogo_ch40_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_01_status.dogo_ch39_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_01_status.dogo_ch38_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_01_status.dogo_ch37_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_01_status.dogo_ch36_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_01_status.dogo_ch35_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_01_status.dogo_ch34_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_01_status.dogo_ch33_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_01_status.dogo_ch32_status,", j);

        // Channel_group_02_status_t channel_group_02_status;
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_02_status.doos_ch01_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_02_status.doos_ch00_status,", j);

        // Channel_group_03_status_t channel_group_03_status;
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch25_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch24_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch23_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch22_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch21_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch20_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch19_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch18_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch17_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch16_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch15_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch14_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch13_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch12_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch11_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch10_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch09_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch08_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch07_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch06_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch05_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch04_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch03_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch02_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch01_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_03_status.doso_ch00_status,", j);

        // Channel_group_04_status_t channel_group_04_status;
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch28_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch27_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch26_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch25_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch24_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch23_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch22_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch21_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch20_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch19_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch18_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch17_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch16_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch15_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch14_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch13_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch12_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch11_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch10_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch09_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch08_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch07_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch06_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch05_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch04_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch03_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch02_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch01_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.cas_digo_ch00_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.icni_digo_ch02_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.icni_digo_ch01_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_04_status.icni_digo_ch00_status,", j);

        // Channel_group_05_status_t channel_group_05_status;
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch60_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch59_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch58_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch57_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch56_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch55_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch54_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch53_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch52_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch51_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch50_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch49_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch48_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch47_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch46_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch45_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch44_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch43_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch42_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch41_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch40_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch39_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch38_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch37_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch36_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch35_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch34_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch33_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch32_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch31_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch30_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_05_status.cas_digo_ch29_status,", j);

        // Channel_group_06_status_t channel_group_06_status;
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.ieos_digo_ch11_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.ieos_digo_ch10_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.ieos_digo_ch09_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.ieos_digo_ch08_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.ieos_digo_ch07_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.ieos_digo_ch06_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.ieos_digo_ch05_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.ieos_digo_ch04_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.ieos_digo_ch03_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.ieos_digo_ch02_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.ieos_digo_ch01_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.ieos_digo_ch00_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch80_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch79_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch78_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch77_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch76_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch75_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch74_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch73_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch72_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch71_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch70_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch69_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch68_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch67_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch66_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch65_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch64_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch63_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch62_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_06_status.cas_digo_ch61_status,", j);

        // Channel_group_07_status_t channel_group_07_status;
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_07_status.wds_diso_ch05_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_07_status.wds_diso_ch04_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_07_status.wds_diso_ch03_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_07_status.wds_diso_ch02_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_07_status.wds_diso_ch01_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_07_status.wds_diso_ch00_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_07_status.irfs_diso_ch03_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_07_status.irfs_diso_ch02_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_07_status.irfs_diso_ch01_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_07_status.irfs_diso_ch00_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_07_status.icni_diso_ch01_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_07_status.icni_diso_ch00_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_07_status.ieos_diso_ch00_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_07_status.cas_diso_ch01_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_07_status.cas_diso_ch00_status,", j);

        // Channel_group_08_status_t channel_group_08_status;
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_08_status.cas_avi_ch03_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_08_status.cas_avi_ch02_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_08_status.cas_avi_ch01_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_08_status.cas_avi_ch00_status,", j);

        // Channel_group_09_status_t channel_group_09_status;
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.ieos_rs422_ch04_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.ieos_rs422_ch03_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.ieos_rs422_ch02_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.ieos_rs422_ch01_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.ieos_rs422_ch00_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.irfs_rs422_ch02_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.irfs_rs422_ch01_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.irfs_rs422_ch00_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.icni_rs422_ch05_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.icni_rs422_ch04_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.icni_rs422_ch03_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.icni_rs422_ch02_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.icni_rs422_ch01_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.icni_rs422_ch00_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.cas_rs422_ch06_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.cas_rs422_ch05_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.cas_rs422_ch04_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.cas_rs422_ch03_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.cas_rs422_ch02_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.cas_rs422_ch01_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_09_status.cas_rs422_ch00_status,", j);

        // Channel_group_10_status_t channel_group_10_status;
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_10_status.icni_a429_ch08_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_10_status.icni_a429_ch07_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_10_status.icni_a429_ch06_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_10_status.icni_a429_ch05_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_10_status.icni_a429_ch04_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_10_status.icni_a429_ch03_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_10_status.icni_a429_ch02_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_10_status.icni_a429_ch01_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_10_status.icni_a429_ch00_status,", j);

        // Channel_group_11_status_t channel_group_11_status;
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_11_status.ethernet_ch01_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_11_status.ethernet_ch00_status,", j);

        // Channel_group_12_status_t channel_group_12_status;
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_12_status.m1553_ch01_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_12_status.m1553_ch00_status,", j);

        // Channel_group_13_status_t channel_group_13_status;
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_13_status.hm_flash_ch01_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].channel_group_13_status.io_vl_config_flash_ch00_status,", j);

        // Sntz_block_status_t sntz_block_status;
        fprintf(fp, "avionics_dcu_status_list[%d].sntz_block_status.adcu_x_sntz_ces_digo_ch00_loss_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].sntz_block_status.adcu_x_sntz_ces_digo_ch01_loss_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].sntz_block_status.adcu_x_sntz_ces_digo_ch02_loss_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].sntz_block_status.adcu_x_sntz_util_digo_ch00_loss_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].sntz_block_status.adcu_x_sntz_cas_diso_ch00_loss_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].sntz_block_status.adcu_x_sntz_dogo_ch00_erroneous_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].sntz_block_status.adcu_x_sntz_dogo_ch01_erroneous_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].sntz_block_status.adcu_x_sntz_dogo_ch02_erroneous_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].sntz_block_status.adcu_x_sntz_dogo_ch03_erroneous_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].sntz_block_status.adcu_x_sntz_doso_ch00_erroneous_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].sntz_block_status.adcu_x_sntz_doos_ch00_erroneous_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].sntz_block_status.adcu_x_sntz_doos_ch01_erroneous_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].sntz_block_status.adcu_x_sntz_doos_ch02_erroneous_status,", j);

        // Block_status_t block_status;
        fprintf(fp, "avionics_dcu_status_list[%d].block_status.adcu_x_do_block_erroneous_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].block_status.adcu_x_cas_block_loss_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].block_status.adcu_x_icni_block_loss_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].block_status.adcu_x_ieos_block_loss_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].block_status.adcu_x_irfs_block_loss_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].block_status.adcu_x_wds_block_loss_status,", j);

        // Block_voltage_hm_data_status_t block_voltage_hm_data_status;
        fprintf(fp, "avionics_dcu_status_list[%d].block_voltage_hm_data_status.adcu_x_cas_voltage_hm_data_loss_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].block_voltage_hm_data_status.adcu_x_icni_voltage_hm_data_loss_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].block_voltage_hm_data_status.adcu_x_ieos_voltage_hm_data_loss_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].block_voltage_hm_data_status.adcu_x_irfs_voltage_hm_data_loss_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].block_voltage_hm_data_status.adcu_x_wds_voltage_hm_data_loss_status,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].block_voltage_hm_data_status.adcu_x_sntz_voltage_hm_data_loss_status,", j);

        // Dcu_ethernet_status_t dcu_ethernet_status;
        // Dcu_ethernet_status_ch_0_x0100_t dcu_ethernet_status_ch_0_x0100;
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_ethernet_status.dcu_ethernet_status_ch_0_x0100.identifier_error,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_ethernet_status.dcu_ethernet_status_ch_0_x0100.ethernet_mode,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_ethernet_status.dcu_ethernet_status_ch_0_x0100.ethernet_speed,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_ethernet_status.dcu_ethernet_status_ch_0_x0100.link_status,", j);
        // Dcu_ethernet_status_ch_0_x0101_t dcu_ethernet_status_ch_0_x0101;
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_ethernet_status.dcu_ethernet_status_ch_0_x0101.identifier_error,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_ethernet_status.dcu_ethernet_status_ch_0_x0101.ethernet_mode,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_ethernet_status.dcu_ethernet_status_ch_0_x0101.ethernet_speed,", j);
        fprintf(fp, "avionics_dcu_status_list[%d].dcu_ethernet_status.dcu_ethernet_status_ch_0_x0101.link_status,", j);
    }


    // Dtn_ir_sw_status_list_t dtn_ir_sw_status_list[2];
    for (int j = 0; j < 2; ++j) {
        fprintf(fp, "dtn_ir_sw_status_list[%d].lru_id,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_manager_group1_data_validity,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_manager_group2_data_validity,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_manager_group3_data_validity,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_mcu_data_validity,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_assistant_group1_data_validity,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_assistant_group2_data_validity,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_dtn_es_data_validity,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_config_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].ptp_sync_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].ptp_config_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_cd_version_status,", j);

        // Dtn_ir_sw_port_link_status_t dtn_ir_sw_port_link_status;
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_33_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_32_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_31_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_30_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_29_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_28_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_27_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_26_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_25_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_24_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_23_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_22_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_21_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_20_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_19_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_18_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_17_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_16_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_15_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_14_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_13_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_12_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_11_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_10_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_9_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_8_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_7_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_6_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_5_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_4_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_3_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_2_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_1_link_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_port_link_status.port_0_link_status,", j);

        // Dtn_ir_sw_fpga_status_t dtn_ir_sw_fpga_status;
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_fpga_status.fpga_1_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_fpga_status.fpga_0_status,", j);

        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_custom_design_version_major,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_custom_design_version_minor,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_custom_design_version_bugfix,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_connection_csfdr_status,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].mcu_firmware_version_major,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].mcu_firmware_version_minor,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].mcu_firmware_version_bugfix,", j);
        fprintf(fp, "dtn_ir_sw_status_list[%d].dtn_ir_sw_mcu_hm_data_status%s", j, (j == 0) ? "," : "\n");
    }
}

static int mmpl_cmsw_mmp_status_parser(FILE *outfp, Mmplcmsw_tai_icd_status_t *mmplcmsw_tai_icd_status) {
    
    mmpl_cmsw_mmp_status_endiannes_converter(mmplcmsw_tai_icd_status);

    fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->mmp_status_report);
    fprintf(outfp, "%llu,", mmplcmsw_tai_icd_status->timestamp);
    fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->master_status.master_cmc_id);
    fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->master_status.master_backplane_slot_index);
    fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->master_status.master_hb_status);
    fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->backup_status.backup_lru_id);
    fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->backup_status.backup_backplane_slot_index);
    fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->backup_status.backup_hb_status);
    fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->mmp_state);
    fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->current_platform_config_id);
    for (int i = 0; i < 2; i++) {
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].computer_lru_id);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].computer_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].computer_chassis_manager_validity);

            for (int j = 0; j < 5; j++) {
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].slot_idx.lrm_backplane_slot_index);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].module_status);
                
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].generic_ipmc_status.ipmc_fw_version_major);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].generic_ipmc_status.ipmc_fw_version_minor);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].generic_ipmc_status.ipmc_fw_version_patch);
                fprintf(outfp, "%llu,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].generic_ipmc_status.lrm_serial_id);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].generic_ipmc_status.lrm_pcb_revision_number);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].generic_ipmc_status.lrm_pcb_assembly_id);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].generic_ipmc_status.lrm_mechanical_revision_number);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].generic_ipmc_status.operating_mode);

                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].schedule_config_status);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].major_frame_alignment_status);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].temperature_data_status);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].voltage_data_status);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].dtn_es_data_validity);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].dtn_es_config_status);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].dtn_es_redundancy_status);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].ptp_sync_status);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].ptp_config_status);

                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].dtn_es_custom_design_version.dtn_es_custom_design_version_major);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].dtn_es_custom_design_version.dtn_es_custom_design_version_minor);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].dtn_es_custom_design_version.dtn_es_custom_design_version_bugfix);

                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].t2080_dpm_bootloader_pbit.ddr_test);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].t2080_dpm_bootloader_pbit.ifc_nand_test);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].t2080_dpm_bootloader_pbit.serdes1_pll1_test);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].t2080_dpm_bootloader_pbit.serdes2_pll1_test);
                fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dpm_status_list[j].t2080_dpm_bootloader_pbit.serdes2_pll2_test);
            }

        for (int j = 0; j < 2; j++) {
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].psm_status_list[j].slot_idx.lrm_backplane_slot_index);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].psm_status_list[j].module_status);
            
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].psm_status_list[j].generic_ipmc_status.ipmc_fw_version_major);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].psm_status_list[j].generic_ipmc_status.ipmc_fw_version_minor);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].psm_status_list[j].generic_ipmc_status.ipmc_fw_version_patch);
            fprintf(outfp, "%llu,", mmplcmsw_tai_icd_status->computer_status_list[i].psm_status_list[j].generic_ipmc_status.lrm_serial_id);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].psm_status_list[j].generic_ipmc_status.lrm_pcb_revision_number);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].psm_status_list[j].generic_ipmc_status.lrm_pcb_assembly_id);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].psm_status_list[j].generic_ipmc_status.lrm_mechanical_revision_number);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].psm_status_list[j].generic_ipmc_status.operating_mode);
        }


        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.slot_idx.lrm_backplane_slot_index);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.module_status);
        
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.generic_ipmc_status.ipmc_fw_version_major);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.generic_ipmc_status.ipmc_fw_version_minor);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.generic_ipmc_status.ipmc_fw_version_patch);
        fprintf(outfp, "%llu,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.generic_ipmc_status.lrm_serial_id);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.generic_ipmc_status.lrm_pcb_revision_number);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.generic_ipmc_status.lrm_pcb_assembly_id);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.generic_ipmc_status.lrm_mechanical_revision_number);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.generic_ipmc_status.operating_mode);

        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.voltage_data_status);

        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.interface_cbit_result.trng_cbit_result);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.interface_cbit_result.key_cbit_result);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.interface_cbit_result.algorithm_cbit_result);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.interface_cbit_result.fo_intf_cbit_result);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.interface_cbit_result.uart_sm_key_storage_cbit_result);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.interface_cbit_result.uart_sm_mngr_cbit_result);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.interface_cbit_result.dtn_1_cbit_result);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.interface_cbit_result.dtn_0_cbit_result);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.interface_cbit_result.pcie_mass_storage_cbit_result);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.interface_cbit_result.pcie_backplane_cbit_result);

        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.alarm_status.crypto_algorithm_fail_alarm_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.alarm_status.crypto_key_integrity_fail_alarm_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.alarm_status.power_supply_high_voltage_alarm_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.alarm_status.power_supply_low_voltage_alarm_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.alarm_status.module_high_temp_alarm_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.alarm_status.battery_voltage_low_alarm);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.alarm_status.external_tamper_detection_alarm_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.alarm_status.internal_tamper_detection_alarm_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.alarm_status.emergency_zeroization_alarm_status);

        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.fpga_code_version);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.security_cntrlr_mcu_fw_version);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].smmm_status.smmm_fo_status);

        for (int j = 0; j < 2; j++) {
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].slot_idx.lrm_backplane_slot_index);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].module_status);
            
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].generic_ipmc_status.ipmc_fw_version_major);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].generic_ipmc_status.ipmc_fw_version_minor);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].generic_ipmc_status.ipmc_fw_version_patch);
            fprintf(outfp, "%llu,", mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].generic_ipmc_status.lrm_serial_id);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].generic_ipmc_status.lrm_pcb_revision_number);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].generic_ipmc_status.lrm_pcb_assembly_id);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].generic_ipmc_status.lrm_mechanical_revision_number);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].generic_ipmc_status.operating_mode);

            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].advb_hm_data_status);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].advb_tx_3_status);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].advb_tx_2_status);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].advb_tx_1_status);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].advb_status);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].dvi_status);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].temperature_data_status);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].gpm_status_list[j].voltage_data_status);
        }

        for (int j = 0; j < 2; j++) {
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].slot_idx.lrm_backplane_slot_index);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].module_status);
            
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].generic_ipmc_status.ipmc_fw_version_major);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].generic_ipmc_status.ipmc_fw_version_minor);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].generic_ipmc_status.ipmc_fw_version_patch);
            fprintf(outfp, "%llu,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].generic_ipmc_status.lrm_serial_id);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].generic_ipmc_status.lrm_pcb_revision_number);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].generic_ipmc_status.lrm_pcb_assembly_id);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].generic_ipmc_status.lrm_mechanical_revision_number);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].generic_ipmc_status.operating_mode);

            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].hsm_voltage_status);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].hsm_current_status);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].hsm_data_validity);

            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].hsn_link_pl_01_08_status.hsn_link_pl_01_status);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].hsn_link_pl_01_08_status.hsn_link_pl_02_status);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].hsn_link_pl_01_08_status.hsn_link_pl_03_status);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].hsn_link_pl_01_08_status.hsn_link_pl_04_status);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].hsn_link_pl_01_08_status.hsn_link_pl_05_status);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].hsn_link_pl_01_08_status.hsn_link_pl_06_status);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].hsn_link_pl_01_08_status.hsn_link_pl_07_status);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].hsn_link_pl_01_08_status.hsn_link_pl_08_status);

            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hsm_status_list[j].hsm_temp_status);
        }

    for (int j = 0; j < 2; j++) {
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].slot_idx.lrm_backplane_slot_index);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].module_status);
        
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].generic_ipmc_status.ipmc_fw_version_major);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].generic_ipmc_status.ipmc_fw_version_minor);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].generic_ipmc_status.ipmc_fw_version_patch);
        fprintf(outfp, "%llu,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].generic_ipmc_status.lrm_serial_id);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].generic_ipmc_status.lrm_pcb_revision_number);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].generic_ipmc_status.lrm_pcb_assembly_id);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].generic_ipmc_status.lrm_mechanical_revision_number);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].generic_ipmc_status.operating_mode);

        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].computer_config_mismatch_slot_list.backplane_slot_17_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].computer_config_mismatch_slot_list.backplane_slot_16_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].computer_config_mismatch_slot_list.backplane_slot_15_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].computer_config_mismatch_slot_list.backplane_slot_14_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].computer_config_mismatch_slot_list.backplane_slot_13_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].computer_config_mismatch_slot_list.backplane_slot_12_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].computer_config_mismatch_slot_list.backplane_slot_11_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].computer_config_mismatch_slot_list.backplane_slot_10_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].computer_config_mismatch_slot_list.backplane_slot_9_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].computer_config_mismatch_slot_list.backplane_slot_8_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].computer_config_mismatch_slot_list.backplane_slot_7_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].computer_config_mismatch_slot_list.backplane_slot_6_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].computer_config_mismatch_slot_list.backplane_slot_5_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].computer_config_mismatch_slot_list.backplane_slot_4_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].computer_config_mismatch_slot_list.backplane_slot_3_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].computer_config_mismatch_slot_list.backplane_slot_2_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].computer_config_mismatch_slot_list.backplane_slot_1_config_status);

        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].module_major_frame_alignment_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_es_data_validity);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_sw_data_validity);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_es_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_switch_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].ptp_sync_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].ptp_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_es_redundancy_status);

        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_switch_port_link_status.dsm_es_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_switch_port_link_status.cross_dsm_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_switch_port_link_status.fo_2_port_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_switch_port_link_status.fo_1_port_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_switch_port_link_status.payload_8_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_switch_port_link_status.payload_7_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_switch_port_link_status.payload_6_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_switch_port_link_status.payload_5_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_switch_port_link_status.payload_4_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_switch_port_link_status.payload_3_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_switch_port_link_status.payload_2_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_switch_port_link_status.payload_1_link_status);

        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_switch_custom_design_version_major);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_switch_custom_design_version_minor);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].dtn_switch_custom_design_version_bugfix);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].temperature_data_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].voltage_data_status);

        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].t2080_dsm_bootloader_pbit.ddr_test);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].t2080_dsm_bootloader_pbit.ifc_nand_test);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].t2080_dsm_bootloader_pbit.serdes1_pll1_test);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].t2080_dsm_bootloader_pbit.serdes1_pll2_test);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].dsm_status_list[j].t2080_dsm_bootloader_pbit.serdes2_pll1_test);
    }
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.slot_idx.lrm_backplane_slot_index);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.module_status);
        
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.generic_ipmc_status.ipmc_fw_version_major);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.generic_ipmc_status.ipmc_fw_version_minor);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.generic_ipmc_status.ipmc_fw_version_patch);
        fprintf(outfp, "%llu,", mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.generic_ipmc_status.lrm_serial_id);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.generic_ipmc_status.lrm_pcb_revision_number);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.generic_ipmc_status.lrm_pcb_assembly_id);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.generic_ipmc_status.lrm_mechanical_revision_number);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.generic_ipmc_status.operating_mode);

        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.temperature_data_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.voltage_data_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].iocm_status.iocm_dvi_status);

        for (int j = 0; j < 2; j++) {
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hum_status_list[j].slot_idx.lrm_backplane_slot_index);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hum_status_list[j].module_status);
            
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hum_status_list[j].generic_ipmc_status.ipmc_fw_version_major);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hum_status_list[j].generic_ipmc_status.ipmc_fw_version_minor);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hum_status_list[j].generic_ipmc_status.ipmc_fw_version_patch);
            fprintf(outfp, "%llu,", mmplcmsw_tai_icd_status->computer_status_list[i].hum_status_list[j].generic_ipmc_status.lrm_serial_id);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hum_status_list[j].generic_ipmc_status.lrm_pcb_revision_number);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hum_status_list[j].generic_ipmc_status.lrm_pcb_assembly_id);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hum_status_list[j].generic_ipmc_status.lrm_mechanical_revision_number);
            fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->computer_status_list[i].hum_status_list[j].generic_ipmc_status.operating_mode);
        }
    }

    for (int j = 0; j < 2; j++) {
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_lru_id);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].hold_up_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].voltage_hm_data_status);

        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_config_status.m1553_0x110_bus_start_receive_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_config_status.m1553_0x111_bus_start_receive_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_config_status.dcu_channel_config_match_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_config_status.dcu_vl_channel_config_match_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_config_status.m1553_inst_list_0x110_cfg_match_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_config_status.m1553_inst_list_0x111_cfg_match_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_config_status.m1553_lut_0x110_cfg_match_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_config_status.m1553_lut_0x111_cfg_match_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_config_status.dcu_channel_config_status_resp);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_config_status.dcu_vl_channel_config_status_resp);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_config_status.m1553_inst_list_status_0x110_resp);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_config_status.m1553_inst_list_status_0x111_resp);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_config_status.m1553_lut_rcv_status_0x110_resp);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_config_status.m1553_lut_rcv_status_0x111_resp);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_config_status.major_frame_sync_resp);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_config_status.heartbeat_resp);

        // Versiyon ve DTN/PTP Durumları
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_custom_design_version_major);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_custom_design_version_minor);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_custom_design_version_bugfix);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_dtn_es_data_validity);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dtn_es_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_dtn_redundancy_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].ptp_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].ptp_sync_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].temp_data_status);

        // M1553 Bitfield Alanları
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].m1553_watchdog_status_ch_0x110);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].m1553_watchdog_status_ch_0x111);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].m1553_bus_type_ch_0x110);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].m1553_bus_type_ch_0x111);

        // Channel Group 00 Status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch31_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch30_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch29_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch28_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch27_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch26_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch25_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch24_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch23_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch22_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch21_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch20_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch19_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch18_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch17_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch16_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch15_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch14_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch13_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch12_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch11_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch10_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch09_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch08_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch07_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch06_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch05_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch04_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch03_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch02_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch01_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_00_status.dogo_ch00_status);

        // Channel Group 01 Status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_01_status.dogo_ch41_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_01_status.dogo_ch40_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_01_status.dogo_ch39_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_01_status.dogo_ch38_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_01_status.dogo_ch37_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_01_status.dogo_ch36_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_01_status.dogo_ch35_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_01_status.dogo_ch34_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_01_status.dogo_ch33_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_01_status.dogo_ch32_status);

        // Channel Group 02 Status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_02_status.doos_ch01_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_02_status.doos_ch00_status);

        // Channel Group 03 Status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch25_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch24_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch23_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch22_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch21_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch20_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch19_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch18_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch17_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch16_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch15_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch14_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch13_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch12_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch11_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch10_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch09_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch08_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch07_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch06_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch05_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch04_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch03_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch02_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch01_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_03_status.doso_ch00_status);

        // Channel Group 04 Status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch28_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch27_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch26_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch25_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch24_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch23_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch22_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch21_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch20_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch19_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch18_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch17_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch16_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch15_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch14_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch13_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch12_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch11_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch10_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch09_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch08_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch07_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch06_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch05_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch04_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch03_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch02_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch01_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.cas_digo_ch00_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.icni_digo_ch02_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.icni_digo_ch01_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_04_status.icni_digo_ch00_status);

        // Channel Group 05 Status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch60_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch59_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch58_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch57_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch56_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch55_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch54_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch53_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch52_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch51_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch50_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch49_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch48_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch47_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch46_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch45_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch44_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch43_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch42_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch41_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch40_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch39_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch38_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch37_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch36_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch35_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch34_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch33_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch32_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch31_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch30_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_05_status.cas_digo_ch29_status);

        // Channel Group 06 Status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.ieos_digo_ch11_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.ieos_digo_ch10_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.ieos_digo_ch09_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.ieos_digo_ch08_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.ieos_digo_ch07_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.ieos_digo_ch06_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.ieos_digo_ch05_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.ieos_digo_ch04_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.ieos_digo_ch03_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.ieos_digo_ch02_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.ieos_digo_ch01_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.ieos_digo_ch00_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch80_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch79_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch78_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch77_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch76_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch75_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch74_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch73_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch72_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch71_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch70_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch69_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch68_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch67_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch66_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch65_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch64_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch63_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch62_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_06_status.cas_digo_ch61_status);

        // Channel Group 07 Status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_07_status.wds_diso_ch05_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_07_status.wds_diso_ch04_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_07_status.wds_diso_ch03_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_07_status.wds_diso_ch02_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_07_status.wds_diso_ch01_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_07_status.wds_diso_ch00_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_07_status.irfs_diso_ch03_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_07_status.irfs_diso_ch02_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_07_status.irfs_diso_ch01_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_07_status.irfs_diso_ch00_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_07_status.icni_diso_ch01_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_07_status.icni_diso_ch00_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_07_status.ieos_diso_ch00_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_07_status.cas_diso_ch01_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_07_status.cas_diso_ch00_status);

        // Channel Group 08 Status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_08_status.cas_avi_ch03_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_08_status.cas_avi_ch02_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_08_status.cas_avi_ch01_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_08_status.cas_avi_ch00_status);

        // Channel Group 09 Status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.ieos_rs422_ch04_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.ieos_rs422_ch03_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.ieos_rs422_ch02_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.ieos_rs422_ch01_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.ieos_rs422_ch00_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.irfs_rs422_ch02_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.irfs_rs422_ch01_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.irfs_rs422_ch00_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.icni_rs422_ch05_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.icni_rs422_ch04_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.icni_rs422_ch03_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.icni_rs422_ch02_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.icni_rs422_ch01_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.icni_rs422_ch00_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.cas_rs422_ch06_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.cas_rs422_ch05_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.cas_rs422_ch04_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.cas_rs422_ch03_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.cas_rs422_ch02_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.cas_rs422_ch01_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_09_status.cas_rs422_ch00_status);

        // Channel Group 10 Status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_10_status.icni_a429_ch08_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_10_status.icni_a429_ch07_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_10_status.icni_a429_ch06_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_10_status.icni_a429_ch05_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_10_status.icni_a429_ch04_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_10_status.icni_a429_ch03_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_10_status.icni_a429_ch02_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_10_status.icni_a429_ch01_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_10_status.icni_a429_ch00_status);

        // Channel Group 11 Status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_11_status.ethernet_ch01_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_11_status.ethernet_ch00_status);

        // Channel Group 12 Status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_12_status.m1553_ch01_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_12_status.m1553_ch00_status);

        // Channel Group 13 Status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_13_status.hm_flash_ch01_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].channel_group_13_status.io_vl_config_flash_ch00_status);

        // Sntz_block_status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].sntz_block_status.adcu_x_sntz_ces_digo_ch00_loss_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].sntz_block_status.adcu_x_sntz_ces_digo_ch01_loss_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].sntz_block_status.adcu_x_sntz_ces_digo_ch02_loss_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].sntz_block_status.adcu_x_sntz_util_digo_ch00_loss_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].sntz_block_status.adcu_x_sntz_cas_diso_ch00_loss_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].sntz_block_status.adcu_x_sntz_dogo_ch00_erroneous_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].sntz_block_status.adcu_x_sntz_dogo_ch01_erroneous_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].sntz_block_status.adcu_x_sntz_dogo_ch02_erroneous_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].sntz_block_status.adcu_x_sntz_dogo_ch03_erroneous_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].sntz_block_status.adcu_x_sntz_doso_ch00_erroneous_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].sntz_block_status.adcu_x_sntz_doos_ch00_erroneous_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].sntz_block_status.adcu_x_sntz_doos_ch01_erroneous_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].sntz_block_status.adcu_x_sntz_doos_ch02_erroneous_status);

        // Block_status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].block_status.adcu_x_do_block_erroneous_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].block_status.adcu_x_cas_block_loss_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].block_status.adcu_x_icni_block_loss_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].block_status.adcu_x_ieos_block_loss_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].block_status.adcu_x_irfs_block_loss_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].block_status.adcu_x_wds_block_loss_status);

        // Block_voltage_hm_data_status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].block_voltage_hm_data_status.adcu_x_cas_voltage_hm_data_loss_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].block_voltage_hm_data_status.adcu_x_icni_voltage_hm_data_loss_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].block_voltage_hm_data_status.adcu_x_ieos_voltage_hm_data_loss_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].block_voltage_hm_data_status.adcu_x_irfs_voltage_hm_data_loss_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].block_voltage_hm_data_status.adcu_x_wds_voltage_hm_data_loss_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].block_voltage_hm_data_status.adcu_x_sntz_voltage_hm_data_loss_status);

        // Dcu_ethernet_status
        // Ch 0x0100
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_ethernet_status.dcu_ethernet_status_ch_0_x0100.identifier_error);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_ethernet_status.dcu_ethernet_status_ch_0_x0100.ethernet_mode);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_ethernet_status.dcu_ethernet_status_ch_0_x0100.ethernet_speed);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_ethernet_status.dcu_ethernet_status_ch_0_x0100.link_status);
        // Ch 0x0101
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_ethernet_status.dcu_ethernet_status_ch_0_x0101.identifier_error);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_ethernet_status.dcu_ethernet_status_ch_0_x0101.ethernet_mode);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_ethernet_status.dcu_ethernet_status_ch_0_x0101.ethernet_speed);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->avionics_dcu_status_list[j].dcu_ethernet_status.dcu_ethernet_status_ch_0_x0101.link_status);
    }
    /* dtn_ir_sw_status_list elemanlarını dönen döngü */
    for (int k = 0; k < 2; k++) 
    {
        // LRU ID (8-bit)
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].lru_id);
        
        // DTN IR SW Status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_status);
        
        // Data Validity Fields
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_manager_group1_data_validity);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_manager_group2_data_validity);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_manager_group3_data_validity);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_mcu_data_validity);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_assistant_group1_data_validity);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_assistant_group2_data_validity);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_dtn_es_data_validity);
        
        // Configuration & Sync Status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].ptp_sync_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].ptp_config_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_cd_version_status);


        // DTN IR SW Port Link Status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_33_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_32_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_31_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_30_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_29_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_28_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_27_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_26_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_25_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_24_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_23_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_22_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_21_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_20_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_19_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_18_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_17_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_16_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_15_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_14_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_13_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_12_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_11_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_10_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_9_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_8_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_7_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_6_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_5_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_4_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_3_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_2_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_1_link_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_port_link_status.port_0_link_status);

        // DTN IR SW FPGA Status
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_fpga_status.fpga_1_status);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_fpga_status.fpga_0_status);

        // Custom Design Versions
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_custom_design_version_major);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_custom_design_version_minor);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_custom_design_version_bugfix);

        // CSFDR Connection
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_connection_csfdr_status);

        // MCU Firmware Versions
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].mcu_firmware_version_major);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].mcu_firmware_version_minor);
        fprintf(outfp, "%u,", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].mcu_firmware_version_bugfix);

        // MCU HM Data Status
        fprintf(outfp, "%u%s", mmplcmsw_tai_icd_status->dtn_ir_sw_status_list[k].dtn_ir_sw_mcu_hm_data_status, (k == 0) ? "," : "\n");
    }
}
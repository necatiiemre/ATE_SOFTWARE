#pragma once

#include <stdint.h>
#include "log_dtnes.h"
#include "log_t2080_exception.h"

typedef struct __attribute__((packed)){
    uint8_t msg_type; /*STATUS REPORT*/
    uint16_t device_id;
    uint8_t padding;
    uint64_t timestamp;

    int32_t a653_schedule_id;
    uint32_t dtn_es_cfg_id;

    uint16_t ptp_cfg_id;
    uint8_t ptp_tod_network;
    uint8_t ptp_device_type;
    uint8_t ptp_synch_status;

    uint8_t dtn_es_mon_data_validity;
    uint8_t dtn_es_redundancy_status;

    uint8_t dtn_es_fw_vers_major;
    uint8_t dtn_es_fw_vers_minor;
    uint8_t dtn_es_fw_vers_micro;

    uint8_t hsn_link_status;
    uint8_t major_frame_alignment_status;
    uint16_t monolith_id;
}Cmsw_ml_status_msg_type;
#if 0
typedef enum
{
    MLCMSW_INIT_FAIL                        = 0,
    MLCMSW_ALREADY_INIT                     = 1,
    MLCMSW_TS_GET_CFG_FILE_FAIL             = 2,
    MLCMSW_TS_CFG_FILE_EMPTY                = 3,
    MLCMSW_NOT_ENOUGH_MEM                   = 4,
    MLCMSW_TS_IMPORT_CFG_FAIL               = 5,
    MLCMSW_TS_INIT_FAIL                     = 6,
    MLCMSW_PCS_HAL_INIT_FAIL                = 7,
    MLCMSW_DTN_INIT_CFG_FAIL                = 8,
    MLCMSW_PCS_GET_GEO_ADDR_FAIL            = 9,
	MLCMSW_GEO_ADDR_PARITY_ERROR            = 10,
    MLCMSW_LRU_ID_FAIL                      = 11,
    MLCMSW_SLOT_ID_FAIL                     = 12,
    MLCMSW_DTN_SET_DEV_ID_FAIL              = 13,
    MLCMSW_DTN_FILE_NAME_FAIL               = 14,
    MLCMSW_DTN_GET_CFG_FILE_SIZE_ERROR      = 15,
    MLCMSW_DTN_CFG_FILE_SIZE_NULL           = 16,
    MLCMSW_DTN_CFG_FILE_NULL_CHECK_FAIL     = 17,
    MLCMSW_DTN_PCS_IMPORT_CFG_FAIL          = 18,
    MLCMSW_DTN_PDI_CFG_FAIL                 = 19,
    MLCMSW_SMMM_LA_OPEN_FAIL                = 20,
    MLCMSW_SMMM_LA_CONFIG_SUCCESS           = 21,
    MLCMSW_SMMM_LA_CONFIG_FAIL              = 22,
    MLCMSW_NO_SMMM_LA_CONFIG                = 23,
    MLCMSW_GET_CONFIGURATION_FAIL           = 24,
    MLCMSW_CONNECTIONS_INIT_FAIL            = 25,
    MLCMSW_STATUS_CONN_CREATE_FAIL          = 26,
    MLCMSW_RCV0_CONN_CREATE_FAIL            = 27,
    MLCMSW_RCV1_CONN_CREATE_FAIL            = 28,
    MLCMSW_PCS_SAL_INIT_FAIL                = 29,
    MLCMSW_VARS_INIT_FAIL                   = 30,
    MLCMSW_GET_PART_STAT_FAIL               = 31,
    MLCMSW_CREATE_PROC_FAIL                 = 32,
    MLCMSW_START_GET_PROC_STAT_FAIL         = 33,
    MLCMSW_START_NOT_INIT                   = 34,
    MLCMSW_START_PROC_START_FAIL            = 35,
    MLCMSW_PTP_SYNCH_HR_TIME_ERR            = 36,
    MLCMSW_PROC_TIME_GET_ERR                = 37,
    MLCMSW_SET_A653_SCH_REQ_INFO            = 38,
    MLCMSW_SET_ALIGN_WITH_SCH_REQ_INFO      = 39,
    MLCMSW_SET_ALIGN_A653_MF_REQ_INFO       = 40,
    MLCMSW_CURR_SCH_EQUAL_NEXT_SCH          = 41,
    MLCMSW_SET_SCH_FAIL                     = 42,
    MLCMSW_SET_SCH_SUCCESS                  = 43,
    MLCMSW_PCS_MF_SYNC_SUCC                 = 44,
    MLCMSW_PCS_MF_SYNC_FAIL                 = 45,
    MLCMSW_PTP_OUT_OF_SYNC                  = 46,
    MLCMSW_MF_REQ_PTP_SYNC_FAIL             = 47,
    MLCMSW_INVALID_REQ                      = 48,
    MLCMSW_DTN_MON_DATA_NULL                = 49,
    MLCMSW_DTN_MON_DATA_INVALID             = 50,
    MLCMSW_GM_PTP_SYNC_VALID                = 51,
    MLCMSW_SLAVE_A_PTP_SYNC_VALID           = 52,
    MLCMSW_SLAVE_B_PTP_SYNC_VALID           = 53,
    MLCMSW_HSN_NOT_INITIALIZED              = 54,
    MLCMSW_HSN_LINK_STATUS_INVALID          = 55,
    MLCMSW_GET_MOD_SCH_STAT_FAIL            = 56,
    MLCMSW_SEND_STATUS_MSG_FAIL             = 57,
    MLCMSW_LOG_STATUS_MSG_FAIL              = 58,
    MLCMSW_STATUS_LOG_BUFFER_OVERFLOW       = 59,
    MLCMSW_PCS_READ_HM_STAT_FAIL            = 60,
    MLCMSW_PCS_READ_HM_LOG_FAIL             = 61,
    MLCMSW_HM_LOG_BUFFER_OVERFLOW           = 62,
    MLCMSW_LOG_HM_LOG_FAIL                  = 63,
    MLCMSW_HM_LOG_EXISTS                    = 64,
    MLCMSW_DTN_MON_DATA_NULL_2              = 65,
    MLCMSW_DTN_MON_DATA_BUFFER_FULL         = 66,
    MLCMSW_LOG_DTN_MON_DATA_FAIL            = 67,
	MLCMSW_INITIAL_DTN_MON_FAIL             = 68,
	MLCMSW_DTN_ES_ALREADY_DONE              = 69,
	MLCMSW_INITIAL_DTN_MON_BUFFER_NULL		= 70,
	MLCMSW_WATCHDOG_SET_TIMEOUT_FAIL        = 71,
    MLCMSW_WATCHDOG_SET_TIMEOUT_SUCCESS     = 72,
    MLCMSW_WATCHDOG_KICK_FAIL               = 73,
    MLCMSW_MF_SYNCH_TIME_EXCEED_WD          = 74,
    MLCMSW_MF_SYNCH_WD_TIMEOUT_NOT_VALID    = 75,
	MLCMSW_WATCHDOG_FREEZE_MODE             = 76,
	MLCMSW_WATCHDOG_RUN_MODE                = 77,
	MLCMSW_WATCHDOG_TIMEOUT_VAL_INVALID     = 78,
	MLCMSW_MF_SYNCH_HR_TIME_GET_FAIL        = 79,
    MLCMSW_LOGTYPE_MAX
} MLCMSW_LOG_TYPE;
#endif

/**
 * @enum Mlcmsw_log_type
 * 
 * @brief Enumeration of ML CMSW system return codes and status indicators.
 * 
 * @details This enumeration defines a comprehensive set of error codes, status messages, and success indicators used
 *          throughout the ML CMSW subsystem. It covers various modules including Watchdog, DTN, SMMM, PTP
 *          synchronization, and configuration management.
 * 
 * @note These values are used for error handling and logging purposes to identify the exact point of failure or the
 *       current operational state.
 */
typedef enum Mlcmsw_log_type
{
    /** @brief MLCMSW initialization failure. */
    MLCMSW_INIT_FAIL_DETECTIONS = 0,

    /** @brief MLCMSW already initialized. */
    MLCMSW_ALREADY_INIT_DETECTIONS = 1,

    /** @brief Failed to get TS configuration file. */
    MLCMSW_TS_IMPORT_CFG_FAIL_DETECTIONS = 2,

    /** @brief TS initialization has failed.*/
    MLCMSW_TS_INIT_FAIL_DETECTIONS = 3,

    /** @brief PCS HAL initialization has failed.*/
    MLCMSW_PCS_HAL_INIT_FAIL_DETECTIONS = 4,

    /** @brief Configuration of DTN has failed.*/
    MLCMSW_DTN_INIT_CFG_FAIL_DETECTIONS = 5,

    /** @brief pcs_get_geographical_address API returned an error. */
    MLCMSW_PCS_GET_GEO_ADDR_FAIL_DETECTIONS = 6,

    /** @brief pcs_get_geographical_address returned @ref PARITY_ERROR. */
    MLCMSW_GEO_ADDR_PARITY_ERROR_DETECTIONS = 7,

    /** @brief pcs_get_geographical_address returned LRU ID that in out of bounds. (shall be in [0-127])*/
    MLCMSW_LRU_ID_FAIL_DETECTIONS = 8,

    /** @brief pcs_get_geographical_address returned Backplane Slot ID that in out of bounds. (shall be in [1-17])*/
    MLCMSW_SLOT_ID_FAIL_DETECTIONS = 9,

    /** @brief a664ESSetDeviceID failed while retrieving geographical address. */
    MLCMSW_DTN_SET_DEV_ID_FAIL_DETECTIONS = 10,

    /** @brief Fail during loading DTN ES configuration file. */
    MLCMSW_DTN_IMPORT_CFG_FAIL_DETECTIONS = 11,

    /** @brief a664ESPDIConfig API returned an error. */
    MLCMSW_DTN_PDI_CFG_FAIL_DETECTIONS = 12,

    /** @brief open() call failed during SMMM LA driver configuration. */
    MLCMSW_SMMM_LA_OPEN_FAIL_DETECTIONS = 13,

    /** @brief SMMM LA driver configuration completed without an error. */
    MLCMSW_SMMM_LA_CONFIG_SUCCESS_DETECTIONS = 14,

    /** @brief posix_devctl returned an error during SMMM LA driver configuration. */
    MLCMSW_SMMM_LA_CONFIG_FAIL_DETECTIONS = 15,

    /** @brief SMMM LA driver configuration is not required according to MLCMSW configuration data file. */
    MLCMSW_NO_SMMM_LA_CONFIG_DETECTIONS = 16,

    /** @brief MLCMSW configuration failed. */
    MLCMSW_GET_CONFIGURATION_FAIL_DETECTIONS = 17,

    /** @brief TS connections initialization failed. */
    MLCMSW_CONNECTIONS_INIT_FAIL_DETECTIONS = 18,

    /** @brief FACE_TSS_Base_Create_Connection API returned an error for connection: c_src_cmsw_ml_report_status. */
    MLCMSW_STATUS_CONN_CREATE_FAIL_DETECTIONS = 19,

    /** @brief FACE_TSS_Base_Create_Connection API returned an error for connection: c_dst_cmsw_ml_rcv_config_0. */
    MLCMSW_RCV0_CONN_CREATE_FAIL_DETECTIONS = 20,

    /** @brief FACE_TSS_Base_Create_Connection API returned an error for connection: c_dst_cmsw_ml_rcv_config_1. */
    MLCMSW_RCV1_CONN_CREATE_FAIL_DETECTIONS = 21,

    /** @brief Initialization of PCS SAL has failed. */
    MLCMSW_PCS_SAL_INIT_FAIL_DETECTIONS = 22,

    /** @brief MLCMSW periodic process creation has failed. */
    MLCMSW_CREATE_PROC_FAIL_DETECTIONS = 23,

    /** @brief MLCMSW has not initialized yet. (call @ref mlcmsw_init()) */
    MLCMSW_START_NOT_INIT_DETECTIONS = 24,

    /** @brief MLCMSW periodic process START call failed. */
    MLCMSW_START_PROC_START_FAIL_DETECTIONS = 25,

    /** @brief a664PTPSyncHRTime API returned an error for perioidic synch tod. */
    MLCMSW_PTP_SYNCH_HR_TIME_ERR_DETECTIONS = 26,

    /** @brief gzisu_clock_core_hr_time_get API returned an error while getting timestamp. */
    MLCMSW_PROC_TIME_GET_ERR_DETECTIONS = 27,

    /** @brief Retrieved request indicates A653 Set Schedule operation. */
    MLCMSW_SET_A653_SCH_REQ_INFO_DETECTIONS = 28,

    /** @brief Retrieved request indicates A653 Major Frame Alignment with Set Schedule operation. */
    MLCMSW_SET_ALIGN_WITH_SCH_REQ_INFO_DETECTIONS = 29,

    /** @brief Retrieved request indicates A653 Major Frame Alignment operation. */
    MLCMSW_SET_ALIGN_A653_MF_REQ_INFO_DETECTIONS = 30,

    /** @brief SET_MODULE_SCHEDULE API returned an error while schedule set. */
    MLCMSW_SET_SCH_FAIL_DETECTIONS = 31,

    /** @brief A653 schedule has set without an error. */
    MLCMSW_SET_SCH_SUCCESS_DETECTIONS = 32,

    /** @brief Major frame synchronization completed without an error. */
    MLCMSW_PCS_MF_SYNC_SUCC_DETECTIONS = 33,

    /** @brief pcs_start_major_frame_sync API returned an error while major frame synchronization. */
    MLCMSW_PCS_MF_SYNC_FAIL_DETECTIONS = 34,

    /** @brief PTP is out of synch according to MLCMSW status report message. */
    MLCMSW_PTP_OUT_OF_SYNC_DETECTIONS = 35,

    /** @brief a664PTPSyncHRTime API returned an error while Major Frame synchronization. */
    MLCMSW_MF_REQ_PTP_SYNC_FAIL_DETECTIONS = 36,

    /** @brief Retrieved request is not defined in @ref Mlcmsw_request_type_enum. */
    MLCMSW_INVALID_REQ_DETECTIONS = 37,

    /** @brief a664ESMonitoring API returned monitoring data as NULL. */
    MLCMSW_DTN_MON_DATA_NULL_DETECTIONS = 38,

    /** @brief a664ESMonitoring API returned an error. */
    MLCMSW_DTN_MON_DATA_INVALID_DETECTIONS = 39,

    /** @brief PTP has synched on port A. */
    MLCMSW_SLAVE_A_PTP_SYNC_VALID_DETECTIONS = 40,

    /** @brief PTP has synched on port B. */
    MLCMSW_SLAVE_B_PTP_SYNC_VALID_DETECTIONS = 41,

    /** @brief tss_send_message API returned an error while sending MLCMSW status report message. */
    MLCMSW_SEND_STATUS_MSG_FAIL_DETECTIONS = 42,

    /** @brief SEND_BUFFER returned an error while logging MLCMSW status report message. */
    MLCMSW_LOG_STATUS_MSG_FAIL_DETECTIONS = 43,

    /** @brief MLCMSW status report message payload is bigger than max log payload. */
    MLCMSW_STATUS_LOG_BUFFER_OVERFLOW_DETECTIONS = 44,

    /** @brief pcs_read_hm_stats API returned an error. Failed to retrieve HM statistics from PCS. */
    MLCMSW_PCS_READ_HM_STAT_FAIL_DETECTIONS = 45,

    /** @brief pcs_read_hm_logs API returned an error. Failed to retrieve specific HM log content from PCS.*/
    MLCMSW_PCS_READ_HM_LOG_FAIL_DETECTIONS = 46,

    /** @brief For HM log, log payload exceeds the allocated buffer size. */
    MLCMSW_HM_LOG_BUFFER_OVERFLOW_DETECTIONS = 47,

    /** @brief SEND_BUFFER API returned an error. Failed to dispatch the report via the CMSW logging service. */
    MLCMSW_LOG_HM_LOG_FAIL_DETECTIONS = 48,

    /** @brief Indicates that HM log exists and logged to the file system without an error. */
    MLCMSW_HM_LOG_EXISTS_DETECTIONS = 49,

    /** @brief Buffer that has tA664ESMonitoring type is NULL. (While logging tA664ESMonitoring data)*/
    MLCMSW_DTN_MON_DATA_NULL_2_DETECTIONS = 50,

    /** @brief DTN monitoring data log payload is bigger than max log payload. */
    MLCMSW_DTN_MON_DATA_BUFFER_FULL_DETECTIONS = 51,

    /** @brief SEND_BUFFER API returned an error while logging tA664ESMonitoring data. */
    MLCMSW_LOG_DTN_MON_DATA_FAIL_DETECTIONS = 52,

    /** @brief a664ESMonitoring API returned an error while DTN ES configuration. */
    MLCMSW_INITIAL_DTN_MON_FAIL_DETECTIONS = 53,

    /** @brief DTN ES has already configured. */
    MLCMSW_DTN_ES_ALREADY_DONE_DETECTIONS = 54,

    /** @brief a664ESMonitoring returned tA664ESMonitoring as NULL while DTN ES configuration. */
    MLCMSW_INITIAL_DTN_MON_BUFFER_NULL_DETECTIONS = 55,

    /** @brief pcs_ipmc_set_watchdog_timeout API returned error while set watchdog timer. */
    MLCMSW_WATCHDOG_SET_TIMEOUT_FAIL_DETECTIONS = 56,

    /** @brief pcs_ipmc_set_watchdog_timeout API success. Watchdog timer has set. */
    MLCMSW_WATCHDOG_SET_TIMEOUT_SUCCESS_DETECTIONS = 57,

    /** @brief pcs_ipmc_send_heartbeat API returned an error. */
    MLCMSW_WATCHDOG_KICK_FAIL_DETECTIONS = 58,

    /** @brief Watcdog timer timeout value is bigger than the freeze for value of MF synchronization rendezvous time. */
    MLCMSW_MF_SYNCH_TIME_EXCEED_WD_DETECTIONS = 59,

    /** @brief Watchdog is in freeze mode. */
    MLCMSW_WATCHDOG_FREEZE_MODE_INFO_DETECTIONS = 60,

    /** @brief Watchdog is in run mode. */
    MLCMSW_WATCHDOG_RUN_MODE_INFO_DETECTIONS = 61,

    /** @brief Failed to get high-resolution time for MF synchronization. */
    MLCMSW_MF_SYNCH_HR_TIME_GET_FAIL_DETECTIONS = 62,

    /** @brief Geographical address retrieved and validated. */
    MLCMSW_DEVICE_ID_OKAY_DETECTIONS = 63,

    /** @brief Failed to initialize MLCMSW status report message. */
    MLCMSW_INIT_STATUS_REPORT_FAIL_DETECTIONS = 64,

    /** @brief Failed to set watchdog timeout during MLCMSW initialization. */
    MLCMSW_INIT_WD_SET_TIMEOUT_FAIL_DETECTIONS = 65,

    /** @brief Successfully set MLCMSW initialization watchdog timeout. */
    MLCMSW_INIT_WD_SET_TIMEOUT_SUCC_DETECTIONS = 66,

    /** @brief pcs_import_config API returned an error. Failed to import SMMM LA configuration. */
    MLCMSW_SMMM_LA_IMPORT_CFG_FAIL_DETECTIONS = 67,

    /** @brief pcs_get_config_file_size API returned an error. Failed to get SMMM LA configuration size. */
    MLCMSW_SMMM_LA_GET_CFG_SIZE_FAIL_DETECTIONS = 68,

    /** @brief Failed to generate SMMM LA file name. */
    MLCMSW_SMMM_LA_FILE_NAME_FAIL_DETECTIONS = 69,

    /** @brief pcs_import_config API returned an error while loading config file. */
    MLCMSW_LOAD_CFG_IMPORT_FAIL_DETECTIONS = 70,

    /** @brief pcs_get_config_file_size API returned an error while loading config file. */
    MLCMSW_LOAD_CFG_FILE_SIZE_FAIL_DETECTIONS = 71,

    /** @brief Failed to generate file name during load config file. */
    MLCMSW_LOAD_CFG_FILE_NAME_FAIL_DETECTIONS = 72,

    /** @brief Malloc returned buffer NULL. */
    MLCMSW_LOAD_CFG_BUFFER_NO_MEMORY_DETECTIONS = 73,

    /** @brief Failed to initialize geographical address. */
    MLCMSW_INIT_GEO_ADDR_FAIL_DETECTIONS = 74,

    /** @brief Failed to get mount point. */
    MLCMSW_GET_MOUNT_POINT_FAIL_DETECTIONS = 75,

    /** @brief DFS client mount success. */
    MLCMSW_DFS_CLIENT_MOUNT_SUCC_DETECTIONS = 76,

    /** @brief gzisu_fs_client_mount API returned an error. DFS client mount failure. */
    MLCMSW_DFS_CLIENT_MOUNT_FAIL_DETECTIONS = 77,

    /** @brief Set watchdog success. */
    MLCMSW_SET_WATCHDOG_SUCC_DETECTIONS = 78,

    /** @brief Set watchdog failure. */
    MLCMSW_SET_WATCHDOG_FAIL_DETECTIONS = 79,

    /** @brief pcs_read_module_info API returned an error. Failed to get monolith ID. */
    MLCMSW_GET_MONOLITH_ID_FAIL_DETECTIONS = 80,

    /** @brief Watchdog timeout value is below minimum (200 ms). */
    MLCMSW_WATCHDOG_TIMEOUT_BELOW_MIN_DETECTIONS = 81,

    /** @brief Watchdog timeout value is not valid. */
    MLCMSW_WATCHDOG_TIMEOUT_NOT_VALID_DETECTIONS = 82,

    /** @brief ARINC653 START operation success for MLCMSW periodic process. */
    MLCMSW_START_SUCCESS_DETECTIONS = 83,

    /** @brief MLCMSW load configuration file failure. */
    MLCMSW_IMPORT_CFG_FAIL_DETECTIONS = 84,

    /** @brief MLCMSW configuration version mismatch. */
    MLCMSW_ML_CFG_VERS_MISMATCH_DETECTIONS = 85,

    /** @brief is_config_smmmla field of MLCMSW configuration file is out of bounds. */
    MLCMSW_SM_CFG_BOUND_CHECK_FAIL_DETECTIONS = 86,

    /** @brief watchdog_timeout_scale field of MLCMSW configuration file is out of bounds. */
    MLCMSW_WD_TIM_SCALE_BOUND_CHECK_FAIL_DETECTIONS = 87,

    /** @brief watchdog_timeout_factor field of MLCMSW configuration file is out of bounds. */
    MLCMSW_WD_TIM_FACTOR_BOUND_CHECK_FAIL_DETECTIONS = 88,

    /** @brief watchdog_timeout_unit field of MLCMSW configuration file is out of bounds. */
    MLCMSW_WD_TIM_UNIT_BOUND_CHECK_FAIL_DETECTIONS = 89,

    /** @brief watchdog_mode field of MLCMSW configuration file is out of bounds. */
    MLCMSW_WD_TIM_MODE_BOUND_CHECK_FAIL_DETECTIONS = 90,

    /** @brief MF synchronization integer overflow. */
    MLCMSW_MF_SYNCH_INT_OVERFLOW_DETECTIONS = 91,

    /** @brief SMMM LA driver configuration has failed. */
    MLCMSW_REQ_DRIVER_CFG_FAIL_DETECTIONS = 92,

    /** @brief MLCMSW initialization service success. */
    MLCMSW_INIT_SUCCESS_DETECTIONS = 93,

    /** @brief No DFS client mount available. */
    MLCMSW_NO_DFS_CLIENT_MOUNT_DETECTIONS = 94,

    /** @brief World start time bigger than rendezvous time. */
    MLCMSW_START_BIGGER_THAN_RENDEZVOUS_DETECTIONS = 95,

    /** @brief Partition period is zero. */
    MLCMSW_PARTITION_PERIOD_ZERO_DETECTIONS = 96,

    /** @brief MLCMSW config file size does not match with @ref Mlcmsw_config_type size. */
    MLCMSW_CFG_SIZE_MISMATCH_DETECTIONS = 97
} Mlcmsw_log_type;


/**
 * @brief String array mapping Mlcmsw_log_type enum values to their string representations.
 * 
 * @note This array must be kept in sync with the Mlcmsw_log_type enumeration.
 */
const char *Mlcmsw_log_type_strings[] = {
    "MLCMSW_INIT_FAIL_DETECTIONS",                  // 0
    "MLCMSW_ALREADY_INIT_DETECTIONS",              // 1
    "MLCMSW_TS_IMPORT_CFG_FAIL_DETECTIONS",         // 2
    "MLCMSW_TS_INIT_FAIL_DETECTIONS",               // 3
    "MLCMSW_PCS_HAL_INIT_FAIL_DETECTIONS",           // 4
    "MLCMSW_DTN_INIT_CFG_FAIL_DETECTIONS",          // 5
    "MLCMSW_PCS_GET_GEO_ADDR_FAIL_DETECTIONS",      // 6
    "MLCMSW_GEO_ADDR_PARITY_ERROR_DETECTIONS",      // 7
    "MLCMSW_LRU_ID_FAIL_DETECTIONS",                // 8
    "MLCMSW_SLOT_ID_FAIL_DETECTIONS",              // 9
    "MLCMSW_DTN_SET_DEV_ID_FAIL_DETECTIONS",       // 10
    "MLCMSW_DTN_IMPORT_CFG_FAIL_DETECTIONS",        // 11
    "MLCMSW_DTN_PDI_CFG_FAIL_DETECTIONS",          // 12
    "MLCMSW_SMMM_LA_OPEN_FAIL_DETECTIONS",          // 13
    "MLCMSW_SMMM_LA_CONFIG_SUCCESS_DETECTIONS",     // 14
    "MLCMSW_SMMM_LA_CONFIG_FAIL_DETECTIONS",       // 15
    "MLCMSW_NO_SMMM_LA_CONFIG_DETECTIONS",          // 16
    "MLCMSW_GET_CONFIGURATION_FAIL_DETECTIONS",   // 17
    "MLCMSW_CONNECTIONS_INIT_FAIL_DETECTIONS",      // 18
    "MLCMSW_STATUS_CONN_CREATE_FAIL_DETECTIONS",   // 19
    "MLCMSW_RCV0_CONN_CREATE_FAIL_DETECTIONS",      // 20
    "MLCMSW_RCV1_CONN_CREATE_FAIL_DETECTIONS",      // 21
    "MLCMSW_PCS_SAL_INIT_FAIL_DETECTIONS",          // 22
    "MLCMSW_CREATE_PROC_FAIL_DETECTIONS",           // 23
    "MLCMSW_START_NOT_INIT_DETECTIONS",             // 24
    "MLCMSW_START_PROC_START_FAIL_DETECTIONS",       // 25
    "MLCMSW_PTP_SYNCH_HR_TIME_ERR_DETECTIONS",      // 26
    "MLCMSW_PROC_TIME_GET_ERR_DETECTIONS",          // 27
    "MLCMSW_SET_A653_SCH_REQ_INFO_DETECTIONS",      // 28
    "MLCMSW_SET_ALIGN_WITH_SCH_REQ_INFO_DETECTIONS",// 29
    "MLCMSW_SET_ALIGN_A653_MF_REQ_INFO_DETECTIONS",// 30
    "MLCMSW_SET_SCH_FAIL_DETECTIONS",               // 31
    "MLCMSW_SET_SCH_SUCCESS_DETECTIONS",             // 32
    "MLCMSW_PCS_MF_SYNC_SUCC_DETECTIONS",           // 33
    "MLCMSW_PCS_MF_SYNC_FAIL_DETECTIONS",            // 34
    "MLCMSW_PTP_OUT_OF_SYNC_DETECTIONS",            // 35
    "MLCMSW_MF_REQ_PTP_SYNC_FAIL_DETECTIONS",       // 36
    "MLCMSW_INVALID_REQ_DETECTIONS",                // 37
    "MLCMSW_DTN_MON_DATA_NULL_DETECTIONS",          // 38
    "MLCMSW_DTN_MON_DATA_INVALID_DETECTIONS",      // 39
    "MLCMSW_SLAVE_A_PTP_SYNC_VALID_DETECTIONS",     // 40
    "MLCMSW_SLAVE_B_PTP_SYNC_VALID_DETECTIONS",     // 41
    "MLCMSW_SEND_STATUS_MSG_FAIL_DETECTIONS",       // 42
    "MLCMSW_LOG_STATUS_MSG_FAIL_DETECTIONS",       // 43
    "MLCMSW_STATUS_LOG_BUFFER_OVERFLOW_DETECTIONS", // 44
    "MLCMSW_PCS_READ_HM_STAT_FAIL_DETECTIONS",      // 45
    "MLCMSW_PCS_READ_HM_LOG_FAIL_DETECTIONS",        // 46
    "MLCMSW_HM_LOG_BUFFER_OVERFLOW_DETECTIONS",    // 47
    "MLCMSW_LOG_HM_LOG_FAIL_DETECTIONS",            // 48
    "MLCMSW_HM_LOG_EXISTS_DETECTIONS",              // 49
    "MLCMSW_DTN_MON_DATA_NULL_2_DETECTIONS",        // 50
    "MLCMSW_DTN_MON_DATA_BUFFER_FULL_DETECTIONS",  // 51
    "MLCMSW_LOG_DTN_MON_DATA_FAIL_DETECTIONS",     // 52
    "MLCMSW_INITIAL_DTN_MON_FAIL_DETECTIONS",       // 53
    "MLCMSW_DTN_ES_ALREADY_DONE_DETECTIONS",        // 54
    "MLCMSW_INITIAL_DTN_MON_BUFFER_NULL_DETECTIONS",// 55
    "MLCMSW_WATCHDOG_SET_TIMEOUT_FAIL_DETECTIONS", // 56
    "MLCMSW_WATCHDOG_SET_TIMEOUT_SUCCESS_DETECTIONS", // 57
    "MLCMSW_WATCHDOG_KICK_FAIL_DETECTIONS",         // 58
    "MLCMSW_MF_SYNCH_TIME_EXCEED_WD_DETECTIONS",    // 59
    "MLCMSW_WATCHDOG_FREEZE_MODE_INFO_DETECTIONS", // 60
    "MLCMSW_WATCHDOG_RUN_MODE_INFO_DETECTIONS",    // 61
    "MLCMSW_MF_SYNCH_HR_TIME_GET_FAIL_DETECTIONS", // 62
    "MLCMSW_DEVICE_ID_OKAY_DETECTIONS",             // 63
    "MLCMSW_INIT_STATUS_REPORT_FAIL_DETECTIONS",    // 64
    "MLCMSW_INIT_WD_SET_TIMEOUT_FAIL_DETECTIONS",   // 65
    "MLCMSW_INIT_WD_SET_TIMEOUT_SUCC_DETECTIONS",   // 66
    "MLCMSW_SMMM_LA_IMPORT_CFG_FAIL_DETECTIONS",    // 67
    "MLCMSW_SMMM_LA_GET_CFG_SIZE_FAIL_DETECTIONS",  // 68
    "MLCMSW_SMMM_LA_FILE_NAME_FAIL_DETECTIONS",     // 69
    "MLCMSW_LOAD_CFG_IMPORT_FAIL_DETECTIONS",       // 70
    "MLCMSW_LOAD_CFG_FILE_SIZE_FAIL_DETECTIONS",   // 71
    "MLCMSW_LOAD_CFG_FILE_NAME_FAIL_DETECTIONS",    // 72
    "MLCMSW_LOAD_CFG_BUFFER_NO_MEMORY_DETECTIONS",  // 73
    "MLCMSW_INIT_GEO_ADDR_FAIL_DETECTIONS",         // 74
    "MLCMSW_GET_MOUNT_POINT_FAIL_DETECTIONS",      // 75
    "MLCMSW_DFS_CLIENT_MOUNT_SUCC_DETECTIONS",      // 76
    "MLCMSW_DFS_CLIENT_MOUNT_FAIL_DETECTIONS",      // 77
    "MLCMSW_SET_WATCHDOG_SUCC_DETECTIONS",          // 78
    "MLCMSW_SET_WATCHDOG_FAIL_DETECTIONS",          // 79
    "MLCMSW_GET_MONOLITH_ID_FAIL_DETECTIONS",       // 80
    "MLCMSW_WATCHDOG_TIMEOUT_BELOW_MIN_DETECTIONS",// 81
    "MLCMSW_WATCHDOG_TIMEOUT_NOT_VALID_DETECTIONS",// 82
    "MLCMSW_START_SUCCESS_DETECTIONS",              // 83
    "MLCMSW_IMPORT_CFG_FAIL_DETECTIONS",            // 84
    "MLCMSW_ML_CFG_VERS_MISMATCH_DETECTIONS",      // 85
    "MLCMSW_SM_CFG_BOUND_CHECK_FAIL_DETECTIONS",   // 86
    "MLCMSW_WD_TIM_SCALE_BOUND_CHECK_FAIL_DETECTIONS", // 87
    "MLCMSW_WD_TIM_FACTOR_BOUND_CHECK_FAIL_DETECTIONS", // 88
    "MLCMSW_WD_TIM_UNIT_BOUND_CHECK_FAIL_DETECTIONS", // 89
    "MLCMSW_WD_TIM_MODE_BOUND_CHECK_FAIL_DETECTIONS", // 90
    "MLCMSW_MF_SYNCH_INT_OVERFLOW_DETECTIONS",     // 91
    "MLCMSW_REQ_DRIVER_CFG_FAIL_DETECTIONS",       // 92
    "MLCMSW_INIT_SUCCESS_DETECTIONS",              // 93
    "MLCMSW_NO_DFS_CLIENT_MOUNT_DETECTIONS",        // 94
    "MLCMSW_START_BIGGER_THAN_RENDEZVOUS_DETECTIONS", // 95
    "MLCMSW_PARTITION_PERIOD_ZERO_DETECTIONS",      // 96
    "MLCMSW_CFG_SIZE_MISMATCH_DETECTIONS"           // 97
};

typedef enum
{
    CMSW_EMERGENCY = 0,
    CMSW_ALERT = 1,
    CMSW_CRITICAL = 2,
    CMSW_ERROR = 3,
    CMSW_WARNING = 4,
    CMSW_NOTICE = 5,
    CMSW_INFORMATIONAL = 6,
    CMSW_DEBUG = 7,
} MLCMSW_SEVERITY_TYPE;

typedef struct __attribute__((packed))
{
    Mlcmsw_log_type log_type;
    uint8_t log_severity;
    int64_t first_param;
    int64_t second_param;
} mlcmsw_log_entry_t;

typedef struct __attribute__((packed))
{
    uint64_t log_timestamp;
    uint32_t log_entry_count;
    uint32_t log_drop_count;
} mlcmsw_log_frame_header_t;

typedef struct __attribute__((packed))
{
    mlcmsw_log_frame_header_t log_frame_header;
    mlcmsw_log_entry_t log_entry_list[256];
} MLCMSW_LOG_FRAME_TYPE;


static inline void log_ml_cmsw_status_endian_convert(Cmsw_ml_status_msg_type* data) {
    // BEGIN Cmsw_ml_status_msg_type
    data->device_id = ntohs(data->device_id);
    data->timestamp = ntohll(data->timestamp);
    data->a653_schedule_id = ntohl(data->a653_schedule_id);
    data->dtn_es_cfg_id = ntohl(data->dtn_es_cfg_id);
    data->ptp_cfg_id = ntohs(data->ptp_cfg_id);
    data->monolith_id = ntohs(data->monolith_id);
    // END Cmsw_ml_status_msg_type
}

static inline void log_ml_cmsw_status_header_print(FILE *fp) {
    fprintf(fp, "DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP,LOG PAYLOAD LENGTH,");
    // BEGIN Cmsw_ml_status_msg_type
    fprintf(fp, "msg_type,");
    fprintf(fp, "device_id,");
    fprintf(fp, "padding,");
    fprintf(fp, "timestamp,");
    fprintf(fp, "a653_schedule_id,");
    fprintf(fp, "dtn_es_cfg_id,");
    fprintf(fp, "ptp_cfg_id,");
    fprintf(fp, "ptp_tod_network,");
    fprintf(fp, "ptp_device_type,");
    fprintf(fp, "ptp_synch_status,");
    fprintf(fp, "dtn_es_mon_data_validity,");
    fprintf(fp, "dtn_es_redundancy_status,");
    fprintf(fp, "dtn_es_fw_vers_major,");
    fprintf(fp, "dtn_es_fw_vers_minor,");
    fprintf(fp, "dtn_es_fw_vers_micro,");
    fprintf(fp, "hsn_link_status,");
    fprintf(fp, "major_frame_alignment_status,");
    fprintf(fp, "monolith_id\n");
    // END Cmsw_ml_status_msg_type
}

static inline void log_ml_cmsw_status_data_print(FILE *fp, Cmsw_ml_status_msg_type* data) {
    log_ml_cmsw_status_endian_convert(data);
    // BEGIN Cmsw_ml_status_msg_type
    fprintf(fp, "%u,", data->msg_type);
    fprintf(fp, "%u,", data->device_id);
    fprintf(fp, "%u,", data->padding);
    fprintf(fp, "%llu,", data->timestamp);
    fprintf(fp, "%d,", data->a653_schedule_id);
    fprintf(fp, "%u,", data->dtn_es_cfg_id);
    fprintf(fp, "%u,", data->ptp_cfg_id);
    fprintf(fp, "%u,", data->ptp_tod_network);
    fprintf(fp, "%u,", data->ptp_device_type);
    fprintf(fp, "%u,", data->ptp_synch_status);
    fprintf(fp, "%u,", data->dtn_es_mon_data_validity);
    fprintf(fp, "%u,", data->dtn_es_redundancy_status);
    fprintf(fp, "%u,", data->dtn_es_fw_vers_major);
    fprintf(fp, "%u,", data->dtn_es_fw_vers_minor);
    fprintf(fp, "%u,", data->dtn_es_fw_vers_micro);
    fprintf(fp, "%u,", data->hsn_link_status);
    fprintf(fp, "%u,", data->major_frame_alignment_status);
    fprintf(fp, "%u\n", data->monolith_id);
    // END Cmsw_ml_status_msg_type
}


static inline void log_ml_cmsw_dtn_es_monitoring_endian_convert(tA664ESMonitoring* data) {
    // BEGIN tA664ESMonitoring
    data->A664_ES_FW_VER = ntohll(data->A664_ES_FW_VER);
    data->A664_ES_DEV_ID = ntohll(data->A664_ES_DEV_ID);
    data->A664_ES_MODE = ntohll(data->A664_ES_MODE);
    data->A664_ES_CONFIG_ID = ntohll(data->A664_ES_CONFIG_ID);
    data->A664_ES_BIT_STATUS = ntohll(data->A664_ES_BIT_STATUS);
    data->A664_ES_CONFIG_STATUS = ntohll(data->A664_ES_CONFIG_STATUS);
    data->A664_PTP_CONFIG_ID = ntohs(data->A664_PTP_CONFIG_ID);
    data->A664_PTP_SYNC_VL_ID = ntohs(data->A664_PTP_SYNC_VL_ID);
    data->A664_PTP_REQ_VL_ID = ntohs(data->A664_PTP_REQ_VL_ID);
    data->A664_PTP_RES_VL_ID = ntohs(data->A664_PTP_RES_VL_ID);
    data->A664_ES_HW_TEMP = ntohl(data->A664_ES_HW_TEMP);
    data->A664_ES_HW_VCC_INT = ntohl(data->A664_ES_HW_VCC_INT);
    data->A664_ES_PORT_SPEED = ntohll(data->A664_ES_PORT_SPEED);
    data->A664_ES_PORT_A_STATUS = ntohll(data->A664_ES_PORT_A_STATUS);
    data->A664_ES_PORT_B_STATUS = ntohll(data->A664_ES_PORT_B_STATUS);
    data->A664_ES_TX_INCOMING_COUNT = ntohll(data->A664_ES_TX_INCOMING_COUNT);
    data->A664_ES_TX_A_OUTGOING_COUNT = ntohll(data->A664_ES_TX_A_OUTGOING_COUNT);
    data->A664_ES_TX_B_OUTGOING_COUNT = ntohll(data->A664_ES_TX_B_OUTGOING_COUNT);
    data->A664_ES_TX_VLID_DROP_COUNT = ntohll(data->A664_ES_TX_VLID_DROP_COUNT);
    data->A664_ES_TX_LMIN_LMAX_DROP_COUNT = ntohll(data->A664_ES_TX_LMIN_LMAX_DROP_COUNT);
    data->A664_ES_TX_MAX_JITTER_DROP_COUNT = ntohll(data->A664_ES_TX_MAX_JITTER_DROP_COUNT);
    data->A664_ES_RX_A_INCOMING_COUNT = ntohll(data->A664_ES_RX_A_INCOMING_COUNT);
    data->A664_ES_RX_B_INCOMING_COUNT = ntohll(data->A664_ES_RX_B_INCOMING_COUNT);
    data->A664_ES_RX_OUTGOING_COUNT = ntohll(data->A664_ES_RX_OUTGOING_COUNT);
    data->A664_ES_RX_A_VLID_DROP_COUNT = ntohll(data->A664_ES_RX_A_VLID_DROP_COUNT);
    data->A664_ES_RX_A_LMIN_LMAX_DROP_COUNT = ntohll(data->A664_ES_RX_A_LMIN_LMAX_DROP_COUNT);
    data->A664_ES_RX_A_NET_ERR_COUNT = ntohll(data->A664_ES_RX_A_NET_ERR_COUNT);
    data->A664_ES_RX_A_SEQ_ERR_COUNT = ntohll(data->A664_ES_RX_A_SEQ_ERR_COUNT);
    data->A664_ES_RX_A_CRC_ERROR_COUNT = ntohll(data->A664_ES_RX_A_CRC_ERROR_COUNT);
    data->A664_ES_RX_A_IP_CHECKSUM_ERROR_COUNT = ntohll(data->A664_ES_RX_A_IP_CHECKSUM_ERROR_COUNT);
    data->A664_ES_RX_B_VLID_DROP_COUNT = ntohll(data->A664_ES_RX_B_VLID_DROP_COUNT);
    data->A664_ES_RX_B_LMIN_LMAX_DROP_COUNT = ntohll(data->A664_ES_RX_B_LMIN_LMAX_DROP_COUNT);
    data->A664_ES_RX_B_SEQ_ERR_COUNT = ntohll(data->A664_ES_RX_B_SEQ_ERR_COUNT);
    data->A664_ES_RX_B_NET_ERR_COUNT = ntohll(data->A664_ES_RX_B_NET_ERR_COUNT);
    data->A664_ES_RX_B_CRC_ERROR_COUNT = ntohll(data->A664_ES_RX_B_CRC_ERROR_COUNT);
    data->A664_ES_RX_B_IP_CHECKSUM_ERROR_COUNT = ntohll(data->A664_ES_RX_B_IP_CHECKSUM_ERROR_COUNT);
    data->A664_BSP_TX_PACKET_COUNT = ntohll(data->A664_BSP_TX_PACKET_COUNT);
    data->A664_BSP_TX_BYTE_COUNT = ntohll(data->A664_BSP_TX_BYTE_COUNT);
    data->A664_BSP_TX_ERROR_COUNT = ntohll(data->A664_BSP_TX_ERROR_COUNT);
    data->A664_BSP_RX_PACKET_COUNT = ntohll(data->A664_BSP_RX_PACKET_COUNT);
    data->A664_BSP_RX_BYTE_COUNT = ntohll(data->A664_BSP_RX_BYTE_COUNT);
    data->A664_BSP_RX_ERROR_COUNT = ntohll(data->A664_BSP_RX_ERROR_COUNT);
    data->A664_BSP_RX_MISSED_FRAME_COUNT = ntohll(data->A664_BSP_RX_MISSED_FRAME_COUNT);
    data->A664_BSP_VER = ntohll(data->A664_BSP_VER);
    data->A664_ES_VENDOR_TYPE = ntohll(data->A664_ES_VENDOR_TYPE);
    data->A664_ES_BSP_QUEUING_RX_VL_PORT_DROP_COUNT = ntohll(data->A664_ES_BSP_QUEUING_RX_VL_PORT_DROP_COUNT);
    data->A664_SW_ES_ENABLE = ntohl(data->A664_SW_ES_ENABLE);
    // END tA664ESMonitoring
}

static inline void log_ml_cmsw_dtn_es_monitoring_header_print(FILE *fp) {
    fprintf(fp, "DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP,LOG PAYLOAD LENGTH,");
    // BEGIN tA664ESMonitoring
    fprintf(fp, "A664_ES_FW_VER,");
    fprintf(fp, "A664_ES_DEV_ID,");
    fprintf(fp, "A664_ES_MODE,");
    fprintf(fp, "A664_ES_CONFIG_ID,");
    fprintf(fp, "A664_ES_BIT_STATUS,");
    fprintf(fp, "A664_ES_CONFIG_STATUS,");
    fprintf(fp, "A664_BSP_CONFIG_STATUS,");
    fprintf(fp, "A664_PTP_CONFIG_ID,");
    fprintf(fp, "A664_PTP_DEVICE_TYPE,");
    fprintf(fp, "A664_PTP_RC_STATUS,");
    fprintf(fp, "A664_PTP_PORT_A_SYNC,");
    fprintf(fp, "A664_PTP_PORT_B_SYNC,");
    fprintf(fp, "A664_PTP_SYNC_VL_ID,");
    fprintf(fp, "A664_PTP_REQ_VL_ID,");
    fprintf(fp, "A664_PTP_RES_VL_ID,");
    fprintf(fp, "A664_PTP_TOD_NETWORK,");
    fprintf(fp, "A664_ES_HW_TEMP,");
    fprintf(fp, "A664_ES_HW_VCC_INT,");
    fprintf(fp, "A664_ES_PORT_SPEED,");
    fprintf(fp, "A664_ES_PORT_A_STATUS,");
    fprintf(fp, "A664_ES_PORT_B_STATUS,");
    fprintf(fp, "A664_ES_TX_INCOMING_COUNT,");
    fprintf(fp, "A664_ES_TX_A_OUTGOING_COUNT,");
    fprintf(fp, "A664_ES_TX_B_OUTGOING_COUNT,");
    fprintf(fp, "A664_ES_TX_VLID_DROP_COUNT,");
    fprintf(fp, "A664_ES_TX_LMIN_LMAX_DROP_COUNT,");
    fprintf(fp, "A664_ES_TX_MAX_JITTER_DROP_COUNT,");
    fprintf(fp, "A664_ES_RX_A_INCOMING_COUNT,");
    fprintf(fp, "A664_ES_RX_B_INCOMING_COUNT,");
    fprintf(fp, "A664_ES_RX_OUTGOING_COUNT,");
    fprintf(fp, "A664_ES_RX_A_VLID_DROP_COUNT,");
    fprintf(fp, "A664_ES_RX_A_LMIN_LMAX_DROP_COUNT,");
    fprintf(fp, "A664_ES_RX_A_NET_ERR_COUNT,");
    fprintf(fp, "A664_ES_RX_A_SEQ_ERR_COUNT,");
    fprintf(fp, "A664_ES_RX_A_CRC_ERROR_COUNT,");
    fprintf(fp, "A664_ES_RX_A_IP_CHECKSUM_ERROR_COUNT,");
    fprintf(fp, "A664_ES_RX_B_VLID_DROP_COUNT,");
    fprintf(fp, "A664_ES_RX_B_LMIN_LMAX_DROP_COUNT,");
    fprintf(fp, "A664_ES_RX_B_SEQ_ERR_COUNT,");
    fprintf(fp, "A664_ES_RX_B_NET_ERR_COUNT,");
    fprintf(fp, "A664_ES_RX_B_CRC_ERROR_COUNT,");
    fprintf(fp, "A664_ES_RX_B_IP_CHECKSUM_ERROR_COUNT,");
    fprintf(fp, "A664_BSP_TX_PACKET_COUNT,");
    fprintf(fp, "A664_BSP_TX_BYTE_COUNT,");
    fprintf(fp, "A664_BSP_TX_ERROR_COUNT,");
    fprintf(fp, "A664_BSP_RX_PACKET_COUNT,");
    fprintf(fp, "A664_BSP_RX_BYTE_COUNT,");
    fprintf(fp, "A664_BSP_RX_ERROR_COUNT,");
    fprintf(fp, "A664_BSP_RX_MISSED_FRAME_COUNT,");
    fprintf(fp, "A664_BSP_VER,");
    fprintf(fp, "A664_ES_VENDOR_TYPE,");
    fprintf(fp, "A664_ES_BSP_QUEUING_RX_VL_PORT_DROP_COUNT,");
    fprintf(fp, "A664_SW_ES_ENABLE\n");
    // END tA664ESMonitoring
}

static inline void log_ml_cmsw_dtn_es_monitoring_data_print(FILE *fp, tA664ESMonitoring* data) {

    log_ml_cmsw_dtn_es_monitoring_endian_convert(data);

    // BEGIN tA664ESMonitoring

    uint64_t vers_major = (data->A664_ES_FW_VER >> 16 ) & 0xFF;
	uint64_t vers_minor = (data->A664_ES_FW_VER >> 8) & 0xFF;
	uint64_t vers_bugfix = data->A664_ES_FW_VER & 0xFF;

    fprintf(fp, "%u.%u.%u,", vers_major, vers_minor, vers_bugfix); //fprintf(fp, "%llu,", data->A664_ES_FW_VER);

    uint64_t dev_id_part1 = (data->A664_ES_DEV_ID >> 8) & 0xFF;
	uint64_t dev_id_part2 = (data->A664_ES_DEV_ID >> 3) & 0x1F;

    fprintf(fp, "%u-%u,", dev_id_part1, dev_id_part2); //fprintf(fp, "%llu,", data->A664_ES_DEV_ID);

    fprintf(fp, "%llu,", data->A664_ES_MODE);
    fprintf(fp, "%llu,", data->A664_ES_CONFIG_ID);
    fprintf(fp, "%llu,", data->A664_ES_BIT_STATUS);
    fprintf(fp, "%llu,", data->A664_ES_CONFIG_STATUS);
    fprintf(fp, "%u,", data->A664_BSP_CONFIG_STATUS);
    fprintf(fp, "%u,", data->A664_PTP_CONFIG_ID);
    fprintf(fp, "%u,", data->A664_PTP_DEVICE_TYPE);
    fprintf(fp, "%u,", data->A664_PTP_RC_STATUS);
    fprintf(fp, "%u,", data->A664_PTP_PORT_A_SYNC);
    fprintf(fp, "%u,", data->A664_PTP_PORT_B_SYNC);
    fprintf(fp, "%u,", data->A664_PTP_SYNC_VL_ID);
    fprintf(fp, "%u,", data->A664_PTP_REQ_VL_ID);
    fprintf(fp, "%u,", data->A664_PTP_RES_VL_ID);
    fprintf(fp, "%u,", data->A664_PTP_TOD_NETWORK);
    fprintf(fp, "%u,", data->A664_ES_HW_TEMP);
    fprintf(fp, "%u,", data->A664_ES_HW_VCC_INT);
    fprintf(fp, "%llu,", data->A664_ES_PORT_SPEED);
    fprintf(fp, "%llu,", data->A664_ES_PORT_A_STATUS);
    fprintf(fp, "%llu,", data->A664_ES_PORT_B_STATUS);
    fprintf(fp, "%llu,", data->A664_ES_TX_INCOMING_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_TX_A_OUTGOING_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_TX_B_OUTGOING_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_TX_VLID_DROP_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_TX_LMIN_LMAX_DROP_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_TX_MAX_JITTER_DROP_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_RX_A_INCOMING_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_RX_B_INCOMING_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_RX_OUTGOING_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_RX_A_VLID_DROP_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_RX_A_LMIN_LMAX_DROP_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_RX_A_NET_ERR_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_RX_A_SEQ_ERR_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_RX_A_CRC_ERROR_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_RX_A_IP_CHECKSUM_ERROR_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_RX_B_VLID_DROP_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_RX_B_LMIN_LMAX_DROP_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_RX_B_SEQ_ERR_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_RX_B_NET_ERR_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_RX_B_CRC_ERROR_COUNT);
    fprintf(fp, "%llu,", data->A664_ES_RX_B_IP_CHECKSUM_ERROR_COUNT);
    fprintf(fp, "%llu,", data->A664_BSP_TX_PACKET_COUNT);
    fprintf(fp, "%llu,", data->A664_BSP_TX_BYTE_COUNT);
    fprintf(fp, "%llu,", data->A664_BSP_TX_ERROR_COUNT);
    fprintf(fp, "%llu,", data->A664_BSP_RX_PACKET_COUNT);
    fprintf(fp, "%llu,", data->A664_BSP_RX_BYTE_COUNT);
    fprintf(fp, "%llu,", data->A664_BSP_RX_ERROR_COUNT);
    fprintf(fp, "%llu,", data->A664_BSP_RX_MISSED_FRAME_COUNT);

    uint64_t bsp_ver_part1 = (data->A664_BSP_VER >> 8) & 0xF;
    uint64_t bsp_ver_part2 = (data->A664_BSP_VER >> 4) & 0xF;
    uint64_t bsp_ver_part3 = data->A664_BSP_VER & 0xF;

    fprintf(fp, "%u.%u.%u,", bsp_ver_part1, bsp_ver_part2, bsp_ver_part3); //fprintf(fp, "%llu,", data->A664_BSP_VER);

    fprintf(fp, "%llu,", data->A664_ES_VENDOR_TYPE);
    fprintf(fp, "%llu,", data->A664_ES_BSP_QUEUING_RX_VL_PORT_DROP_COUNT);
    fprintf(fp, "%u\n", data->A664_SW_ES_ENABLE);
    // END tA664ESMonitoring
}


static inline void log_mlcmsw_major_sync_error_endian_convert(Pcs_hm_stats_info* data) {
    // BEGIN Pcs_hm_stats_info
    data->nb_of_unread_logs = ntohll(data->nb_of_unread_logs);
    data->nb_of_overwritten_logs = ntohll(data->nb_of_overwritten_logs);
    data->nb_of_total_hm_logs = ntohll(data->nb_of_total_hm_logs);
    data->nb_of_missed_major_frames = ntohll(data->nb_of_missed_major_frames);
    // END Pcs_hm_stats_info
}

static inline void log_mlcmsw_major_sync_error_header_print(FILE *fp) {
    fprintf(fp, "DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP,LOG PAYLOAD LENGTH,");
    // BEGIN Pcs_hm_stats_info
    fprintf(fp, "nb_of_unread_logs,");
    fprintf(fp, "nb_of_overwritten_logs,");
    fprintf(fp, "nb_of_total_hm_logs,");
    fprintf(fp, "nb_of_missed_major_frames\n");
    // END Pcs_hm_stats_info
}

static inline void log_mlcmsw_major_sync_error_data_print(FILE *fp, Pcs_hm_stats_info* data) {
    log_mlcmsw_major_sync_error_endian_convert(data);
    // BEGIN Pcs_hm_stats_info
    fprintf(fp, "%llu,", data->nb_of_unread_logs);
    fprintf(fp, "%llu,", data->nb_of_overwritten_logs);
    fprintf(fp, "%llu,", data->nb_of_total_hm_logs);
    fprintf(fp, "%llu\n", data->nb_of_missed_major_frames);
    // END Pcs_hm_stats_info
}


static int mlcmsw_detections_parser(FILE *outfp, MLCMSW_LOG_FRAME_TYPE **frame)
{
    MLCMSW_LOG_FRAME_TYPE *log_frame = *frame;

    uint64_t seconds;
    uint64_t nanoseconds;

    log_frame->log_frame_header.log_timestamp = ntohll(log_frame->log_frame_header.log_timestamp);
    log_frame->log_frame_header.log_entry_count = ntohl(log_frame->log_frame_header.log_entry_count);
    log_frame->log_frame_header.log_drop_count = ntohl(log_frame->log_frame_header.log_drop_count);

    for(int i = 0; i < log_frame->log_frame_header.log_entry_count; i++)
    {
        log_frame->log_entry_list[i].log_type     = ntohl(log_frame->log_entry_list[i].log_type);
        log_frame->log_entry_list[i].log_severity = ntohl(log_frame->log_entry_list[i].log_severity);
        log_frame->log_entry_list[i].first_param  = ntohll(log_frame->log_entry_list[i].first_param);
        log_frame->log_entry_list[i].second_param = ntohll(log_frame->log_entry_list[i].second_param);
    }

    if(0 == log_frame->log_frame_header.log_entry_count)
    {
        seconds = log_frame->log_frame_header.log_timestamp / 1000000000;
        nanoseconds = log_frame->log_frame_header.log_timestamp % 1000000000;
        fprintf(outfp, "%llu sec,%09llu nsec,", seconds, nanoseconds);
        fprintf(outfp, "%u,", log_frame->log_frame_header.log_entry_count);
        fprintf(outfp, "%u,", log_frame->log_frame_header.log_drop_count);

        fprintf(outfp, "N/A,", "");
        fprintf(outfp, "N/A,", "");
        fprintf(outfp, "N/A,", "");
        fprintf(outfp, "N/A\n", "");
    }
    else
    {
        for(int i = 0; i < log_frame->log_frame_header.log_entry_count; i++)
        {
            seconds = log_frame->log_frame_header.log_timestamp / 1000000000;
            nanoseconds = log_frame->log_frame_header.log_timestamp % 1000000000;
            fprintf(outfp, "%llu sec,%09llu nsec,", seconds, nanoseconds);
            fprintf(outfp, "%u,", log_frame->log_frame_header.log_entry_count);
            fprintf(outfp, "%u,", log_frame->log_frame_header.log_drop_count);

            fprintf(outfp, "%s,", log_frame->log_entry_list[i].log_type >= MLCMSW_LOGTYPE_MAX ? Mlcmsw_log_type_strings[MLCMSW_LOGTYPE_MAX] : Mlcmsw_log_type_strings[log_frame->log_entry_list[i].log_type]);
            fprintf(outfp, "%u,", log_frame->log_entry_list[i].log_severity);
            //fprintf(outfp, "%s,", log_frame->log_entry_list[i].log_severity >= CMSW_SEVERITY_MAX ? severity_strings[CMSW_SEVERITY_MAX] : severity_strings[log_frame->log_entry_list[i].log_severity]);
            fprintf(outfp, "%lld,", log_frame->log_entry_list[i].first_param);
            fprintf(outfp, "%lld\n", log_frame->log_entry_list[i].second_param);
        }
    }

    return 0;
}


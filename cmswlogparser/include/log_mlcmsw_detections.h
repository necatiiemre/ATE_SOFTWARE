#pragma once

#if 0

/**
 * @file     mlcmsw_logger.h
 *
 * @brief   Provides the logging interface for the MLCMSW module.
 *
 * @details This header file contains:
 *          - Logging severity and type enumerations
 *          - Log file header and frame structures
 *          - Log context and data structures
 *          - Logging initialization and reporting functions
 *
 * @note    This interface supports both internal MLCMSW logs and application-level logs.
 *
 * @sa      mlcmsw_logger.c
 * 
 * @author   fahri.nebioglu 2026
 */

#ifndef MLCMSW_LOGGER_H
#define MLCMSW_LOGGER_H

/**********************************************************************************************************************
*                                                       Includes                                                      *
**********************************************************************************************************************/

#include "../inc/mlcmsw_config_types.h"

#include "mlcmsw_types.h"

#include "stdint.h"
#include "fs/fileSystem.h"

#include "arinc653/ARINC653.h"

/**********************************************************************************************************************
*                                                   Type Definitions                                                  *
**********************************************************************************************************************/

/**
 * @enum Mlcmsw_severity_type
 * 
 * @brief Enumeration defining the severity levels for log messages.
 * 
 * @details This enumeration is used to categorize the importance or urgency of a log entry, ranging from emergency to
 *          debug.
 */
typedef enum Mlcmsw_severity_type
{
    /** Highest priority: System is unusable. */
    CMSW_EMERGENCY = 0,

    /** Action must be taken immediately. */
    CMSW_ALERT = 1,

    /** Critical conditions that require immediate attention. */
    CMSW_CRITICAL = 2,

    /** Error conditions that may require investigation. */
    CMSW_ERROR = 3,

    /** Warning conditions that may lead to errors. */
    CMSW_WARNING = 4,

    /** Normal but significant operational events. */
    CMSW_NOTICE = 5,

    /** General operational information. */
    CMSW_INFORMATIONAL = 6,

    /** Detailed information for debugging purposes. */
    CMSW_DEBUG = 7,
} Mlcmsw_severity_type;

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
    MLCMSW_INIT_FAIL = 0,

    /** @brief MLCMSW already initialized. */
    MLCMSW_ALREADY_INIT = 1,

    /** @brief Failed to get TS configuration file. */
    MLCMSW_TS_IMPORT_CFG_FAIL = 2,

    /** @brief TS initialization has failed.*/
    MLCMSW_TS_INIT_FAIL = 3,

    /** @brief PCS HAL initialization has failed.*/
    MLCMSW_PCS_HAL_INIT_FAIL = 4,

    /** @brief Configuration of DTN has failed.*/
    MLCMSW_DTN_INIT_CFG_FAIL = 5,

    /** @brief pcs_get_geographical_address API returned an error. */
    MLCMSW_PCS_GET_GEO_ADDR_FAIL = 6,

    /** @brief pcs_get_geographical_address returned @ref PARITY_ERROR. */
    MLCMSW_GEO_ADDR_PARITY_ERROR = 7,

    /** @brief pcs_get_geographical_address returned LRU ID that in out of bounds. (shall be in [0-127])*/
    MLCMSW_LRU_ID_FAIL = 8,

    /** @brief pcs_get_geographical_address returned Backplane Slot ID that in out of bounds. (shall be in [1-17])*/
    MLCMSW_SLOT_ID_FAIL = 9,

    /** @brief a664ESSetDeviceID failed while retrieving geographical address. */
    MLCMSW_DTN_SET_DEV_ID_FAIL = 10,

    /** @brief Fail during loading DTN ES configuration file. */
    MLCMSW_DTN_IMPORT_CFG_FAIL = 11,

    /** @brief a664ESPDIConfig API returned an error. */
    MLCMSW_DTN_PDI_CFG_FAIL = 12,

    /** @brief open() call failed during SMMM LA driver configuration. */
    MLCMSW_SMMM_LA_OPEN_FAIL = 13,

    /** @brief SMMM LA driver configuration completed without an error. */
    MLCMSW_SMMM_LA_CONFIG_SUCCESS = 14,

    /** @brief posix_devctl returned an error during SMMM LA driver configuration. */
    MLCMSW_SMMM_LA_CONFIG_FAIL = 15,

    /** @brief SMMM LA driver configuration is not required according to MLCMSW configuration data file. */
    MLCMSW_NO_SMMM_LA_CONFIG = 16,

    /** @brief MLCMSW configuration failed. */
    MLCMSW_GET_CONFIGURATION_FAIL = 17,

    /** @brief TS connections initialization failed. */
    MLCMSW_CONNECTIONS_INIT_FAIL = 18,

    /** @brief FACE_TSS_Base_Create_Connection API returned an error for connection: c_src_cmsw_ml_report_status. */
    MLCMSW_STATUS_CONN_CREATE_FAIL = 19,

    /** @brief FACE_TSS_Base_Create_Connection API returned an error for connection: c_dst_cmsw_ml_rcv_config_0. */
    MLCMSW_RCV0_CONN_CREATE_FAIL = 20,

    /** @brief FACE_TSS_Base_Create_Connection API returned an error for connection: c_dst_cmsw_ml_rcv_config_1. */
    MLCMSW_RCV1_CONN_CREATE_FAIL = 21,

    /** @brief Initialization of PCS SAL has failed. */
    MLCMSW_PCS_SAL_INIT_FAIL = 22,

    /** @brief MLCMSW periodic process creation has failed. */
    MLCMSW_CREATE_PROC_FAIL = 23,

    /** @brief MLCMSW has not initialized yet. (call @ref mlcmsw_init()) */
    MLCMSW_START_NOT_INIT = 24,

    /** @brief MLCMSW periodic process START call failed. */
    MLCMSW_START_PROC_START_FAIL = 25,

    /** @brief a664PTPSyncHRTime API returned an error for perioidic synch tod. */
    MLCMSW_PTP_SYNCH_HR_TIME_ERR = 26,

    /** @brief gzisu_clock_core_hr_time_get API returned an error while getting timestamp. */
    MLCMSW_PROC_TIME_GET_ERR = 27,

    /** @brief Retrieved request indicates A653 Set Schedule operation. */
    MLCMSW_SET_A653_SCH_REQ_INFO = 28,

    /** @brief Retrieved request indicates A653 Major Frame Alignment with Set Schedule operation. */
    MLCMSW_SET_ALIGN_WITH_SCH_REQ_INFO = 29,

    /** @brief Retrieved request indicates A653 Major Frame Alignment operation. */
    MLCMSW_SET_ALIGN_A653_MF_REQ_INFO = 30,

    /** @brief SET_MODULE_SCHEDULE API returned an error while schedule set. */
    MLCMSW_SET_SCH_FAIL = 31,

    /** @brief A653 schedule has set without an error. */
    MLCMSW_SET_SCH_SUCCESS = 32,

    /** @brief Major frame synchronization completed without an error. */
    MLCMSW_PCS_MF_SYNC_SUCC = 33,

    /** @brief pcs_start_major_frame_sync API returned an error while major frame synchronization. */
    MLCMSW_PCS_MF_SYNC_FAIL = 34,

    /** @brief PTP is out of synch according to MLCMSW status report message. */
    MLCMSW_PTP_OUT_OF_SYNC = 35,

    /** @brief a664PTPSyncHRTime API returned an error while Major Frame synchronization. */
    MLCMSW_MF_REQ_PTP_SYNC_FAIL = 36,

    /** @brief Retrieved request is not defined in @ref Mlcmsw_request_type_enum. */
    MLCMSW_INVALID_REQ = 37,

    /** @brief a664ESMonitoring API returned monitoring data as NULL. */
    MLCMSW_DTN_MON_DATA_NULL = 38,

    /** @brief a664ESMonitoring API returned an error. */
    MLCMSW_DTN_MON_DATA_INVALID = 39,

    /** @brief PTP has synched on port A. */
    MLCMSW_SLAVE_A_PTP_SYNC_VALID = 40,

    /** @brief PTP has synched on port B. */
    MLCMSW_SLAVE_B_PTP_SYNC_VALID = 41,

    /** @brief tss_send_message API returned an error while sending MLCMSW status report message. */
    MLCMSW_SEND_STATUS_MSG_FAIL = 42,

    /** @brief SEND_BUFFER returned an error while logging MLCMSW status report message. */
    MLCMSW_LOG_STATUS_MSG_FAIL = 43,

    /** @brief MLCMSW status report message payload is bigger than max log payload. */
    MLCMSW_STATUS_LOG_BUFFER_OVERFLOW = 44,

    /** @brief pcs_read_hm_stats API returned an error. Failed to retrieve HM statistics from PCS. */
    MLCMSW_PCS_READ_HM_STAT_FAIL= 45,

    /** @brief pcs_read_hm_logs API returned an error. Failed to retrieve specific HM log content from PCS.*/
    MLCMSW_PCS_READ_HM_LOG_FAIL = 46,

    /** @brief For HM log, log payload exceeds the allocated buffer size. */
    MLCMSW_HM_LOG_BUFFER_OVERFLOW = 47,

    /** @brief SEND_BUFFER API returned an error. Failed to dispatch the report via the CMSW logging service. */
    MLCMSW_LOG_HM_LOG_FAIL = 48,

    /** @brief Indicates that HM log exists and logged to the file system without an error. */
    MLCMSW_HM_LOG_EXISTS = 49,

    /** @brief Buffer that has tA664ESMonitoring type is NULL. (While logging tA664ESMonitoring data)*/
    MLCMSW_DTN_MON_DATA_NULL_2 = 50,

    /** @brief DTN monitoring data log payload is bigger than max log payload. */
    MLCMSW_DTN_MON_DATA_BUFFER_FULL = 51,

    /** @brief SEND_BUFFER API returned an error while logging tA664ESMonitoring data. */
    MLCMSW_LOG_DTN_MON_DATA_FAIL = 52,

    /** @brief a664ESMonitoring API returned an error while DTN ES configuration. */
    MLCMSW_INITIAL_DTN_MON_FAIL = 53,

    /** @brief DTN ES has already configured. */
    MLCMSW_DTN_ES_ALREADY_DONE = 54,

    /** @brief a664ESMonitoring returned tA664ESMonitoring as NULL while DTN ES configuration. */
    MLCMSW_INITIAL_DTN_MON_BUFFER_NULL = 55,

    /** @brief pcs_ipmc_set_watchdog_timeout API returned error while set watchdog timer. */
    MLCMSW_WATCHDOG_SET_TIMEOUT_FAIL = 56,

    /** @brief pcs_ipmc_set_watchdog_timeout API success. Watchdog timer has set. */
    MLCMSW_WATCHDOG_SET_TIMEOUT_SUCCESS = 57,

    /** @brief pcs_ipmc_send_heartbeat API returned an error. */
    MLCMSW_WATCHDOG_KICK_FAIL = 58,

    /** @brief Watcdog timer timeout value is bigger than the freeze for value of MF synchronization rendezvous time. */
    MLCMSW_MF_SYNCH_TIME_EXCEED_WD = 59,

    /** @brief Watchdog is in freeze mode. */
    MLCMSW_WATCHDOG_FREEZE_MODE_INFO = 60,

    /** @brief Watchdog is in run mode. */
    MLCMSW_WATCHDOG_RUN_MODE_INFO = 61,

    /** @brief Failed to get high-resolution time for MF synchronization. */
    MLCMSW_MF_SYNCH_HR_TIME_GET_FAIL = 62,

    /** @brief Geographical address retrieved and validated. */
    MLCMSW_DEVICE_ID_OKAY = 63,

    /** @brief Failed to initialize MLCMSW status report message. */
    MLCMSW_INIT_STATUS_REPORT_FAIL = 64,

    /** @brief Failed to set watchdog timeout during MLCMSW initialization. */
    MLCMSW_INIT_WD_SET_TIMEOUT_FAIL = 65,

    /** @brief Successfully set MLCMSW initialization watchdog timeout. */
    MLCMSW_INIT_WD_SET_TIMEOUT_SUCC = 66,

    /** @brief pcs_import_config API returned an error. Failed to import SMMM LA configuration. */
    MLCMSW_SMMM_LA_IMPORT_CFG_FAIL = 67,

    /** @brief pcs_get_config_file_size API returned an error. Failed to get SMMM LA configuration size. */
    MLCMSW_SMMM_LA_GET_CFG_SIZE_FAIL = 68,

    /** @brief Failed to generate SMMM LA file name. */
    MLCMSW_SMMM_LA_FILE_NAME_FAIL = 69,

    /** @brief pcs_import_config API returned an error while loading config file. */
    MLCMSW_LOAD_CFG_IMPORT_FAIL = 70,

    /** @brief pcs_get_config_file_size API returned an error while loading config file. */
    MLCMSW_LOAD_CFG_FILE_SIZE_FAIL = 71,

    /** @brief Failed to generate file name during load config file. */
    MLCMSW_LOAD_CFG_FILE_NAME_FAIL = 72,

    /** @brief Malloc returned buffer NULL. */
    MLCMSW_LOAD_CFG_BUFFER_NO_MEMORY = 73,

    /** @brief Failed to initialize geographical address. */
    MLCMSW_INIT_GEO_ADDR_FAIL = 74,

    /** @brief Failed to get mount point. */
    MLCMSW_GET_MOUNT_POINT_FAIL = 75,

    /** @brief DFS client mount success. */
    MLCMSW_DFS_CLIENT_MOUNT_SUCC = 76,

    /** @brief gzisu_fs_client_mount API returned an error. DFS client mount failure. */
    MLCMSW_DFS_CLIENT_MOUNT_FAIL = 77,

    /** @brief Set watchdog success. */
    MLCMSW_SET_WATCHDOG_SUCC = 78,

    /** @brief Set watchdog failure. */
    MLCMSW_SET_WATCHDOG_FAIL = 79,

    /** @brief pcs_read_module_info API returned an error. Failed to get monolith ID. */
    MLCMSW_GET_MONOLITH_ID_FAIL = 80,

    /** @brief Watchdog timeout value is below minimum (200 ms). */
    MLCMSW_WATCHDOG_TIMEOUT_BELOW_MIN = 81,

    /** @brief Watchdog timeout value is not valid. */
    MLCMSW_WATCHDOG_TIMEOUT_NOT_VALID = 82,

    /** @brief ARINC653 START operation success for MLCMSW periodic process. */
    MLCMSW_START_SUCCESS = 83,

    /** @brief MLCMSW load configuration file failure. */
    MLCMSW_IMPORT_CFG_FAIL = 84,

    /** @brief MLCMSW configuration version mismatch. */
    MLCMSW_ML_CFG_VERS_MISMATCH = 85,

    /** @brief is_config_smmmla field of MLCMSW configuration file is out of bounds. */
    MLCMSW_SM_CFG_BOUND_CHECK_FAIL = 86,

    /** @brief watchdog_timeout_scale field of MLCMSW configuration file is out of bounds. */
    MLCMSW_WD_TIM_SCALE_BOUND_CHECK_FAIL = 87,

    /** @brief watchdog_timeout_factor field of MLCMSW configuration file is out of bounds. */
    MLCMSW_WD_TIM_FACTOR_BOUND_CHECK_FAIL = 88,

    /** @brief watchdog_timeout_unit field of MLCMSW configuration file is out of bounds. */
    MLCMSW_WD_TIM_UNIT_BOUND_CHECK_FAIL = 89,

    /** @brief watchdog_mode field of MLCMSW configuration file is out of bounds. */
    MLCMSW_WD_TIM_MODE_BOUND_CHECK_FAIL = 90,

    /** @brief MF synchronization integer overflow. */
    MLCMSW_MF_SYNCH_INT_OVERFLOW = 91,

    /** @brief SMMM LA driver configuration has failed. */
    MLCMSW_REQ_DRIVER_CFG_FAIL = 92,

    /** @brief MLCMSW initialization service success. */
    MLCMSW_INIT_SUCCESS = 93,

    /** @brief No DFS client mount available. */
    MLCMSW_NO_DFS_CLIENT_MOUNT = 94,

    /** @brief World start time bigger than rendezvous time. */
    MLCMSW_START_BIGGER_THAN_RENDEZVOUS = 95,

    /** @brief Partition period is zero. */
    MLCMSW_PARTITION_PERIOD_ZERO = 96,

    /** @brief MLCMSW config file size does not match with @ref Mlcmsw_config_type size. */
    MLCMSW_CFG_SIZE_MISMATCH = 97
} Mlcmsw_log_type;

/**
 * @enum Log_cmsw_init_ret_type_enum
 * 
 * @brief Enumeration for log initialization return codes.
 * 
 * @details Provides specific error and status codes returned during the initialization process of the logging
 *          subsystem.
 */
typedef enum Log_cmsw_init_ret_type_enum
{
    /** Initialization successful, no error. */
    LOG_RET_NO_ERR = 0,

    /** Failed to create the logging buffer. */
    LOG_RET_CREATE_BUFFER = 1,

    /** Failed to create the logging process. */
    LOG_RET_CREATE_PROCESS = 2,

    /** Failed to start the logging process. */
    LOG_RET_START_PROCESS = 3,

    /** Failed to open the log file. */
    LOG_RET_OPEN_LOG_FILE = 4,

    /** Error retrieving the file name. */
    LOG_RET_FILE_NAME = 5,

    /** Maximum number of files exceeded. */
    LOG_RET_MAX_FILE_NO_EXCEEDED = 6,

    /** Failed to create blackboard for folder creation. */
    LOG_RET_CREATE_BLACKBOARD = 7
} Log_cmsw_init_ret_type_enum;

/**
 * @enum Log_component_type
 * 
 * @brief Enumeration defining the source component of the log.
 */
typedef enum Log_component_type
{
    /** Log originates from the ML CMSW component. */
    LOG_MLCMSW = 0,
} Log_component_type;

/**
 * @enum Log_msg_type
 * 
 * @brief Enumeration for different types of log messages.
 * 
 * @details Defines the functional category of the log message, such as system messages, monitoring data, or RTOS
 *          exceptions.
 */
typedef enum Log_msg_type
{
    /** MLCMSW status report message. */
    LOG_STATUS_MSG = 0,

    /** DTN ES Monitoring Data message. */
    LOG_DTN_ES_MONITORING = 1,

    /** RTOS related exceptions. */
    LOG_RTOS_EXCEPTIONS = 2,

    /** Major Frame synchronization error. */
    LOG_MAJOR_SYNC_ERROR  = 3
} Log_msg_type;

/**
 * @struct Mlcmsw_log_file_header
 * 
 * @brief Structure representing the header of a log file.
 * 
 * @details This structure contains metadata required to identify, version, and locate the origin of a log file.
 * 
 * @note This structure is packed to ensure binary compatibility across platforms.
 */
typedef struct __attribute__((packed)) Mlcmsw_log_file_header
{
    /** Magic number/Identifier for the log file. (0x495050504C4F4700) */
    uint64_t log_file_identifier[8]; 

    /** Major version of the log file format. */
    uint8_t  log_file_version_major;

    /** Minor version of the log file format. */
    uint8_t  log_file_version_minor;

    /** Type of the log file. */
    Mlcmsw_file_type_enum log_file_type; 

    /** Owner type: 0: ML CMSW */
    uint8_t  log_file_owner_type; 

    /** Geographical address: MSB byte: LRU ID, LSB[7-3]: slot id. LSB[2-0]: 0x0. */
    uint16_t log_file_owner_geographical_address; 

    /** The partition ID of the file owner. */
    int32_t	 log_file_owner_partition_id;

    /** Timestamp of when the log file was created (gzisu_clock_core_hr_time_get). */
    uint64_t log_file_creation_time; 
} Mlcmsw_log_file_header;

/**
 * @struct Mlcmsw_log_entry_t
 * 
 * @brief Structure representing a single log entry.
 * 
 * @details Contains the core information of a specific log event, including its type, severity, and parameters.
 */
typedef struct __attribute__((packed)) Mlcmsw_log_entry_t
{
    /** The type of the log. (See @ref Mlcmsw_log_type). */
    Mlcmsw_log_type log_type;

    /** Severity level of the log entry. */
    int8_t log_severity;

    /** First parameter/value associated with the log entry. */
    int64_t first_param;

    /** Second parameter/value associated with the log entry. */
    int64_t second_param;
} Mlcmsw_log_entry_t;

/**
 * @struct Mlcmsw_log_frame_header_t
 * 
 * @brief Structure for the header of a log frame.
 * 
 * @details Provides metadata for a collection of log entries, such as timestamp and entry counts.
 */
typedef struct __attribute__((packed)) Mlcmsw_log_frame_header_t
{
    /** Timestamp of the log frame. */
    uint8_gt log_timestamp;

    /** Total number of valid log entries in this frame. */
    int32_t log_entry_count;

    /** Number of log entries dropped due to buffer overflow. */
    int32_t log_drop_count;
} Mlcmsw_log_frame_header_t;

/**
 * @struct Mlcmsw_log_frame_type
 * 
 * @brief Structure representing a complete log frame.
 * 
 * @details A log frame consists of a header and a list of log entries. This is the primary unit of log data
 *          transmission or storage.
 */
typedef struct __attribute__((packed)) Mlcmsw_log_frame_type
{
    /** Header containing frame metadata. */
    Mlcmsw_log_frame_header_t log_frame_header;

    /** Array of log entries contained in this frame. */
    Mlcmsw_log_entry_t log_entry_list[CMSW_MAX_LOG_ENTRY];
} Mlcmsw_log_frame_type;

/**
 * @struct Log_unique_identifier_type
 * 
 * @brief Structure providing a unique identification for a log source.
 */
typedef struct __attribute__((packed)) Log_unique_identifier_type
{
    /** Hardware/Device specific identifier. */
    uint16_t packed_device_id;

    /** The component type that generated the log. (See @ref Log_component_type). */
    Log_component_type component_type;
} Log_unique_identifier_type;

/**
 * @struct Log_header_type
 * 
 * @brief Structure for the general log message header.
 * 
 * @details Contains routing and identification information for the log payload.
 */
typedef struct __attribute__((packed)) Log_header_type
{
    /** Unique identifier for the log source. */
    Log_unique_identifier_type log_id;

    /** The specific type of the message. (See @ref Log_msg_type). */
    Log_msg_type message_type;

    /** Timestamp when the log was generated. */
    uint64_t timestamp;

    /** Length of the following payload in bytes. */
    uint32_t log_payload_len;
} Log_header_type;

/**
 * @struct Log_data_type
 * 
 * @brief Structure representing the complete log packet.
 * 
 * @details Combines the header information with the actual payload to form a complete data packet.
 */
typedef struct __attribute__((packed)) Log_data_type
{
    /** Metadata and routing information. */
    Log_header_type header;

    /** The raw data payload of the log. */
    APEX_BYTE log_payload[CMSW_LOGGER_MAX_BUFFER_SIZE];
} Log_data_type;

/**
 * @struct Mlcmsw_log_context_type
 * 
 * @brief Structure representing the logging context.
 * 
 * @details Maintains the state and configuration of a specific log file handle, including ownership and status.
 */
typedef struct Mlcmsw_log_context_type
{
    /** Unique identification of the log source. */
    Log_unique_identifier_type log_id;

    /** File identifier. */
    FILE_ID_TYPE file_id;

    /** Sequence number of the file. */
    int16_t file_no;

    /** Flag indicating if the log is an internal system log. */
    bool_gt is_internal;

    /** Flag indicating whether the log file is currently open. */
    bool_gt is_file_open;

    /** Total length of the log file in bytes. */
    int64_t log_file_len;
} Mlcmsw_log_context_type;

/**********************************************************************************************************************
*                                                 Function Prototypes                                                 *
**********************************************************************************************************************/

/**
 * @fn mlcmsw_get_log_directory (DIRECTORY_ENTRY_TYPE out)
 * 
 * @brief Retrieves the current active log directory.
 * 
 * @param[out] out Buffer where the current directory path will be copied.
 */
void mlcmsw_get_log_directory(DIRECTORY_ENTRY_TYPE out);

/**
 * @fn mlcmsw_get_log_file_no(Mlcmsw_file_type_enum file_type)
 * 
 * @brief Gets the current file number for a specific log type.
 * 
 * @param[in] file_type Enumeration specifying if it's an internal or message log file.
 * 
 * @returns int16_t The current file index/number.
 */
int16_t mlcmsw_get_log_file_no(Mlcmsw_file_type_enum file_type);

/**
 * @fn cmsw_log_frame_init (void)
 * 
 * @brief Starts a new major frame logging window.
 *
 * @details This function shall be called at the beginning of each major frame to reset the frame log buffer and set
 *          the frame timestamp.
 */
void cmsw_log_frame_init(void);

/**
 * @fn cmsw_log_flush (void)
 * 
 * @brief Flushes the current log frame to the internal log buffer.
 * 
 * @details Sends the accumulated log frame data to the internal logger task via the internal buffer.
 */
void cmsw_log_flush(void);

/**
 * @fn cmsw_log_event (Mlcmsw_severity_type log_severity, Mlcmsw_log_type log_type, int64_t first_param, 
 *                     int64_t second_param)
 * 
 * @brief Logs an internal MLCMSW event into the current major frame log buffer.
 *
 * @details This API is used to record diagnostic events (errors, informational states, or debug conditions) occuring
 *          within the MLCMSW. Each call appends a single log entry containing the specified severity, event types, and
 *          associated parameter value.
 *
 *          The collected entries are stored in a frame-level buffer and are intended to be flushed to persistent
 *          storage at the end of the major frame.
 *
 * @param[in] log_severity  Severity level of the event (e.g., CMSW_EMERGENCY, CMSW_DEBUG).
 * @param[in] log_type      Unique identifier of the logged event.
 * @param[in] first_param   Associated parameter value (typically a return code).
 * @param[in] second_param  Associated parameter value (typically an error code).
 */
void cmsw_log_event(
    Mlcmsw_severity_type log_severity,
    Mlcmsw_log_type log_type,
    int64_t first_param,
    int64_t second_param);

/**
 * @fn cmsw_report_log (Log_data_type* const log_data, RETURN_CODE_TYPE* return_code)
 *
 * @brief Reports application-level log data to the message log buffer.
 * 
 * @param[in]  log_data     Pointer to the log data structure to be reported.
 * @param[out] return_code  Return value returned from SEND_BUFFER() service.
 */
void cmsw_report_log(
    Log_data_type* const log_data,
    RETURN_CODE_TYPE* return_code);

/**
 * @fn mlcmsw_logger_init (Log_cmsw_init_ret_type_enum* const ret_code, int64_t* const ret_param)
 * 
 * @brief Initializes the logging system, creates buffers, and starts logging tasks.
 * 
 * @details This function performs the following:
 *          - Initializes log file contexts.
 *          - Creates Blackboard for inter process communication for folder creation.
 *          - Creates FIFO buffers for message and internal logs.
 *          - Creates and starts the background logging processes.
 * 
 * @param[out] ret_code Pointer to return the initialization status.
 * @param[out] ret_param Pointer to return additional error parameters or error codes.
 * 
 * @retval LOG_RET_NO_ERR
 *         Initialization successful.
 * 
 * @retval LOG_RET_CREATE_BLACKBOARD
 *         Failed to create the folder availability blackboard.
 * 
 * @retval LOG_RET_CREATE_BUFFER
 *         Failed to create one of the FIFO log buffers.
 * 
 * @retval LOG_RET_CREATE_PROCESS
 *         Failed to create the logger or internal logger process.
 * 
 * @retval LOG_RET_START_PROCESS
 *         Failed to start the logger or internal logger process.
 */
void mlcmsw_logger_init(
    Log_cmsw_init_ret_type_enum* const ret_code,
    int64_t* const ret_param);

#endif /* MLCMSW_LOGGER_H */

#else

#define CMSW_MAX_LOG_ENTRY 256

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
 * @struct Mlcmsw_log_entry_t
 * 
 * @brief Structure representing a single log entry.
 * 
 * @details Contains the core information of a specific log event, including its type, severity, and parameters.
 */
typedef struct __attribute__((packed)) Mlcmsw_log_entry_t
{
    /** The type of the log. (See @ref Mlcmsw_log_type). */
    Mlcmsw_log_type log_type;

    /** Severity level of the log entry. */
    int8_t log_severity;

    /** First parameter/value associated with the log entry. */
    int64_t first_param;

    /** Second parameter/value associated with the log entry. */
    int64_t second_param;
} Mlcmsw_log_entry_t;

/**
 * @struct Mlcmsw_log_frame_header_t
 * 
 * @brief Structure for the header of a log frame.
 * 
 * @details Provides metadata for a collection of log entries, such as timestamp and entry counts.
 */
typedef struct __attribute__((packed)) Mlcmsw_log_frame_header_t
{
    /** Timestamp of the log frame. */
    uint64_t log_timestamp; // uint8_gt -> uint64_t

    /** Total number of valid log entries in this frame. */
    int32_t log_entry_count;

    /** Number of log entries dropped due to buffer overflow. */
    int32_t log_drop_count;
} Mlcmsw_log_frame_header_t;

/**
 * @struct Mlcmsw_log_frame_type
 * 
 * @brief Structure representing a complete log frame.
 * 
 * @details A log frame consists of a header and a list of log entries. This is the primary unit of log data
 *          transmission or storage.
 */
typedef struct __attribute__((packed)) Mlcmsw_log_frame_type
{
    /** Header containing frame metadata. */
    Mlcmsw_log_frame_header_t log_frame_header;

    /** Array of log entries contained in this frame. */
    Mlcmsw_log_entry_t log_entry_list[CMSW_MAX_LOG_ENTRY];
} Mlcmsw_log_frame_type;

static inline void log_mlcmsw_detections_endian_convert(Mlcmsw_log_frame_type* data) {
    // BEGIN Mlcmsw_log_frame_type
    // BEGIN Mlcmsw_log_frame_header_t
    data->log_frame_header.log_timestamp = ntohll(data->log_frame_header.log_timestamp);
    data->log_frame_header.log_entry_count = ntohl(data->log_frame_header.log_entry_count);
    data->log_frame_header.log_drop_count = ntohl(data->log_frame_header.log_drop_count);
    // END Mlcmsw_log_frame_header_t
    for (int i = 0; i < CMSW_MAX_LOG_ENTRY; i++) {
        // BEGIN Mlcmsw_log_entry_t
        data->log_entry_list[i].log_type = (Mlcmsw_log_type) ntohl((uint32_t)(data->log_entry_list[i].log_type));
        data->log_entry_list[i].first_param = ntohll(data->log_entry_list[i].first_param);
        data->log_entry_list[i].second_param = ntohll(data->log_entry_list[i].second_param);
        // END Mlcmsw_log_entry_t
    }
    // END Mlcmsw_log_frame_type
}

static inline void log_mlcmsw_detections_header_print(FILE *fp) {
    fprintf(fp, "DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP,LOG PAYLOAD LENGTH,");
    // BEGIN Mlcmsw_log_frame_type
    // BEGIN Mlcmsw_log_frame_header_t
    fprintf(fp, "log_frame_header.log_timestamp,");
    fprintf(fp, "log_frame_header.log_entry_count,");
    fprintf(fp, "log_frame_header.log_drop_count,");
    // END Mlcmsw_log_frame_header_t
    for (int i = 0; i < CMSW_MAX_LOG_ENTRY; i++) {
        // BEGIN Mlcmsw_log_entry_t
        fprintf(fp, "log_entry_list[%d].log_type,", i);
        fprintf(fp, "log_entry_list[%d].log_severity,", i);
        fprintf(fp, "log_entry_list[%d].first_param,", i);
        fprintf(fp, "log_entry_list[%d].second_param\n", i);
        // END Mlcmsw_log_entry_t
    }
    // END Mlcmsw_log_frame_type
}

static inline void log_mlcmsw_detections_data_print(FILE *fp, Mlcmsw_log_frame_type* data) {
    log_mlcmsw_detections_endian_convert(data);
    // BEGIN Mlcmsw_log_frame_type
    // BEGIN Mlcmsw_log_frame_header_t
    fprintf(fp, "%llu,", data->log_frame_header.log_timestamp);
    fprintf(fp, "%d,", data->log_frame_header.log_entry_count);
    fprintf(fp, "%d,", data->log_frame_header.log_drop_count);
    // END Mlcmsw_log_frame_header_t
    for (int i = 0; i < CMSW_MAX_LOG_ENTRY; i++) {
        // BEGIN Mlcmsw_log_entry_t
        fprintf(fp, "%u,", (unsigned int)data->log_entry_list[i].log_type);
        fprintf(fp, "%d,", data->log_entry_list[i].log_severity);
        fprintf(fp, "%lld,", data->log_entry_list[i].first_param);
        fprintf(fp, "%lld\n", data->log_entry_list[i].second_param);
        // END Mlcmsw_log_entry_t
    }
    // END Mlcmsw_log_frame_type
}

#endif
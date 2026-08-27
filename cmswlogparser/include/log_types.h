#pragma once
#include <stdint.h>

const char *log_components[] = {"ML",
	"","MMP","DTN_IRSW","ADCU","SMMM","MMPLCMSW_DETECTIONS","PL","","","","",
	"","","","","","","","","","","",
	"","","","","","","","","","","",
	"","","","","","","","","","","",
	"","","","","","CL","DSM_EXCEPTION","DSM_DTNSW",
	"DSM_DTNES","CHASSIS", "CLCMSW_DETECTIONS", "IPMC_DPM", "IPMC_DSM", "IPMC_HSM", "IPMC_GPM", "IPMC_PSM",
	"IPMC_HUM", "IPMC_SMMM", "IPMC_IOCM"
};

const char *log_message[] = {"STATUS", "DTN_ES_MONITORING", "RTOS_EXCEPTIONS", "MAJOR_SYNC_ERROR"};
const char *adcu_message[] = {"CBIT", "PBIT", "DTNES"};
const char *dtn_message[] = {"STATUS_MON", "PORT_MON", "PARTIAL_PORT_MON", "HEALTH_MON"};

typedef enum {
	LOG_MLCMSW         = 0,
	LOG_MLCMSW_DETECTIONS = 1,
    LOG_MMPL_CMSW_MMP_STATUS     = 2,
    LOG_DTN_IRSW                 = 3,
	LOG_ADCU			= 4,
	LOG_SMMM			= 5,
	LOG_MMPLINTERNAL	= 6,
	LOG_MMPL_STATE		= 7,
	LOG_CL_CMSW         = 50,
	LOG_RTOS_EXCEPTION  = 51,
	LOG_DTNSW           = 52,
	LOG_DTNES           = 53,
	LOG_CHMC		    = 54,
	LOG_CLINTERNAL   	= 55,
	LOG_IPMC_DPM 		= 56,
	LOG_IPMC_DSM 		= 57,
	LOG_IPMC_HSM		= 58,
	LOG_IPMC_GPM		= 59,
	LOG_IPMC_PSM		= 60,
	LOG_IPMC_HUM		= 61,
	LOG_IPMC_SMMM		= 62,
	LOG_IPMC_IOCM		= 63,
} log_component_type;

typedef enum {
    LOG_STATUS_MSG              = 0,
    LOG_DTN_ES_MONITORING       = 1,
    LOG_RTOS_EXCEPTIONS         = 2,
	LOG_MAJOR_SYNC_ERROR        = 3,
} log_msg_type;

typedef struct __attribute__((packed))
{
	uint16_t device_id;
	log_component_type component_type;
} log_unique_identifier_type;

#if 0
typedef struct __attribute__((packed)){
	log_unique_identifier_type log_id;
	log_msg_type message_type;
	uint32_t 	 log_payload_len;
} log_header_type;
#else
typedef struct __attribute__((packed)){
	log_unique_identifier_type log_id;
	log_msg_type message_type;
	uint64_t timestamp;
	uint32_t 	 log_payload_len;
} log_header_type;
#endif
typedef struct __attribute__((packed)){
	log_header_type header;
	uint8_t *log_payload;
} log_data_type;

typedef struct __attribute__((packed)){
	uint64_t log_file_identifier; /* 0x495050504C4F4700 */
	uint8_t  log_file_version_major;
	uint8_t  log_file_version_minor;
	log_component_type log_file_type; /* log_component_type */
	uint8_t  log_file_owner_type; /*0: ML CMSW, 1: CL CMSW, 2: PL CMSW*/
	uint16_t log_file_owner_geographical_address; /* MSB byte: LRU ID,  LSB[7-3]: slot id. LSB[2-0]: 0x0 */
	int32_t	 log_file_owner_partition_id;
	uint64_t log_file_creation_time; /* gzisu_clock_core_hr_time_get() */
} log_file_header;

typedef struct __attribute__((packed)){
	uint64_t log_file_identifier[8]; /* 0x495050504C4F4700 */
	uint8_t  log_file_version_major;
	uint8_t  log_file_version_minor;
	log_component_type log_file_type; /* log_component_type */
	uint8_t  log_file_owner_type; /*0: ML CMSW, 1: CL CMSW, 2: PL CMSW*/
	uint16_t log_file_owner_geographical_address; /* MSB byte: LRU ID,  LSB[7-3]: slot id. LSB[2-0]: 0x0 */
	int32_t	 log_file_owner_partition_id;
	uint64_t log_file_creation_time; /* gzisu_clock_core_hr_time_get() */
} log_file_header_mlcmsw;

#define CMSW_MAX_LOG_ENTRY 256
#define CMSW_SEVERITY_MAX 8
#define MLCMSW_LOGTYPE_MAX 98
static char *severity_strings[9] = {"EMERGENCY", "ALERT", "CRITICAL", "ERROR", "WARNING", "NOTICE", "INFORMATIONAL", "DEBUG", "UNKNOWN"};
static char mlcmsw_log_fname_start[] = "mlcmsw_";
static char mlcmsw_detections_log_fname_start[] = "mlcmsw_detections_";
static char clcmsw_detections_log_fname_start[] = "clcmsw_detections_";
static char plcmsw_detections_log_fname_start[] = "mmplcmsw_detections_";

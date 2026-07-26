#include <stdio.h>
#include <stdint.h>

typedef enum pcs_LRM_dev_type
{
    DSM           = 1,
    DPM           = 2,
    HSM           = 3,
    PSM           = 4,
    HUM           = 5,
    GPM           = 6,
    IO            = 7,
    SMMM          = 8,
    HPM           = 9,
    SPM           = 10,
    GPGPM         = 11,
    TIMEOUT       = 14,
    NOT_REQUESTED = 15
} pcs_LRM_dev_type;

typedef enum Pcs_ipmc_validity_region_type
{
    pcs_invalid_region = 0,
    pcs_green_region   = 1,
    pcs_orange_region  = 2,
    pcs_red_region     = 3
} Pcs_ipmc_validity_region_type;

typedef enum pcs_power_status_type
{
    PCS_POWER_ON           = 1,
    PCS_POWER_OFF          = 2,
    PCS_WAITING            = 3,
    PCS_PARTIALLY_POWER_ON = 4
} pcs_power_status_type;


typedef struct __attribute__((packed)) Pcs_ipmc_limit_status
{
    uint32_t                       is_specified;   
    Pcs_ipmc_validity_region_type validity_region; 
} Pcs_ipmc_limit_status;
 
typedef struct __attribute__((packed)) Pcs_ipmc_signed_value_type
{
    int16_t               value;
    Pcs_ipmc_limit_status limit_status;
} Pcs_ipmc_signed_value_type;

typedef struct __attribute__((packed)){
	uint16_t 				device_id;
	pcs_LRM_dev_type		LRM_dev;
	uint8_t					fw_version_major;
	uint8_t					fw_version_minor;
	uint16_t				fw_version_patch;
	pcs_power_status_type	power_status;
    Pcs_ipmc_signed_value_type tmp451_sensor_for_cpu;
    Pcs_ipmc_signed_value_type tmp451_sensor_for_fpga;
}clcmsw_ipmc_cpu_data;

typedef struct __attribute__((packed)){
	uint16_t 				device_id;
	pcs_LRM_dev_type		LRM_dev;
	uint8_t					fw_version_major;
	uint8_t					fw_version_minor;
	uint16_t				fw_version_patch;
	pcs_power_status_type	power_status;
    Pcs_ipmc_signed_value_type tmp451_sensor_1;
    Pcs_ipmc_signed_value_type tmp451_sensor_2;
    Pcs_ipmc_signed_value_type tmp451_sensor_3;
    Pcs_ipmc_signed_value_type tmp451_sensor_4;
}clcmsw_ipmc_gpu_data;


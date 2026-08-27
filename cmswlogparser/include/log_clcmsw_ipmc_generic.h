#pragma once
#include <stdint.h>
typedef enum pcs_ipmc_op_mode_type
{
    PCS_IPMC_OFP         = 0,
    PCS_IPMC_MAINTENANCE = 1
} pcs_ipmc_op_mode_type;

typedef enum pcs_bus_state_type
{
    PCS_HEALTHY      = 0,
    PCS_CONN_TIMEOUT = 1
} pcs_bus_state_type;

typedef enum pcs_good_type
{
    PCS_NOT_GOOD = 0,
    PCS_GOOD     = 1
} pcs_good_type;

typedef enum pcs_valid_type
{
    PCS_VALID     = 0,
    PCS_NOT_VALID = 1
} pcs_valid_type;

typedef enum Pcs_ipmc_validity_region_type
{
    pcs_invalid_region  = 0,
    pcs_green_region    = 1,
    pcs_orange_region   = 2,
    pcs_red_region      = 3
} Pcs_ipmc_validity_region_type;

typedef struct __attribute__((packed)) Pcs_ipmc_limit_status
{
    uint32_t is_specified;
    Pcs_ipmc_validity_region_type validity_region;
} Pcs_ipmc_limit_status;

typedef struct __attribute__((packed)) Pcs_ipmc_signed_value_type
{
    uint16_t value;
    Pcs_ipmc_limit_status limit_status;
} Pcs_ipmc_signed_value_type;

typedef struct __attribute__((packed)) Pcs_ipmc_unsigned_value_type
{
    uint16_t value;
    Pcs_ipmc_limit_status limit_status;
} Pcs_ipmc_unsigned_value_type;

typedef enum pcs_power_status_type
{
    PCS_POWER_ON            = 1,
    PCS_POWER_OFF           = 2,
    PCS_WAITING             = 3,
    PCS_PARTIALLY_POWER_ON  = 4
} pcs_power_status_type;

typedef struct {
	pcs_LRM_dev_type    LRM_dev;
	uint8_t           is_auth;
	pcs_bus_state_type  ipmb_a ;
	pcs_bus_state_type  ipmb_b ;
	uint8_t fw_version_major;
	uint8_t fw_version_minor;
	uint8_t fw_version_patch;
	pcs_ipmc_op_mode_type   operation_mode;
	uint16_t              sequence_number;
	pcs_power_status_type   power_status;
	uint8_t               reset_counter;
	uint8_t               health_data_refresh_counter;
}clcmsw_generic_ipmc_data_type;


static const char *is_specified[] = {"TRUE", "FALSE", "UNKNOWN"};
static const char *validity_region[] = {"INVALID", "GREEN", "ORANGE", "RED", "UNKNOWN"};

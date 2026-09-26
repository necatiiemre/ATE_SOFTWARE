/*
 * smmm_message_types.h
 *
 * SMMM (Secure Mass Memory Module) monitoring telemetry.
 * VL 2021 üzerinden gelir (DPM-1 relay). Wire big-endian; UDP payload =
 * struct (130 B) + 1 byte sequence trailer = 131 B.
 *
 * Orijinal ICD (ate_comm_process.h) vendor tipleri kullanıyordu; burada
 * stdint karşılıklarına çevrildi:
 *   uint1_gt -> uint8_t   (1 byte)
 *   uint2_gt -> uint16_t  (2 byte)
 *   uint8_gt -> uint64_t  (8 byte)
 */

#ifndef SMMM_MESSAGE_TYPES_H
#define SMMM_MESSAGE_TYPES_H

#include <stdint.h>

// ============================================================================
// ADC dönüşüm formülleri — raw 16-bit ADC kodu → fiziksel birim
//   TEMP: sonuç °C. VCC: sonuç Volt; voltage VE current aynı formülü kullanır.
// ============================================================================
#define SM_HW_TEMP_ADC_CODE(x) (((509.3140064 * (double)(x)) / 65536.0) - 280.23087870)
#define SM_HW_VCC_ADC_CODE(x)  (((double)(x) / 65536.0) * 3.0)

typedef struct Smmm_time_typedef
{
    uint8_t hours;    /**< Hour component of the time.*/
    uint8_t minutes;  /**< Minute component of the time.*/
    uint8_t seconds;  /**< Second component of the time.*/
} Smmm_time_typedef_t;

typedef struct Smmm_date_typedef
{
    uint8_t day;      /**< Day of the month.*/
    uint8_t month;    /**< Month of the year.*/
    uint8_t year;     /**< Year.*/
} Smmm_date_typedef_t;

typedef struct Smmm_time
{
    Smmm_date_typedef_t date;  /**< Calendar date component.*/
    Smmm_time_typedef_t time;  /**< Time of day component.*/
} Smmm_time_t;

typedef struct __attribute__((packed))
{
    uint8_t reserved     : 6U;
    uint8_t trng         : 1U;
    uint8_t key          : 1U;
    uint8_t algorithm    : 1U;
    uint8_t f_o          : 1U;
    uint8_t uart_1       : 1U;
    uint8_t uart_0       : 1U;
    uint8_t dtn_1        : 1U;
    uint8_t dtn_0        : 1U;
    uint8_t mass_storage : 1U;
    uint8_t backplane    : 1U;
} Smmm_mgmt_bit_status_t;

typedef enum Smmm_mgmt_battery_voltage_level
{
    smmm_empty    = 0x00,
    smmm_critical = 0x33,
    smmm_low      = 0x55,
    smmm_medium   = 0x77,
    smmm_high     = 0xAA
} Smmm_mgmt_battery_voltage_level_t;

typedef struct Smmm_mgmt_alarm_status
{
    uint8_t reserved              : 7U;
    uint8_t crypto_algorithm      : 1U;
    uint8_t crypto_key_integrty   : 1U;
    uint8_t high_psu_voltage      : 1U;
    uint8_t low_psu_voltage       : 1U;
    uint8_t high_module_temp      : 1U;
    uint8_t battery_voltage_low   : 1U;
    uint8_t external_tamper       : 1U;
    uint8_t internal_tamper       : 1U;
    uint8_t emergency_zeroization : 1U;
} Smmm_mgmt_alarm_status_t;

typedef struct __attribute__((packed))
{
    uint16_t                          smmm_version;                     /**< SMMM version.*/
    uint16_t                          voltage;                          /**< Voltage (raw ADC).*/
    uint16_t                          current;                          /**< Current (raw ADC).*/
    uint16_t                          temperature;                      /**< Temperature (raw ADC).*/
    Smmm_mgmt_bit_status_t            bit_status;                       /**< Bit status.*/
    Smmm_time_t                       smmm_time;                        /**< SMMM time (SMMM RTC).*/
    uint64_t                          smmm_tod;                         /**< SMMM time of day (SMMM ToD).*/
    uint64_t                          crypto_status[4];                 /**< Crypto status.*/
    uint64_t                          nvme_rx_count;                    /**< SMMM NVMe read sector count.*/
    uint64_t                          nvme_tx_count;                    /**< SMMM NVMe write sector count.*/
    Smmm_mgmt_battery_voltage_level_t battery_level;                    /**< internal battery level.*/
    uint16_t                          sanitization_status;              /**< Sanitization status.*/
    uint16_t                          current_state;                    /**< SMMM current state.*/
    uint16_t                          hw_id;                            /**< SMMM unique identifier.*/
    uint16_t                          security_controller_fw_version;   /**< Security controller FW version.*/
    uint16_t                          ipmc_fw_version;                  /**< IPMC firmware version.*/
    Smmm_mgmt_alarm_status_t          alarm_status;                     /**< Alarm status.*/
    uint8_t                           latest_pilot_auth_status;         /**< Pilot authentication status.*/
    uint64_t                          pilot_auth_status_occurance_time; /**< Timestamp when pilot auth status changed.*/
    uint8_t                           smmm_storage_lock_status;         /**< SMMM storage lock status.*/
    uint64_t                          eth_packet_drop_count;            /**< Ethernet packet drop count.*/
    uint64_t                          eth_header_error_count;           /**< Ethernet header error count.*/
    uint64_t                          eth_rx_count;                     /**< Ethernet RX count.*/
    uint64_t                          eth_tx_count;                     /**< Ethernet TX count.*/
} Smmm_monitoring_data_t;

#endif /* SMMM_MESSAGE_TYPES_H */
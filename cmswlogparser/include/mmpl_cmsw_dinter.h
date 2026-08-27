/**
* @file mmpl_cmsw_dinter.h
* @copyright TUBITAK
* @details This file includes definitions for DTN IR SW message structure.
* @author ufuk.aksoy 2021
* @author samet.yildiz 2025
* @author abdulhamit.kumru 2025
*/

#ifndef INTERNAL_COMMON_MMPL_CMSW_DINTER_H_
#define INTERNAL_COMMON_MMPL_CMSW_DINTER_H_

#include <stdint.h>

#define DINTER_PARSING_STEP_COUNT 7U
#define INVALID_CONDITION_1_BYTE 255U

static const uint32_t DINTER_STATUS_MON_MSG_SIZE = 1144U;
static const uint32_t DINTER_PORT_MON_MSG_SIZE = 1040U;
static const uint32_t DINTER_PARTIAL_PORT_MON_MSG_SIZE = 395U;
static const uint32_t DINTER_MCU_MON_MSG_SIZE = 51U;
static const uint32_t DINTER_DTN_ES_MON_MSG_SIZE = 1007U;

// ==========================================
// DEVICE HEADER OFFSETS (from UDP payload)
// ==========================================

#define DEV_OFF_DEVICE_ID            0    // 2 bytes
#define DEV_OFF_OPERATION_TYPE       2    // 1 byte (ReadWriteFlag)
#define DEV_OFF_CONFIG_TYPE          3    // 1 byte (config type start address)
#define DEV_OFF_FRAME_LENGTH         4    // 2 bytes (monitoring data length)
#define DEV_OFF_STATUS_ENABLE        6    // 1 byte (assistant/manager flag)
#define DEV_OFF_STATUS_ADDR          7    // 2 bytes (status address)
#define DEV_OFF_TX_TOTAL_COUNT       9    // 6 bytes
#define DEV_OFF_RX_TOTAL_COUNT       15   // 6 bytes
#define DEV_OFF_TX_ERR_TOTAL_COUNT   21   // 6 bytes
#define DEV_OFF_RX_ERR_TOTAL_COUNT   27   // 6 bytes
#define DEV_OFF_HEARTBEAT            33   // 1 byte
#define DEV_OFF_DEV_ID2              34   // 2 bytes
#define DEV_OFF_PORT_COUNT           36   // 1 byte
#define DEV_OFF_TOKEN_BUCKET         37   // 1 byte
#define DEV_OFF_SW_MODE              38   // 1 byte
#define DEV_OFF_PADDING1             39   // 3 bytes
#define DEV_OFF_VENDOR_ID            42   // 1 byte
#define DEV_OFF_AUTO_MAC_UPDATE      43   // 1 byte
#define DEV_OFF_UPSTREAM_MODE        44   // 1 byte
#define DEV_OFF_SW_IP_CORE_VER       45   // 6 bytes
#define DEV_OFF_ES_IP_CORE_VER       51   // 6 bytes
#define DEV_OFF_SW_INPUT_FIFO        57   // 2 bytes
#define DEV_OFF_PKT_PRO_FIFO         59   // 2 bytes
#define DEV_OFF_SW_OUTPUT_FIFO       61   // 2 bytes
#define DEV_OFF_HP_FIFO_SIZE         63   // 2 bytes
#define DEV_OFF_LP_FIFO_SIZE         65   // 2 bytes
#define DEV_OFF_BE_FIFO_SIZE         67   // 2 bytes
#define DEV_OFF_PADDING2             69   // 1 byte
#define DEV_OFF_TOD_NS               70   // 5 bytes
#define DEV_OFF_PADDING3             75   // 1 byte
#define DEV_OFF_TOD_SEC              76   // 5 bytes
#define DEV_OFF_ETH_WRONG_DEV_CNT   81   // 6 bytes
#define DEV_OFF_ETH_WRONG_OP_CNT    87   // 6 bytes
#define DEV_OFF_ETH_WRONG_TYPE_CNT  93   // 6 bytes
#define DEV_OFF_RESERVED1            99   // 2 bytes
#define DEV_OFF_FPGA_VOLTAGE         101  // 2 bytes
#define DEV_OFF_FPGA_TEMP            103  // 2 bytes
#define DEV_OFF_CONFIG_ID            105  // 2 bytes
#define DEV_OFF_RESERVED2            107  // 4 bytes

// ==========================================
// PORT DATA OFFSETS (from port data start)
// ==========================================

#define PORT_OFF_PORT_NUMBER         0    // 2 bytes (monitoring port address)
#define PORT_OFF_BIT_STATUS          2    // 1 byte (upper 4: bit test result, lower 4: get port stats)
#define PORT_OFF_CRC_ERR_CNT         3    // 6 bytes
#define PORT_OFF_ALI_ERR_CNT         9    // 6 bytes (alignment error)
#define PORT_OFF_LEN_EXC_64          15   // 6 bytes (length exceed count 64)
#define PORT_OFF_LEN_EXC_1518        21   // 6 bytes (length exceed count 1518)
#define PORT_OFF_MIN_VL_FRAME_ERR    27   // 6 bytes (length exceed vl min)
#define PORT_OFF_MAX_VL_FRAME_ERR    33   // 6 bytes (length exceed vl max)
#define PORT_OFF_INP_PORT_TERR_CNT   39   // 6 bytes (input port terror count)
#define PORT_OFF_TRAFFIC_POLICY_DROP 45   // 6 bytes (traffic filter count)
#define PORT_OFF_BE_COUNT            51   // 6 bytes (consider count)
#define PORT_OFF_TX_COUNT            57   // 6 bytes
#define PORT_OFF_RX_COUNT            63   // 6 bytes
#define PORT_OFF_VL_SOURCE_ERR       69   // 6 bytes (count err vl)
#define PORT_OFF_MAX_DELAY_ERR       75   // 6 bytes (count err over max delay)
#define PORT_OFF_QUEUE_OVERFLOW      81   // 6 bytes (count err queue overflow)
#define PORT_OFF_VLID_DROP           87   // 6 bytes (undefined vl err count)
#define PORT_OFF_UNDEF_MAC           93   // 6 bytes (undefined be mac err count)
#define PORT_OFF_HP_QUEUE_OVERFLOW   99   // 6 bytes
#define PORT_OFF_LP_QUEUE_OVERFLOW   105  // 6 bytes
#define PORT_OFF_BE_QUEUE_OVERFLOW   111  // 6 bytes
#define PORT_OFF_MAX_DELAY_PARAM     117  // 6 bytes (max delay parameter)
#define PORT_OFF_PORT_SPEED          123  // 6 bytes (0=1000M, 1=10M, 2=100M)

// ==========================================
// MCU DATA OFFSETS (from UDP payload)
// ==========================================

#define MCU_OFF_DEVICE_ID            0    // 2 bytes
#define MCU_OFF_OPERATION_TYPE       2    // 1 byte (ReadWriteFlag)
#define MCU_OFF_CONFIG_TYPE          3    // 1 byte
#define MCU_OFF_FRAME_LENGTH         4    // 2 bytes
#define MCU_OFF_STATUS_ENABLE        6    // 1 byte
#define MCU_OFF_FW_VERSION           7    // 2 bytes (MCU firmware version major.minor.patch)
#define MCU_OFF_INPUT_POWER_STATUS   9    // 1 byte (bit0: 28V Primary, bit1: 28V Secondary; 0=SUCCESS, 1=FAIL)
#define MCU_OFF_PBIT                 10   // 1 byte (DTN IRSW CBA components PBIT: 0x00=SUCCESS, 0x01=FAIL)
#define MCU_OFF_CBIT                 11   // 1 byte (DTN IRSW CBA components CBIT: 0x00=SUCCESS, 0x01=FAIL)
// Current data (readValue/1000)
#define MCU_OFF_CURR_12V             12   // 2 bytes (12V current)
#define MCU_OFF_CURR_3V3             14   // 2 bytes (3.3V current)
#define MCU_OFF_CURR_1V8             16   // 2 bytes (1.8V current)
#define MCU_OFF_CURR_3V3_FO          18   // 2 bytes (3.3V FO transceiver current)
#define MCU_OFF_CURR_1V3             20   // 2 bytes (1.3V current)
#define MCU_OFF_CURR_1V0_MGR         22   // 2 bytes (1.0V DTN IRSW manager FPGA current)
#define MCU_OFF_CURR_1V0_AST         24   // 2 bytes (1.0V DTN IRSW assistant FPGA current)
// Voltage data (readValue/1000)
#define MCU_OFF_VOLT_3V3             26   // 2 bytes (3.3V voltage)
#define MCU_OFF_VOLT_3V3_FO          28   // 2 bytes (3.3V FO transceiver voltage)
#define MCU_OFF_VOLT_12V             30   // 2 bytes (12V voltage)
#define MCU_OFF_VOLT_1V8             32   // 2 bytes (1.8V VCCIO voltage)
#define MCU_OFF_VOLT_1V3             34   // 2 bytes (1.3V VCC voltage)
#define MCU_OFF_VOLT_1V0_MGR         36   // 2 bytes (1.0V VDD DTN IRSW manager FPGA voltage)
#define MCU_OFF_VOLT_1V0_AST         38   // 2 bytes (1.0V VDD DTN IRSW assistant FPGA voltage)
// Temperature data
#define MCU_OFF_BOARD_TEMP           40   // 2 bytes (DTN IRSW CBA temperature, readValue/100)
#define MCU_OFF_FO_TRANS1_TEMP       42   // 2 bytes (FO transceiver 1 temperature, readValue/100)
#define MCU_OFF_RESERVED1            44   // 2 bytes (reserved)
#define MCU_OFF_RESERVED2            46   // 2 bytes (reserved)
#define MCU_OFF_ETH_PHY_1G_TEMP      48   // 1 byte (Ethernet PHY 1G temperature, signed)
#define MCU_OFF_ETH_PHY_100M_TEMP    49   // 1 byte (Ethernet PHY 100mbit temperature, signed)

// Mini header size for packets without full device header
#define HEALTH_MINI_HEADER_SIZE      7   // DevID(2) + OpType(1) + CfgType(1) + FrameLen(2) + Reserved(1)

// Port data constants
#define HEALTH_PORT_DATA_SIZE        129
#define HEALTH_MAX_PORTS             35

// ==========================================
// BYTE PARSING FUNCTIONS (Big-Endian)
// ==========================================

static inline uint16_t parse_2byte_be(const uint8_t *data)
{
    return ((uint16_t)data[0] << 8) | (uint16_t)data[1];
}

static inline uint32_t parse_4byte_be(const uint8_t *data)
{
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8)  | (uint32_t)data[3];
}

static inline uint64_t parse_5byte_be(const uint8_t *data)
{
    return ((uint64_t)data[0] << 32) | ((uint64_t)data[1] << 24) |
           ((uint64_t)data[2] << 16) | ((uint64_t)data[3] << 8) |
           (uint64_t)data[4];
}

static inline uint64_t parse_6byte_be(const uint8_t *data)
{
    return ((uint64_t)data[0] << 40) | ((uint64_t)data[1] << 32) |
           ((uint64_t)data[2] << 24) | ((uint64_t)data[3] << 16) |
           ((uint64_t)data[4] << 8)  | (uint64_t)data[5];
}

typedef enum
{
    DINTER_STATUS_MON_A = 0U,
    DINTER_PORT_MON_A,
    DINTER_STATUS_MON_B,
    DINTER_PORT_MON_B,
    DINTER_PARTIAL_PORT_MON_B,
    DINTER_MCU_MON,
    DINTER_DTN_ES,
    DINTER_INVALID_STATE
} Mmplcmsw_dinter_state;

typedef struct
{
    uint16_t port_number;         // Monitoring port address
    uint8_t  bit_status;          // Upper 4 bit: bit test result, Lower 4 bit: get port stats
    uint64_t crc_err_count;       // CRC error count
    uint64_t ali_err_count;       // Alignment error count
    uint64_t len_exc_64;          // Length exceed count 64
    uint64_t len_exc_1518;        // Length exceed count 1518
    uint64_t min_vl_frame_err;    // Length exceed VL min
    uint64_t max_vl_frame_err;    // Length exceed VL max
    uint64_t inp_port_terr_cnt;   // Input port terror count
    uint64_t traffic_policy_drop; // Traffic filter count
    uint64_t be_count;            // Consider count
    uint64_t tx_count;            // TX frame count
    uint64_t rx_count;            // RX frame count
    uint64_t vl_source_err;       // Count error VL
    uint64_t max_delay_err;       // Count error over max delay
    uint64_t queue_overflow;      // Count error queue overflow
    uint64_t vlid_drop_count;     // Undefined VL error count
    uint64_t undef_mac_count;     // Undefined BE MAC error count
    uint64_t hp_queue_overflow;   // High priority queue overflow count
    uint64_t lp_queue_overflow;   // Low priority queue overflow count
    uint64_t be_queue_overflow;   // Best effort queue overflow count
    uint64_t max_delay_param;     // Max delay parameter
    uint64_t port_speed;          // Port speed (0=1000M, 1=10M, 2=100M)
    bool     valid;               // Data received flag

} Mmplcmsw_port_based_incoming_msg;

typedef struct Mmplcmsw_dtn_irs_status_mon_msg_t {
    uint16_t device_id;           // Device ID
    uint8_t  operation_type;      // ReadWriteFlag (e.g., 0x53)
    uint8_t  config_type;         // Config type start address (e.g., 0x44)
    uint16_t frame_length;        // Monitoring data (frame) length
    uint8_t  status_enable;       // Status enable (assistant/manager indicator)
    uint16_t status_addr;         // Status address
    uint64_t tx_total_count;      // Total TX frame count
    uint64_t rx_total_count;      // Total RX frame count
    uint64_t tx_err_total_count;  // Total error TX frame count
    uint64_t rx_err_total_count;  // Total error RX frame count
    uint8_t  heartbeat;           // Heartbeat counter
    uint16_t device_id2;          // Device ID (repeated)
    uint8_t  port_count;          // Number of ports (e.g., 0x23 = 35)
    uint8_t  token_bucket_status; // Token bucket status
    uint8_t  sw_mode;             // Switch mode
    uint8_t  vendor_id;           // Vendor ID
    uint8_t  auto_mac_update;     // Auto MAC update
    uint8_t  upstream_mode;       // Upstream mode
    uint8_t  sw_ip_major;         // SW IP core version major
    uint8_t  sw_ip_minor;         // SW IP core version minor
    uint8_t  sw_ip_patch;         // SW IP core version patch
    uint8_t  es_ip_major;         // ES IP core version major
    uint8_t  es_ip_minor;         // ES IP core version minor
    uint8_t  es_ip_patch;         // ES IP core version patch
    uint16_t sw_input_fifo_size;  // SW input FIFO size
    uint16_t pkt_pro_fifo_size;   // Packet pro output FIFO size
    uint16_t sw_output_fifo_size; // SW output FIFO size
    uint16_t hp_fifo_size;        // High priority FIFO size
    uint16_t lp_fifo_size;        // Low priority FIFO size
    uint16_t be_fifo_size;        // Best effort FIFO size
    uint64_t tod_ns;              // Time of day nanoseconds (5 bytes)
    uint64_t tod_sec;             // Time of day seconds (5 bytes)
    uint64_t eth_wrong_dev_cnt;   // Eth conf wrong device ID count
    uint64_t eth_wrong_op_cnt;    // Eth conf wrong operation mode count
    uint64_t eth_wrong_type_cnt;  // Eth conf wrong type count
    uint16_t fpga_voltage;        // FPGA voltage (raw)
    int16_t  fpga_temp;           // FPGA temperature (raw, signed)
    uint16_t config_id;           // Configuration ID
} Mmplcmsw_dtn_irs_status_mon_msg_t;

typedef struct __attribute__((packed))
{
    uint16_t header_device_id;
    uint8_t  reserved_0[2];
    uint16_t monitoring_frame_length;
    uint8_t  status_enable;

    Mmplcmsw_port_based_incoming_msg port_i[8];
} Mmplcmsw_dinter_port_mon_msg_t;

typedef struct __attribute__((packed))
{
    uint16_t header_device_id;
    uint8_t  reserved_0[2];
    uint16_t monitoring_frame_length;
    uint8_t  status_enable;

    Mmplcmsw_port_based_incoming_msg port_i[3];
} Mmplcmsw_dinter_partial_port_mon_t;

typedef struct {
    // Header
    uint16_t device_id;
    uint8_t  operation_type;
    uint8_t  config_type;
    uint16_t frame_length;
    uint8_t  status_enable;
    uint8_t  fw_major;            // MCU firmware version major (high nibble of byte 0)
    uint8_t  fw_minor;            // MCU firmware version minor (low nibble of byte 0)
    uint8_t  fw_patch;            // MCU firmware version patch (byte 1)
    uint8_t  input_power_status;  // bit0: 28V Primary, bit1: 28V Secondary (0=SUCCESS, 1=FAIL)
    uint8_t  pbit;                // DTN IRSW CBA components PBIT (0x00=SUCCESS, 0x01=FAIL)
    uint8_t  cbit;                // DTN IRSW CBA components CBIT (0x00=SUCCESS, 0x01=FAIL)

    // Current data (raw value, divide by 1000 for display)
    uint16_t curr_12v;
    uint16_t curr_3v3;
    uint16_t curr_1v8;
    uint16_t curr_3v3_fo;         // 3.3V FO transceiver
    uint16_t curr_1v3;
    uint16_t curr_1v0_mgr;       // 1.0V DTN IRSW manager FPGA
    uint16_t curr_1v0_ast;       // 1.0V DTN IRSW assistant FPGA

    // Voltage data (raw value, divide by 1000 for display)
    uint16_t volt_3v3;
    uint16_t volt_3v3_fo;         // 3.3V FO transceiver
    uint16_t volt_12v;
    uint16_t volt_1v8;            // 1.8V VCCIO
    uint16_t volt_1v3;            // 1.3V VCC
    uint16_t volt_1v0_mgr;       // 1.0V VDD DTN IRSW manager FPGA
    uint16_t volt_1v0_ast;       // 1.0V VDD DTN IRSW assistant FPGA

    // Temperature data
    int16_t  board_temp;          // DTN IRSW CBA temperature (raw / 100.0, signed)
    int16_t  fo_trans1_temp;      // FO transceiver 1 temperature (raw / 100.0, signed)
    int8_t   eth_phy_1g_temp;     // Ethernet PHY 1G temperature (signed)
    int8_t   eth_phy_100m_temp;   // Ethernet PHY 100mbit temperature (signed)

    bool     valid;
} Dtn_ir_switch_cba_health_monitoring_data_field_t;

typedef struct
{
    Mmplcmsw_dinter_state expected_state;
    uint16_t              expected_msg_len;
    uint16_t              expected_status_enable;
    uint16_t              expected_port_count;
} Parser_rule;

static const Parser_rule parser_table[7] = {
    {.expected_state         = DINTER_STATUS_MON_A,
     .expected_msg_len       = 0x478,
     .expected_status_enable = 0x03,
     .expected_port_count    = 8},
    {.expected_state         = DINTER_PORT_MON_A,
     .expected_msg_len       = 0x408,
     .expected_status_enable = 0x00,
     .expected_port_count    = 8},
    {.expected_state         = DINTER_STATUS_MON_B,
     .expected_msg_len       = 0x470,
     .expected_status_enable = 0x01,
     .expected_port_count    = 8},
    {.expected_state         = DINTER_PORT_MON_B,
     .expected_msg_len       = 0x408,
     .expected_status_enable = 0x00,
     .expected_port_count    = 8},
    //        {
    //                .expected_state = DINTER_PARTIAL_PORT_MON_A,
    //                .expected_msg_len = 0x183,
    //                .expected_status_enable = 0x00,
    //                .expected_port_count = 3
    //        },
    {
        .expected_state         = DINTER_PARTIAL_PORT_MON_B,
        .expected_msg_len       = 0x183,
        .expected_status_enable = 0x00,
        .expected_port_count    = 3},
    {.expected_state         = DINTER_MCU_MON,
     .expected_msg_len       = 0x3C,
     .expected_status_enable = 0x05,
     .expected_port_count    = 0},
    {.expected_state         = DINTER_DTN_ES,
     .expected_msg_len       = 0x3e8,
     .expected_status_enable = 0x00,
     .expected_port_count    = 0}};

#endif /* INTERNAL_COMMON_MMPL_CMSW_DINTER_H_ */

static int dinter_log_print_header(FILE **fp, int dinter_msg_chooser) {
    // Base CSV headers common to all logs
    fprintf(*fp, "DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP,LOG PAYLOAD LENGTH,");

    if (0 == dinter_msg_chooser) {
        // --- Mmplcmsw_dtn_irs_status_mon_msg_t Header ---
        fprintf(*fp, "device_id,operation_type,config_type,frame_length,status_enable,"
                "status_addr,tx_total_count,rx_total_count,tx_err_total_count,"
                "rx_err_total_count,heartbeat,device_id2,port_count,token_bucket_status,"
                "sw_mode,vendor_id,auto_mac_update,upstream_mode,sw_ip_major,sw_ip_minor,"
                "sw_ip_patch,es_ip_major,es_ip_minor,es_ip_patch,sw_input_fifo_size,"
                "pkt_pro_fifo_size,sw_output_fifo_size,hp_fifo_size,lp_fifo_size,be_fifo_size,"
                "tod_ns,tod_sec,eth_wrong_dev_cnt,eth_wrong_op_cnt,eth_wrong_type_cnt,"
                "fpga_voltage,fpga_temp,config_id\n");
    } 
    else if (1 == dinter_msg_chooser) {
        // --- Mmplcmsw_dinter_port_mon_msg_t Header ---

        // 1. Print the Mini Header (The top-level fields)
        fprintf(*fp, "header_device_id,monitoring_frame_length,status_enable,");
        
        // 2. Print the Port Array columns
        for (int i = 0; i < 8; i++) {            
            fprintf(*fp, "p[%d]_port_number,p[%d]_bit_status,p[%d]_crc_err_count,p[%d]_ali_err_count,"
                            "p[%d]_len_exc_64,p[%d]_len_exc_1518,p[%d]_min_vl_frame_err,p[%d]_max_vl_frame_err,"
                            "p[%d]_inp_port_terr_cnt,p[%d]_traffic_policy_drop,p[%d]_be_count,p[%d]_tx_count,"
                            "p[%d]_rx_count,p[%d]_vl_source_err,p[%d]_max_delay_err,p[%d]_queue_overflow,"
                            "p[%d]_vlid_drop_count,p[%d]_undef_mac_count,p[%d]_hp_queue_overflow,p[%d]_lp_queue_overflow,"
                            "p[%d]_be_queue_overflow,p[%d]_max_delay_param,p[%d]_port_speed",
                    i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i);

            if (i < 7) {
                fprintf(*fp, ",");
            } else {
                fprintf(*fp, "\n");
            }
        }
    }
    else if (2 == dinter_msg_chooser) {
        // --- Mmplcmsw_dinter_partial_port_mon_msg_t Header ---

        // 1. Print the Mini Header (The top-level fields)
        fprintf(*fp, "header_device_id,monitoring_frame_length,status_enable,");
        
        // 2. Print the Port Array columns
        for (int i = 0; i < 3; i++) {            
            fprintf(*fp, "p[%d]_port_number,p[%d]_bit_status,p[%d]_crc_err_count,p[%d]_ali_err_count,"
                            "p[%d]_len_exc_64,p[%d]_len_exc_1518,p[%d]_min_vl_frame_err,p[%d]_max_vl_frame_err,"
                            "p[%d]_inp_port_terr_cnt,p[%d]_traffic_policy_drop,p[%d]_be_count,p[%d]_tx_count,"
                            "p[%d]_rx_count,p[%d]_vl_source_err,p[%d]_max_delay_err,p[%d]_queue_overflow,"
                            "p[%d]_vlid_drop_count,p[%d]_undef_mac_count,p[%d]_hp_queue_overflow,p[%d]_lp_queue_overflow,"
                            "p[%d]_be_queue_overflow,p[%d]_max_delay_param,p[%d]_port_speed",
                    i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i);

            if (i < 2) {
                fprintf(*fp, ",");
            } else {
                fprintf(*fp, "\n");
            }
        }
    }
    else if (3 == dinter_msg_chooser) {
        // --- dtn_es_mon Header ---
        fprintf(*fp, "%s,", "device_id");
        fprintf(*fp, "%s,", "wr");
        fprintf(*fp, "%s,", "monitoring_type");
        fprintf(*fp, "%s,", "monitoring_length");

        // TX Counts
        fprintf(*fp, "%s,", "A664_ES_TX_INCOMING_COUNT");
        fprintf(*fp, "%s,", "A664_ES_TX_VLID_DROP_COUNT");
        fprintf(*fp, "%s,", "A664_ES_TX_LMIN_LMAX_DROP_COUNT");
        fprintf(*fp, "%s,", "A664_ES_TX_BE_INCOMING_COUNT");
        fprintf(*fp, "%s,", "A664_ES_TX_FIFO_OVERFLOW_COUNT");

        // Monitoring and Config
        fprintf(*fp, "%s,", "A664_ES_TX_MONITOR_COUNT");
        fprintf(*fp, "%s,", "A664_ES_CONFIG_RB_COUNT");
        fprintf(*fp, "%s,", "A664_ES_CONFIG_STATUS");

        // Identification and Versioning
        fprintf(*fp, "%s,", "A664_ES_DEV_ID");
        fprintf(*fp, "%s,", "A664_ES_FW_VER");
        fprintf(*fp, "%s,", "A664_ES_PTP_STATUS");

        // TX and RX Flow
        fprintf(*fp, "%s,", "A664_ES_TX_A_OUTGOING_COUNT");
        fprintf(*fp, "%s,", "A664_ES_TX_B_OUTGOING_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_A_INCOMING_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_B_INCOMING_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_A_CRC_ERR_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_B_CRC_ERR_COUNT");
        fprintf(*fp, "%s,", "A664_ES_PORT_A_STATUS");
        fprintf(*fp, "%s,", "A664_ES_PORT_B_STATUS");
        fprintf(*fp, "%s,", "A664_ES_PORT_SPEED");

        // RX A Errors
        fprintf(*fp, "%s,", "A664_ES_RX_A_SEQ_ERR_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_A_LMIN_LMAX_DROP_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_A_VLID_DROP_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_A_NET_ERR_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_A_BE_INCOMING_COUNT");

        // RX B Errors
        fprintf(*fp, "%s,", "A664_ES_RX_B_SEQ_ERR_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_B_LMIN_LMAX_DROP_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_B_VLID_DROP_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_B_NET_ERR_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_B_BE_INCOMING_COUNT");

        // RX Throughput and Overflow
        fprintf(*fp, "%s,", "A664_ES_RX_OUTGOING_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_FIFO_OVERFLOW_COUNT");

        // Environmental and System Status
        fprintf(*fp, "%s,", "A664_ES_TEMPERATURE");
        fprintf(*fp, "%s,", "A664_ES_VOLTAGE");
        fprintf(*fp, "%s,", "A664_ES_BITSTREAM_UPDATE_STATUS");
        fprintf(*fp, "%s,", "A664_ES_MODE");
        fprintf(*fp, "%s,", "A664_ES_CONFIGURATION_ID");
        fprintf(*fp, "%s,", "A664_ES_BIT_STATUS");
        fprintf(*fp, "%s\n", "finish");
    }
    else if (4 == dinter_msg_chooser) {
        // --- Dtn_ir_switch_cba_health_monitoring_data_field_t Header ---
        fprintf(*fp, "device_id,operation_type,config_type,frame_length,status_enable,"
                "fw_major,fw_minor,fw_patch,input_power_status,pbit,cbit,"
                "curr_12v,curr_3v3,curr_1v8,curr_3v3_fo,curr_1v3,curr_1v0_mgr,curr_1v0_ast,"
                "volt_3v3,volt_3v3_fo,volt_12v,volt_1v8,volt_1v3,volt_1v0_mgr,volt_1v0_ast,"
                "board_temp,fo_trans1_temp,eth_phy_1g_temp,eth_phy_100m_temp,valid\n");
    }

    return 0;
}

/**
 * @brief Writes the actual payload data for Mmplcmsw_dtn_irs_status_mon_msg_t to the CSV
 * @param fp Pointer to the file pointer
 * @param payload Pointer to the raw data buffer
 */
static int dinter_status_mon_log_parser(FILE *fp, const uint8_t *payload) {
    // Cast the raw payload to the actual struct type for easy access
    Mmplcmsw_dtn_irs_status_mon_msg_t msg;

    msg.device_id          = parse_2byte_be(payload + DEV_OFF_DEVICE_ID);
    msg.operation_type     = payload[DEV_OFF_OPERATION_TYPE];
    msg.config_type        = payload[DEV_OFF_CONFIG_TYPE];
    msg.frame_length       = parse_2byte_be(payload + DEV_OFF_FRAME_LENGTH);
    msg.status_enable      = payload[DEV_OFF_STATUS_ENABLE];
    msg.status_addr        = parse_2byte_be(payload + DEV_OFF_STATUS_ADDR);
    msg.tx_total_count     = parse_6byte_be(payload + DEV_OFF_TX_TOTAL_COUNT);
    msg.rx_total_count     = parse_6byte_be(payload + DEV_OFF_RX_TOTAL_COUNT);
    msg.tx_err_total_count = parse_6byte_be(payload + DEV_OFF_TX_ERR_TOTAL_COUNT);
    msg.rx_err_total_count = parse_6byte_be(payload + DEV_OFF_RX_ERR_TOTAL_COUNT);
    msg.heartbeat          = payload[DEV_OFF_HEARTBEAT];
    msg.device_id2         = parse_2byte_be(payload + DEV_OFF_DEV_ID2);
    msg.port_count         = payload[DEV_OFF_PORT_COUNT];
    msg.token_bucket_status = payload[DEV_OFF_TOKEN_BUCKET];
    msg.sw_mode            = payload[DEV_OFF_SW_MODE];
    msg.vendor_id          = payload[DEV_OFF_VENDOR_ID];
    msg.auto_mac_update    = payload[DEV_OFF_AUTO_MAC_UPDATE];
    msg.upstream_mode      = payload[DEV_OFF_UPSTREAM_MODE];

    // SW IP core version (3x uint16_t big-endian in 6-byte field)
    msg.sw_ip_major = (uint8_t)parse_2byte_be(payload + DEV_OFF_SW_IP_CORE_VER);
    msg.sw_ip_minor = (uint8_t)parse_2byte_be(payload + DEV_OFF_SW_IP_CORE_VER + 2);
    msg.sw_ip_patch = (uint8_t)parse_2byte_be(payload + DEV_OFF_SW_IP_CORE_VER + 4);

    // ES IP core version (3 single bytes in last 3 bytes of 6-byte field)
    msg.es_ip_major = payload[DEV_OFF_ES_IP_CORE_VER + 3];
    msg.es_ip_minor = payload[DEV_OFF_ES_IP_CORE_VER + 4];
    msg.es_ip_patch = payload[DEV_OFF_ES_IP_CORE_VER + 5];

    msg.sw_input_fifo_size  = parse_2byte_be(payload + DEV_OFF_SW_INPUT_FIFO);
    msg.pkt_pro_fifo_size   = parse_2byte_be(payload + DEV_OFF_PKT_PRO_FIFO);
    msg.sw_output_fifo_size = parse_2byte_be(payload + DEV_OFF_SW_OUTPUT_FIFO);
    msg.hp_fifo_size        = parse_2byte_be(payload + DEV_OFF_HP_FIFO_SIZE);
    msg.lp_fifo_size        = parse_2byte_be(payload + DEV_OFF_LP_FIFO_SIZE);
    msg.be_fifo_size        = parse_2byte_be(payload + DEV_OFF_BE_FIFO_SIZE);

    msg.tod_ns              = parse_5byte_be(payload + DEV_OFF_TOD_NS);
    msg.tod_sec             = parse_5byte_be(payload + DEV_OFF_TOD_SEC);

    msg.eth_wrong_dev_cnt   = parse_6byte_be(payload + DEV_OFF_ETH_WRONG_DEV_CNT);
    msg.eth_wrong_op_cnt    = parse_6byte_be(payload + DEV_OFF_ETH_WRONG_OP_CNT);
    msg.eth_wrong_type_cnt  = parse_6byte_be(payload + DEV_OFF_ETH_WRONG_TYPE_CNT);

    msg.fpga_voltage        = parse_2byte_be(payload + DEV_OFF_FPGA_VOLTAGE);
    msg.fpga_temp           = (int16_t)parse_2byte_be(payload + DEV_OFF_FPGA_TEMP);
    msg.config_id           = parse_2byte_be(payload + DEV_OFF_CONFIG_ID);

    fprintf(fp,
        "%u,%u,%u,%u,%u,%u,%llu,%llu,%llu,%llu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%llu,%llu,%llu,%llu,%llu,%u,%hd,%u\n",
        msg.device_id, msg.operation_type, msg.config_type, msg.frame_length,
        msg.status_enable, msg.status_addr, msg.tx_total_count, msg.rx_total_count,
        msg.tx_err_total_count, msg.rx_err_total_count, msg.heartbeat, msg.device_id2,
        msg.port_count, msg.token_bucket_status, msg.sw_mode, msg.vendor_id,
        msg.auto_mac_update, msg.upstream_mode, msg.sw_ip_major, msg.sw_ip_minor,
        msg.sw_ip_patch, msg.es_ip_major, msg.es_ip_minor, msg.es_ip_patch,
        msg.sw_input_fifo_size, msg.pkt_pro_fifo_size, msg.sw_output_fifo_size,
        msg.hp_fifo_size, msg.lp_fifo_size, msg.be_fifo_size,
        msg.tod_ns, msg.tod_sec, msg.eth_wrong_dev_cnt, msg.eth_wrong_op_cnt,
        msg.eth_wrong_type_cnt, msg.fpga_voltage, msg.fpga_temp, msg.config_id);

    return 0;
}

/**
 * @brief Helper: Parses exactly one port from the provided buffer offset
 * @param payload The base payload pointer
 * @param offset  The starting byte offset for this specific port
 * @param msg     Pointer to the struct to store the results
 */
static void parse_single_port(const uint8_t *payload, size_t offset, Mmplcmsw_port_based_incoming_msg *msg) {
    const uint8_t *p = payload + offset;

    msg->port_number         = parse_2byte_be(p + PORT_OFF_PORT_NUMBER);
    msg->bit_status          = p[PORT_OFF_BIT_STATUS];
    msg->crc_err_count       = parse_6byte_be(p + PORT_OFF_CRC_ERR_CNT);
    msg->ali_err_count       = parse_6byte_be(p + PORT_OFF_ALI_ERR_CNT);
    msg->len_exc_64          = parse_6byte_be(p + PORT_OFF_LEN_EXC_64);
    msg->len_exc_1518        = parse_6byte_be(p + PORT_OFF_LEN_EXC_1518);
    msg->min_vl_frame_err    = parse_6byte_be(p + PORT_OFF_MIN_VL_FRAME_ERR);
    msg->max_vl_frame_err    = parse_6byte_be(p + PORT_OFF_MAX_VL_FRAME_ERR);
    msg->inp_port_terr_cnt   = parse_6byte_be(p + PORT_OFF_INP_PORT_TERR_CNT);
    msg->traffic_policy_drop  = parse_6byte_be(p + PORT_OFF_TRAFFIC_POLICY_DROP);
    msg->be_count            = parse_6byte_be(p + PORT_OFF_BE_COUNT);
    msg->tx_count            = parse_6byte_be(p + PORT_OFF_TX_COUNT);
    msg->rx_count            = parse_6byte_be(p + PORT_OFF_RX_COUNT);
    msg->vl_source_err       = parse_6byte_be(p + PORT_OFF_VL_SOURCE_ERR);
    msg->max_delay_err       = parse_6byte_be(p + PORT_OFF_MAX_DELAY_ERR);
    msg->queue_overflow      = parse_6byte_be(p + PORT_OFF_QUEUE_OVERFLOW);
    msg->vlid_drop_count     = parse_6byte_be(p + PORT_OFF_VLID_DROP);
    msg->undef_mac_count     = parse_6byte_be(p + PORT_OFF_UNDEF_MAC);
    msg->hp_queue_overflow   = parse_6byte_be(p + PORT_OFF_HP_QUEUE_OVERFLOW);
    msg->lp_queue_overflow   = parse_6byte_be(p + PORT_OFF_LP_QUEUE_OVERFLOW);
    msg->be_queue_overflow   = parse_6byte_be(p + PORT_OFF_BE_QUEUE_OVERFLOW);
    msg->max_delay_param     = parse_6byte_be(p + PORT_OFF_MAX_DELAY_PARAM);
    msg->port_speed          = parse_6byte_be(p + PORT_OFF_PORT_SPEED);
}

/**
 * @brief Main Parser: Writes one single long CSV row containing the mini-header 
 *        followed by all port array data.
 */
static int dinter_port_mon_log_parser(FILE *fp, const uint8_t *payload) {
    // 1. Cast payload to the top-level struct to easily access mini-header fields
    Mmplcmsw_dinter_port_mon_msg_t full_msg;

    full_msg.header_device_id = parse_2byte_be(payload);
    full_msg.monitoring_frame_length = payload[2];
    full_msg.status_enable =  parse_2byte_be(payload + 3);

    // 2. Start the CSV Row with the Mini Header fields (the first 3 columns)
    fprintf(fp, "%u,%u,%u,", 
            full_msg.header_device_id, 
            full_msg.monitoring_frame_length, 
            full_msg.status_enable);

    // 3. Loop through each port to append its data to the same line
    for (int i = 0; i < 8; i++) {
        Mmplcmsw_port_based_incoming_msg msg;
        memset(&msg, 0, sizeof(msg));

        // Calculate the specific offset for this port in the array
        size_t current_port_offset = HEALTH_MINI_HEADER_SIZE + (i * HEALTH_PORT_DATA_SIZE);

        // Parse the data for this one port
        parse_single_port(payload, current_port_offset, &msg);

        // 4. Write the port data
        fprintf(fp, "%u,%u,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu",
            msg.port_number,          // uint16_t
            msg.bit_status,           // uint8_t
            msg.crc_err_count,        // uint64_t
            msg.ali_err_count,        // uint64_t
            msg.len_exc_64,           // uint64_t
            msg.len_exc_1518,         // uint64_t
            msg.min_vl_frame_err,     // uint64_t
            msg.max_vl_frame_err,     // uint64_t
            msg.inp_port_terr_cnt,    // uint64_t
            msg.traffic_policy_drop,  // uint64_t
            msg.be_count,             // uint64_t
            msg.tx_count,             // uint64_t
            msg.rx_count,             // uint64_t
            msg.vl_source_err,        // uint64_t
            msg.max_delay_err,        // uint64_t
            msg.queue_overflow,       // uint64_t
            msg.vlid_drop_count,      // uint64_t
            msg.undef_mac_count,      // uint64_t
            msg.hp_queue_overflow,    // uint64_t
            msg.lp_queue_overflow,    // uint64_t
            msg.be_queue_overflow,    // uint64_t
            msg.max_delay_param,      // uint64_t
            msg.port_speed            // uint64_t
        );

        if (i < 7) {
            fprintf(fp, ",");
        } else {
            fprintf(fp, "\n");
        }
    }

    return 0;
}

/**
 * @brief Main Parser: Loops through the port array and writes rows to CSV
 */
static int dinter_partial_port_mon_log_parser(FILE *fp, const uint8_t *payload) {
    // 1. Cast payload to the top-level struct to easily access mini-header fields
    Mmplcmsw_dinter_partial_port_mon_t full_msg;

    full_msg.header_device_id = parse_2byte_be(payload);
    full_msg.monitoring_frame_length = payload[2];
    full_msg.status_enable =  parse_2byte_be(payload + 3);

    // 2. Start the CSV Row with the Mini Header fields (the first 3 columns)
    fprintf(fp, "%u,%u,%u,", 
            full_msg.header_device_id, 
            full_msg.monitoring_frame_length, 
            full_msg.status_enable);

    // 3. Loop through each port to append its data to the same line
    for (int i = 0; i < 3; i++) {
        Mmplcmsw_port_based_incoming_msg msg;
        memset(&msg, 0, sizeof(msg));

        // Calculate the specific offset for this port in the array
        size_t current_port_offset = HEALTH_MINI_HEADER_SIZE + (i * HEALTH_PORT_DATA_SIZE);

        // Parse the data for this one port
        parse_single_port(payload, current_port_offset, &msg);

        // 4. Write the port data
        fprintf(fp, "%u,%u,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu",
            msg.port_number,          // uint16_t
            msg.bit_status,           // uint8_t
            msg.crc_err_count,        // uint64_t
            msg.ali_err_count,        // uint64_t
            msg.len_exc_64,           // uint64_t
            msg.len_exc_1518,         // uint64_t
            msg.min_vl_frame_err,     // uint64_t
            msg.max_vl_frame_err,     // uint64_t
            msg.inp_port_terr_cnt,    // uint64_t
            msg.traffic_policy_drop,  // uint64_t
            msg.be_count,             // uint64_t
            msg.tx_count,             // uint64_t
            msg.rx_count,             // uint64_t
            msg.vl_source_err,        // uint64_t
            msg.max_delay_err,        // uint64_t
            msg.queue_overflow,       // uint64_t
            msg.vlid_drop_count,      // uint64_t
            msg.undef_mac_count,      // uint64_t
            msg.hp_queue_overflow,    // uint64_t
            msg.lp_queue_overflow,    // uint64_t
            msg.be_queue_overflow,    // uint64_t
            msg.max_delay_param,      // uint64_t
            msg.port_speed            // uint64_t
        );

        if (i < 2) {
            fprintf(fp, ",");
        } else {
            fprintf(fp, "\n");
        }
    }

    return 0;
}


/**
 * @brief Writes the actual payload data for Dtn_ir_switch_cba_health_monitoring_data_field_t to the CSV
 * @param fp Pointer to the file pointer
 * @param payload Pointer to the raw data buffer
 */
static int dinter_cba_hm_log_parser(FILE *fp, const uint8_t *payload) {
    // Cast the raw payload to the actual struct type
    Dtn_ir_switch_cba_health_monitoring_data_field_t msg;

    // Header
    msg.device_id          = parse_2byte_be(payload + MCU_OFF_DEVICE_ID);
    msg.operation_type     = payload[MCU_OFF_OPERATION_TYPE];
    msg.config_type        = payload[MCU_OFF_CONFIG_TYPE];
    msg.frame_length       = parse_2byte_be(payload + MCU_OFF_FRAME_LENGTH);
    msg.status_enable      = payload[MCU_OFF_STATUS_ENABLE];
    msg.fw_major           = (payload[MCU_OFF_FW_VERSION] >> 4) & 0x0F;
    msg.fw_minor           = payload[MCU_OFF_FW_VERSION] & 0x0F;
    msg.fw_patch           = payload[MCU_OFF_FW_VERSION + 1];
    msg.input_power_status = payload[MCU_OFF_INPUT_POWER_STATUS];
    msg.pbit               = payload[MCU_OFF_PBIT];
    msg.cbit               = payload[MCU_OFF_CBIT];

    // Current data (readValue/1000)
    msg.curr_12v      = parse_2byte_be(payload + MCU_OFF_CURR_12V);
    msg.curr_3v3      = parse_2byte_be(payload + MCU_OFF_CURR_3V3);
    msg.curr_1v8      = parse_2byte_be(payload + MCU_OFF_CURR_1V8);
    msg.curr_3v3_fo   = parse_2byte_be(payload + MCU_OFF_CURR_3V3_FO);
    msg.curr_1v3      = parse_2byte_be(payload + MCU_OFF_CURR_1V3);
    msg.curr_1v0_mgr  = parse_2byte_be(payload + MCU_OFF_CURR_1V0_MGR);
    msg.curr_1v0_ast  = parse_2byte_be(payload + MCU_OFF_CURR_1V0_AST);

    // Voltage data (readValue/1000)
    msg.volt_3v3      = parse_2byte_be(payload + MCU_OFF_VOLT_3V3);
    msg.volt_3v3_fo   = parse_2byte_be(payload + MCU_OFF_VOLT_3V3_FO);
    msg.volt_12v      = parse_2byte_be(payload + MCU_OFF_VOLT_12V);
    msg.volt_1v8      = parse_2byte_be(payload + MCU_OFF_VOLT_1V8);
    msg.volt_1v3      = parse_2byte_be(payload + MCU_OFF_VOLT_1V3);
    msg.volt_1v0_mgr  = parse_2byte_be(payload + MCU_OFF_VOLT_1V0_MGR);
    msg.volt_1v0_ast  = parse_2byte_be(payload + MCU_OFF_VOLT_1V0_AST);

    // Temperature data
    msg.board_temp        = (int16_t)parse_2byte_be(payload + MCU_OFF_BOARD_TEMP);
    msg.fo_trans1_temp    = (int16_t)parse_2byte_be(payload + MCU_OFF_FO_TRANS1_TEMP);
    msg.eth_phy_1g_temp   = (int8_t)payload[MCU_OFF_ETH_PHY_1G_TEMP];
    msg.eth_phy_100m_temp = (int8_t)payload[MCU_OFF_ETH_PHY_100M_TEMP];

    fprintf(fp, "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
                "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
                "%d,%d,%d,%d\n",
        msg.device_id,        // uint16_t
        msg.operation_type,   // uint8_t
        msg.config_type,     // uint8_t
        msg.frame_length,     // uint16_t
        msg.status_enable,   // uint8_t
        msg.fw_major,        // uint8_t
        msg.fw_minor,        // uint8_t
        msg.fw_patch,        // uint8_t
        msg.input_power_status, // uint8_t
        msg.pbit,            // uint8_t
        msg.cbit,            // uint8_t
        msg.curr_12v,        // uint16_t
        msg.curr_3v3,        // uint16_t
        msg.curr_1v8,        // uint16_t
        msg.curr_3v3_fo,     // uint16_t
        msg.curr_1v3,        // uint16_t
        msg.curr_1v0_mgr,    // uint16_t
        msg.curr_1v0_ast,    // uint16_t
        msg.volt_3v3,        // uint16_t
        msg.volt_3v3_fo,     // uint16_t
        msg.volt_12v,        // uint16_t
        msg.volt_1v8,        // uint16_t
        msg.volt_1v3,        // uint16_t
        msg.volt_1v0_mgr,    // uint16_t
        msg.volt_1v0_ast,    // uint16_t
        msg.board_temp,      // int16_t
        msg.fo_trans1_temp,  // int16_t
        msg.eth_phy_1g_temp, // int8_t
        msg.eth_phy_100m_temp // int8_t
    );

    return 0;
}

static int dinter_dtn_es_log_parser(FILE *outfp, dtn_es_mon *dtnes)
{
    if (!outfp || !dtnes) {
        return -1;
    }

    fprintf(outfp, "%u,", ntohs(dtnes->device_id));
    fprintf(outfp, "%u,", dtnes->wr);
    fprintf(outfp, "%u,", dtnes->monitoring_type);
    fprintf(outfp, "%u,", ntohs(dtnes->monitoring_length));

    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_TX_INCOMING_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_TX_VLID_DROP_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_TX_LMIN_LMAX_DROP_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_TX_BE_INCOMING_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_TX_FIFO_OVERFLOW_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_TX_MONITOR_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_CONFIG_RB_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_CONFIG_STATUS));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_DEV_ID));

    uint64_t fwver = ntohll(dtnes->A664_ES_FW_VER);
    fprintf(outfp, "%u.%u.%u,", (fwver >> 16) & 0xFF, (fwver >> 8) & 0xFF, fwver & 0xFF);
    fprintf(outfp, "0x%016llX,", ntohll(dtnes->A664_ES_PTP_STATUS));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_TX_A_OUTGOING_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_TX_B_OUTGOING_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_RX_A_INCOMING_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_RX_B_INCOMING_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_RX_A_CRC_ERR_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_RX_B_CRC_ERR_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_PORT_A_STATUS));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_PORT_B_STATUS));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_PORT_SPEED));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_RX_A_SEQ_ERR_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_RX_A_LMIN_LMAX_DROP_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_RX_A_VLID_DROP_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_RX_A_NET_ERR_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_RX_A_BE_INCOMING_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_RX_B_SEQ_ERR_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_RX_B_LMIN_LMAX_DROP_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_RX_B_VLID_DROP_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_RX_B_NET_ERR_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_RX_B_BE_INCOMING_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_RX_OUTGOING_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_RX_FIFO_OVERFLOW_COUNT));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_TEMPERATURE));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_VOLTAGE));
    fprintf(outfp, "0x%016llX,", ntohll(dtnes->A664_ES_BITSTREAM_UPDATE_STATUS));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_MODE));
    fprintf(outfp, "%llu,", ntohll(dtnes->A664_ES_CONFIGURATION_ID));
    fprintf(outfp, "0x%016llX,", ntohll(dtnes->A664_ES_BIT_STATUS));

    fprintf(outfp, "%u\n", dtnes->finish);
    return 0;
}

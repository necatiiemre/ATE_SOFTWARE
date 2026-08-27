#pragma once
#include <stdlib.h>
#define ADCU_CBIT_REGISTER_COUNT 21U
#define ADCU_PBIT_REGISTER_COUNT 20U

#define ADCU_ANALOG_RAW_REGISTER_COUNT 12U
#define ADCU_DISCRETE_RAW_REGISTER_COUNT 24U
static const uint8_t ADCU_HM_PBIT_MSG_TYPE = 0x01U;
static const uint8_t ADCU_HM_CBIT_MSG_TYPE = 0x02U;

static const uint32_t ADCU_HM_CBIT_MSG_SIZE   = 201U;
static const uint32_t ADCU_HM_PBIT_MSG_SIZE   = 194U;
static const uint32_t ADCU_HM_DCU_ES_MSG_SIZE = 1007U;

static const uint8_t ADCU_HM_MSG_TYPE_INDEX = 2U;

typedef enum
{
    ADCU_CBIT         = 0,
    ADCU_PBIT         = 1,
    ADCU_ES           = 2,
    ADCU_UNIDENTIFIED = 3,
} Cmsw_pl_adcu_hm_msg_type;

typedef struct __attribute__((packed)) 
{
    uint8_t reserved_u4 :       4; // Bit 7-4: Reserved
    uint8_t identifier_error :  1; // Bit 3: 0=No error, 1=Error
    uint8_t ethernet_mode_u1 :  1; // Bit 1: 0=Half-dupex, 1=Full-dupex
    uint8_t ethernet_speed_u1 : 1; // Bit 2: 0=10Mbit, 1=100Mbit
    uint8_t link_status_u1 :    1; // Bit 0: 0=Link down, 1=Link up
}Mmplcmsw_adcu_ethernet_status;

/**
 * @brief ADCU CBIT HM message in network byte order. Used in message serialization. Compatible with ADCU 0.1.12 firmware
 *
 * @see AV_DCU_HSI
 */
typedef struct __attribute__((packed))
{
	uint16_t device_id;
	uint8_t hm_report_type; // 0x02
	uint32_t timetag_sec;
    uint32_t timetag_nsec;
	uint16_t msg_len;
	uint8_t dcu_type; 		// 0x02
	uint32_t channel_config_id;
	uint32_t channel_vl_mapping_config_id;
	uint8_t m1553_status_0x0110_reserved_u4 : 4;
	uint8_t m1553_status_0x0110_rt_sub_addr_lut_rcv_status_u1 : 1;
	uint8_t m1553_status_0x0110_inst_list_rcv_status_u1 : 1;
	uint8_t m1553_status_0x0110_bus_type_u2 : 2;
	uint8_t m1553_status_0x0111_reserved_u4 : 4;
	uint8_t m1553_status_0x0111_rt_sub_addr_lut_rcv_status_u1 : 1;
	uint8_t m1553_status_0x0111_inst_list_rcv_status_u1 : 1;
	uint8_t m1553_status_0x0111_bus_type_u2 : 2;
	uint32_t m1553_inst_list_id_0x0110;
	uint32_t m1553_inst_list_id_0x0111;
	uint32_t m1553_lut_id_0x0110;
	uint32_t m1553_lut_id_0x0111;
    Mmplcmsw_adcu_ethernet_status eth_status_0x100;
	Mmplcmsw_adcu_ethernet_status eth_status_0x101;
    uint8_t config_flash_read_resp: 1; // 0= Not Read, 1: Read
    uint8_t config_flash_crc_reponse : 1;                      // 0=Pass, 1=Fail
    uint8_t channel_vl_mapping_config_id_mismatch_u1 : 1; // Bit 5: 0=Compatible, 1=Incompatible
    uint8_t channel_config_id_mismatch_u1 : 1;    // Bit 4: 0=Compatible, 1=Incompatible
    uint8_t heart_beat_received_u1 : 1;           // Bit 3: 0=Not Received, 1=Received
    uint8_t major_frame_sync_u1 : 1;              // Bit 2: 0=Not Sync, 1=Sync
    uint8_t structure_version_2nd_packet_u1 : 1;  // Bit 1: 0=Compatible, 1=Incompatible
    uint8_t structure_version_1st_packet_u1 : 1;  // Bit 0: 0=Compatible, 1=Incompatible
	// uint8_t config_reponse;
	uint32_t cbit_registers[ADCU_CBIT_REGISTER_COUNT];
	uint8_t analog_hm_len;
	uint8_t discerete_hm_len;
	uint32_t analog_raw_data_registers[ADCU_ANALOG_RAW_REGISTER_COUNT];	   // The Analog HM Data Length shall be equal to 0x0C (12).
	uint8_t discrete_raw_data_registers[ADCU_DISCRETE_RAW_REGISTER_COUNT]; // The Discrete HM Data Length shall be equal to 0xB9 (185)
} Cmsw_pl_adcu_cbit_msg_type;

typedef struct __attribute__((packed))
{
    uint16_t device_id;
    uint8_t  hm_report_type; // 0x01
    uint8_t  timetag[8];
    uint16_t msg_len;
    uint8_t  dcu_type;          // 0x02
    uint32_t dcu_fw_hw_version; // 0-12 Bits: DCU Firmware Version (0x001), 13-32 DCU HW Version
    uint8_t  dcu_hw_uid[8];
    uint32_t channel_config_id;
    uint32_t channel_vl_mapping_config_id;
    uint32_t pbit_registers[ADCU_PBIT_REGISTER_COUNT];
    uint32_t pbit_timeout[ADCU_PBIT_REGISTER_COUNT];
} Cmsw_pl_adcu_pbit_msg_type;

typedef struct __attribute__((packed))
{
	uint16_t device_id;
	uint8_t wr; // Indicated monitoring write or read
	uint8_t monitoring_type;
	uint16_t monitoring_length; // 0x03e8

	uint16_t ADDRESS_FOR_A664_ES_TX_INCOMING_COUNT;/*0x00*/
	uint64_t A664_ES_TX_INCOMING_COUNT;

    uint8_t  reserved[10];

	uint16_t ADDRESS_FOR_A664_ES_TX_VLID_DROP_COUNT; /*0x02*/
	uint64_t A664_ES_TX_VLID_DROP_COUNT;

	uint16_t ADDRESS_FOR_A664_ES_TX_LMIN_LMAX_DROP_COUNT;/*0x03*/
	uint64_t A664_ES_TX_LMIN_LMAX_DROP_COUNT;

	uint16_t ADDRESS_FOR_A664_ES_TX_BE_INCOMING_COUNT;/*0x04*/
	uint64_t A664_ES_TX_BE_INCOMING_COUNT;

	uint16_t ADDRESS_FOR_A664_ES_TX_FIFO_OVERFLOW_COUNT;/*0x05*/
	uint64_t A664_ES_TX_FIFO_OVERFLOW_COUNT;

    uint8_t  reserved2[10];

	uint16_t ADDRESS_FOR_A664_ES_TX_MONITOR_COUNT; /*0x07*/
	uint64_t A664_ES_TX_MONITOR_COUNT;

	uint16_t ADDRESS_FOR_A664_ES_CONFIG_RB_COUNT; /*0x08*/
	uint64_t A664_ES_CONFIG_RB_COUNT;

	uint16_t ADDRESS_FOR_A664_ES_CONFIG_STATUS;/*0x09*/
	uint64_t A664_ES_CONFIG_STATUS;

    uint8_t  reserved3[70];

	uint16_t ADDRESS_FOR_A664_ES_DEV_ID;/*0x11*/
	uint64_t A664_ES_DEV_ID;

	uint16_t ADDRESS_FOR_A664_ES_FW_VER; /*0x12*/
	uint64_t A664_ES_FW_VER;

	uint16_t ADDRESS_FOR_A664_ES_PTP_STATUS; /*0x13*/
	uint64_t A664_ES_PTP_STATUS;

	uint16_t ADDRESS_FOR_A664_ES_TX_A_OUTGOING_COUNT; /*0x14*/
	uint64_t A664_ES_TX_A_OUTGOING_COUNT;

	uint16_t ADDRESS_FOR_A664_ES_TX_B_OUTGOING_COUNT; /*0x15*/
	uint64_t A664_ES_TX_B_OUTGOING_COUNT;

	uint16_t ADDRESS_FOR_A664_ES_RX_A_INCOMING_COUNT;/*0x16*/
	uint64_t A664_ES_RX_A_INCOMING_COUNT;

	uint16_t ADDRESS_FOR_A664_ES_RX_B_INCOMING_COUNT;/*0x17*/
	uint64_t A664_ES_RX_B_INCOMING_COUNT;

	uint16_t ADDRESS_FOR_A664_ES_RX_A_CRC_ERR_COUNT;/*0x18*/
	uint64_t A664_ES_RX_A_CRC_ERR_COUNT;

	uint16_t ADDRESS_FOR_A664_ES_RX_B_CRC_ERR_COUNT;/*0x19*/
	uint64_t A664_ES_RX_B_CRC_ERR_COUNT;

	uint16_t ADDRESS_FOR_A664_ES_PORT_A_STATUS; /*0x1A*/
	uint64_t A664_ES_PORT_A_STATUS;

	uint16_t ADDRESS_FOR_A664_ES_PORT_B_STATUS;/*0x1B*/
	uint64_t A664_ES_PORT_B_STATUS;

	uint16_t ADDRESS_FOR_A664_ES_PORT_SPEED; /*0x1C*/
	uint64_t A664_ES_PORT_SPEED;

    uint8_t  reserved4[20];

	uint16_t ADDRESS_FOR_A664_ES_RX_A_SEQ_ERR_COUNT; /*0x1F*/
	uint64_t A664_ES_RX_A_SEQ_ERR_COUNT;

	uint16_t ADDRESS_FOR_A664_ES_RX_A_LMIN_LMAX_DROP_COUNT; /*0x20*/
	uint64_t A664_ES_RX_A_LMIN_LMAX_DROP_COUNT;

	uint16_t ADDRESS_FOR_A664_ES_RX_A_VLID_DROP_COUNT; /*0x21*/
	uint64_t A664_ES_RX_A_VLID_DROP_COUNT;

	uint16_t ADDRESS_FOR_A664_ES_RX_A_NET_ERR_COUNT;/*0x22*/
	uint64_t A664_ES_RX_A_NET_ERR_COUNT;

    uint8_t  reserved5[30];

	uint16_t ADDRESS_FOR_A664_ES_RX_A_BE_INCOMING_COUNT;/*0x26*/
	uint64_t A664_ES_RX_A_BE_INCOMING_COUNT;

    uint8_t  reserved5_1[20];


	uint16_t ADDRESS_FOR_A664_ES_RX_B_SEQ_ERR_COUNT; /*0x29*/
	uint64_t A664_ES_RX_B_SEQ_ERR_COUNT;

	uint16_t ADDRESS_FOR_A664_ES_RX_B_LMIN_LMAX_DROP_COUNT;/*0x2A*/
	uint64_t A664_ES_RX_B_LMIN_LMAX_DROP_COUNT;

	uint16_t ADDRESS_FOR_A664_ES_RX_B_VLID_DROP_COUNT; /*0x2B*/
	uint64_t A664_ES_RX_B_VLID_DROP_COUNT;

	uint16_t ADDRESS_FOR_A664_ES_RX_B_NET_ERR_COUNT;/*0x2C*/
	uint64_t A664_ES_RX_B_NET_ERR_COUNT;

    uint8_t  reserved6[30];

	uint16_t ADDRESS_FOR_A664_ES_RX_B_BE_INCOMING_COUNT; /*0x30*/
	uint64_t A664_ES_RX_B_BE_INCOMING_COUNT;

    uint8_t  reserved7[130];

	uint16_t ADDRESS_FOR_A664_ES_RX_OUTGOING_COUNT; /*0x3E*/
	uint64_t A664_ES_RX_OUTGOING_COUNT;

    uint8_t  reserved8[30];

	uint16_t ADDRESS_FOR_A664_ES_RX_FIFO_OVERFLOW_COUNT;/*0x42*/
	uint64_t A664_ES_RX_FIFO_OVERFLOW_COUNT;

    uint8_t  reserved9[230];

	uint16_t ADDRESS_FOR_A664_ES_TEMPERATURE;/*0x5A*/
	uint64_t A664_ES_TEMPERATURE;

	uint16_t ADDRESS_FOR_A664_ES_VOLTAGE; /*0x5B*/
	uint64_t A664_ES_VOLTAGE;

    uint8_t  reserved10[10];

    uint16_t ADDRESS_FOR_A664_ES_BITSTREAM_UPDATE_STATUS; /*0x5D*/
	uint64_t A664_ES_BITSTREAM_UPDATE_STATUS;

    uint8_t  reserved11[10];

	uint16_t ADDRESS_FOR_A664_ES_MODE; /*0x5F*/
	uint64_t A664_ES_MODE;

	uint16_t ADDRESS_FOR_A664_ES_CONFIGURATION_ID; /*0x60*/
	uint64_t A664_ES_CONFIGURATION_ID;

	uint16_t ADDRESS_FOR_A664_ES_BIT_STATUS; /*0x61*/
	uint64_t A664_ES_BIT_STATUS;
    uint8_t  reserved12[20];

	uint8_t finish; // Finish byte indicates finish 0x42
}dtn_es_mon;

void print_binary(FILE *outfp,uint32_t n, int bytes)
{
    fprintf(outfp, "0b");
    for(int i = (bytes * 8) -1; i >= 0; i--)
    {
        fprintf(outfp, "%u", (n >> i) & 1);
    }
}

static int adcu_log_print_header(FILE **fp, int adcu_msg_chooser) {
    fprintf(*fp, "DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP,LOG PAYLOAD LENGTH,");
    if(0 == adcu_msg_chooser)
    {
        fprintf(*fp, "%s,", "device_id");
        fprintf(*fp, "%s,", "hm_report_type");
        fprintf(*fp, "%s,", "timetag_nsec");
        fprintf(*fp, "%s,", "msg_len");
        fprintf(*fp, "%s,", "dcu_type");
        fprintf(*fp, "%s,", "channel_config_id");
        fprintf(*fp, "%s,", "channel_vl_mapping_config_id");
        fprintf(*fp, "%s,", "m1553_status_0x0110_reserved_u4");
        fprintf(*fp, "%s,", "m1553_status_0x0110_rt_sub_addr_lut_rcv_status_u1");
        fprintf(*fp, "%s,", "m1553_status_0x0110_inst_list_rcv_status_u1");
        fprintf(*fp, "%s,", "m1553_status_0x0110_bus_type_u2");
        fprintf(*fp, "%s,", "m1553_status_0x0111_reserved_u4");
        fprintf(*fp, "%s,", "m1553_status_0x0111_rt_sub_addr_lut_rcv_status_u1");
        fprintf(*fp, "%s,", "m1553_status_0x0111_inst_list_rcv_status_u1");
        fprintf(*fp, "%s,", "m1553_status_0x0111_bus_type_u2");
        fprintf(*fp, "%s,", "m1553_inst_list_id_0x0110");
        fprintf(*fp, "%s,", "m1553_inst_list_id_0x0111");
        fprintf(*fp, "%s,", "m1553_lut_id_0x0110");
        fprintf(*fp, "%s,", "m1553_lut_id_0x0111");
        fprintf(*fp, "%s,", "ethernet_0x100_identifier_error");
        fprintf(*fp, "%s,", "ethernet_0x100_ethernet_mode");
        fprintf(*fp, "%s,", "ethernet_0x100_ethernet_speed");
        fprintf(*fp, "%s,", "ethernet_0x100_link_status");
        fprintf(*fp, "%s,", "ethernet_0x101_identifier_error");
        fprintf(*fp, "%s,", "ethernet_0x101_ethernet_mode");
        fprintf(*fp, "%s,", "ethernet_0x101_ethernet_speed");
        fprintf(*fp, "%s,", "ethernet_0x101_link_status");
        fprintf(*fp, "%s,", "config_flash_read_response");
        fprintf(*fp, "%s,", "config_flash_crc_reponse");
        fprintf(*fp, "%s,", "channel_vl_mapping_config_id_mismatch_u1");
        fprintf(*fp, "%s,", "channel_config_id_mismatch_u1");
        fprintf(*fp, "%s,", "heart_beat_received_u1");
        fprintf(*fp, "%s,", "major_frame_sync_u1");
        fprintf(*fp, "%s,", "structure_version_2nd_packet_u1");
        fprintf(*fp, "%s,", "structure_version_1st_packet_u1");

        for (int i = 0; i < ADCU_CBIT_REGISTER_COUNT; i++) {
            fprintf(*fp, "cbit_register_%d,", i);
        }

        fprintf(*fp, "%s,", "analog_hm_len");
        fprintf(*fp, "%s,", "discerete_hm_len");

        for (int i = 0; i < ADCU_ANALOG_RAW_REGISTER_COUNT; i++) {
            fprintf(*fp, "analog_raw_data_register_%d,", i);
        }
        int i;
        for (i = 0; i < ADCU_DISCRETE_RAW_REGISTER_COUNT-1; i++) {
            fprintf(*fp, "discrete_raw_data_register_%d,", i);
        }
        fprintf(*fp, "discrete_raw_data_register_%d\n", i);
    }
    else if(1 == adcu_msg_chooser)
    {
        fprintf(*fp, "%s,", "DCU Device ID");
        fprintf(*fp, "%s,", "HM Report Type");
        
        fprintf(*fp, "%s,", "Timetag");

        fprintf(*fp, "%s,", "Message Length");
        fprintf(*fp, "%s,", "DCU Type");
        fprintf(*fp, "%s,", "DCU Firmware Version/DCU Hardware Version");

        fprintf(*fp, "%s,", "DCU HW UID");

        fprintf(*fp, "%s,", "Channel Config ID");
        fprintf(*fp, "%s,", "Channel-VL Mapping Config ID");
        for(int i = 0; i < ADCU_PBIT_REGISTER_COUNT; i++) {
            fprintf(*fp, "pbit_registers[%d],", i);
        }
        for(int i = 0; i < ADCU_PBIT_REGISTER_COUNT; i++) {
            fprintf(*fp, "pbit_timeout[%d]", i);
            
            fprintf(*fp, "%s", (i == ADCU_PBIT_REGISTER_COUNT - 1) ? "\n" : ",");
        }
    }
    else if(2 == adcu_msg_chooser)
    {
        fprintf(*fp, "%s,", "device_id");
        fprintf(*fp, "%s,", "wr");
        fprintf(*fp, "%s,", "monitoring_type");
        fprintf(*fp, "%s,", "monitoring_length");
        fprintf(*fp, "%s,", "A664_ES_TX_INCOMING_COUNT");
        fprintf(*fp, "%s,", "A664_ES_TX_VLID_DROP_COUNT");
        fprintf(*fp, "%s,", "A664_ES_TX_LMIN_LMAX_DROP_COUNT");
        fprintf(*fp, "%s,", "A664_ES_TX_BE_INCOMING_COUNT");
        fprintf(*fp, "%s,", "A664_ES_TX_FIFO_OVERFLOW_COUNT");
        fprintf(*fp, "%s,", "A664_ES_TX_MONITOR_COUNT");
        fprintf(*fp, "%s,", "A664_ES_CONFIG_RB_COUNT");
        fprintf(*fp, "%s,", "A664_ES_CONFIG_STATUS");
        fprintf(*fp, "%s,", "A664_ES_DEV_ID");
        fprintf(*fp, "%s,", "A664_ES_FW_VER");
        fprintf(*fp, "%s,", "A664_ES_PTP_STATUS");
        fprintf(*fp, "%s,", "A664_ES_TX_A_OUTGOING_COUNT");
        fprintf(*fp, "%s,", "A664_ES_TX_B_OUTGOING_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_A_INCOMING_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_B_INCOMING_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_A_CRC_ERR_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_B_CRC_ERR_COUNT");
        fprintf(*fp, "%s,", "A664_ES_PORT_A_STATUS");
        fprintf(*fp, "%s,", "A664_ES_PORT_B_STATUS");
        fprintf(*fp, "%s,", "A664_ES_PORT_SPEED");
        fprintf(*fp, "%s,", "A664_ES_RX_A_SEQ_ERR_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_A_LMIN_LMAX_DROP_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_A_VLID_DROP_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_A_NET_ERR_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_A_BE_INCOMING_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_B_SEQ_ERR_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_B_LMIN_LMAX_DROP_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_B_VLID_DROP_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_B_NET_ERR_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_B_BE_INCOMING_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_OUTGOING_COUNT");
        fprintf(*fp, "%s,", "A664_ES_RX_FIFO_OVERFLOW_COUNT");
        fprintf(*fp, "%s,", "A664_ES_TEMPERATURE");
        fprintf(*fp, "%s,", "A664_ES_VOLTAGE");
        fprintf(*fp, "%s,", "A664_ES_BITSTREAM_UPDATE_STATUS");
        fprintf(*fp, "%s,", "A664_ES_MODE");
        fprintf(*fp, "%s,", "A664_ES_CONFIGURATION_ID");
        fprintf(*fp, "%s,", "A664_ES_BIT_STATUS");
        fprintf(*fp, "%s\n", "finish");
    }
}

static int adcu_cbit_log_parser(FILE *outfp, Cmsw_pl_adcu_cbit_msg_type *adcu)
{
    if (!outfp || !adcu) {
        return -1;
    }

    // Convert and print basic fields
    fprintf(outfp, "%u,", ntohs(adcu->device_id));
    fprintf(outfp, "%u,", adcu->hm_report_type);
    uint64_t timetag = ntohl(adcu->timetag_sec);
    timetag = timetag * 1000000000LLU;
    timetag += ntohl(adcu->timetag_nsec);
    fprintf(outfp, "%llu,", timetag);
    fprintf(outfp, "%u,", ntohs(adcu->msg_len));
    fprintf(outfp, "%u,", adcu->dcu_type);
    fprintf(outfp, "%u,", ntohl(adcu->channel_config_id));
    fprintf(outfp, "%u,", ntohl(adcu->channel_vl_mapping_config_id));

    // M1553 Status 0x0110
    fprintf(outfp, "%u,", adcu->m1553_status_0x0110_reserved_u4);
    fprintf(outfp, "%u,", adcu->m1553_status_0x0110_rt_sub_addr_lut_rcv_status_u1);
    fprintf(outfp, "%u,", adcu->m1553_status_0x0110_inst_list_rcv_status_u1);
    fprintf(outfp, "%u,", adcu->m1553_status_0x0110_bus_type_u2);

    // M1553 Status 0x0111
    fprintf(outfp, "%u,", adcu->m1553_status_0x0111_reserved_u4);
    fprintf(outfp, "%u,", adcu->m1553_status_0x0111_rt_sub_addr_lut_rcv_status_u1);
    fprintf(outfp, "%u,", adcu->m1553_status_0x0111_inst_list_rcv_status_u1);
    fprintf(outfp, "%u,", adcu->m1553_status_0x0111_bus_type_u2);

    // M1553 Inst List IDs
    fprintf(outfp, "%u,", ntohl(adcu->m1553_inst_list_id_0x0110));
    fprintf(outfp, "%u,", ntohl(adcu->m1553_inst_list_id_0x0111));

    // M1553 LUT IDs
    fprintf(outfp, "%u,", ntohl(adcu->m1553_lut_id_0x0110));
    fprintf(outfp, "%u,", ntohl(adcu->m1553_lut_id_0x0111));

    fprintf(outfp, "%u,", adcu->eth_status_0x100.identifier_error);
    fprintf(outfp, "%u,", adcu->eth_status_0x100.ethernet_mode_u1);
    fprintf(outfp, "%u,", adcu->eth_status_0x100.ethernet_speed_u1);
    fprintf(outfp, "%u,", adcu->eth_status_0x100.link_status_u1);

    fprintf(outfp, "%u,", adcu->eth_status_0x101.identifier_error);
    fprintf(outfp, "%u,", adcu->eth_status_0x101.ethernet_mode_u1);
    fprintf(outfp, "%u,", adcu->eth_status_0x101.ethernet_speed_u1);
    fprintf(outfp, "%u,", adcu->eth_status_0x101.link_status_u1);

    // Config mismatch flags and status bits
    fprintf(outfp, "%u,", adcu->config_flash_read_resp);
    fprintf(outfp, "%u,", adcu->config_flash_crc_reponse);
    fprintf(outfp, "%u,", adcu->channel_vl_mapping_config_id_mismatch_u1);
    fprintf(outfp, "%u,", adcu->channel_config_id_mismatch_u1);
    fprintf(outfp, "%u,", adcu->heart_beat_received_u1);
    fprintf(outfp, "%u,", adcu->major_frame_sync_u1);
    fprintf(outfp, "%u,", adcu->structure_version_2nd_packet_u1);
    fprintf(outfp, "%u,", adcu->structure_version_1st_packet_u1);

    // Print CBIT registers
    for (int i = 0; i < ADCU_CBIT_REGISTER_COUNT; ++i) {
        print_binary(outfp, ntohl(adcu->cbit_registers[i]),4);
        //fprintf(outfp, "%u", ntohl(adcu->cbit_registers[i]));
        if (i < ADCU_CBIT_REGISTER_COUNT - 1) {
            fprintf(outfp, ",");
        }
    }
    fprintf(outfp, ",");

    // Analog HM data length
    fprintf(outfp, "%u,", adcu->analog_hm_len);

    // Discrete HM data length
    fprintf(outfp, "%u,", adcu->discerete_hm_len);

    // Analog raw data registers
    for (int i = 0; i < ADCU_ANALOG_RAW_REGISTER_COUNT; i++) {
        fprintf(outfp, "%u,", ntohl(adcu->analog_raw_data_registers[i]));
    }

    // Discrete raw data registers
    for (int i = 0; i < ADCU_DISCRETE_RAW_REGISTER_COUNT; i++) {
        print_binary(outfp, adcu->discrete_raw_data_registers[i],1);
        //fprintf(outfp, "%u", adcu->discrete_raw_data_registers[i]);
        if (i < ADCU_DISCRETE_RAW_REGISTER_COUNT - 1) {
            fprintf(outfp, ",");
        }
    }

    fprintf(outfp, "\n");
    return 0;
}

static int adcu_pbit_log_parser(FILE *outfp, Cmsw_pl_adcu_pbit_msg_type *adcu)
{
    if (!outfp || !adcu) {
        return -1;
    }
    fprintf(outfp, "%u,", ntohs(adcu->device_id));
    fprintf(outfp, "%u,", adcu->hm_report_type);

    uint64_t timetag_64 = (
        ((uint64_t)adcu->timetag[0] << 56) |
        ((uint64_t)adcu->timetag[1] << 48) |
        ((uint64_t)adcu->timetag[2] << 40) |
        ((uint64_t)adcu->timetag[3] << 32) |
        ((uint64_t)adcu->timetag[4] << 24) |
        ((uint64_t)adcu->timetag[5] << 16) |
        ((uint64_t)adcu->timetag[6] << 8)  |
        ((uint64_t)adcu->timetag[7]));

    fprintf(outfp, "%u,", timetag_64);

    fprintf(outfp, "%u,", ntohs(adcu->msg_len));
    fprintf(outfp, "%u,", adcu->dcu_type);

    adcu->dcu_fw_hw_version = ntohl(adcu->dcu_fw_hw_version);

    uint8_t dcu_fw_vers_major =  (adcu->dcu_fw_hw_version & 0xF0000000) >> 28;
	uint8_t dcu_fw_vers_minor =  (adcu->dcu_fw_hw_version & 0x0F000000U) >> 24;
	uint8_t dcu_fw_vers_bugfix = (adcu->dcu_fw_hw_version & 0x00F00000U) >> 20;
    
    uint16_t dcu_hw_vers_moresignificant = (adcu->dcu_fw_hw_version >> 10) & 0x3FF;
    uint16_t dcu_hw_vers_lessersignificant = (adcu->dcu_fw_hw_version ) & 0x3FF;

    fprintf(outfp, "%u.%u.%u/",dcu_fw_vers_major, dcu_fw_vers_minor, dcu_fw_vers_bugfix );
    fprintf(outfp, "%u.%u,",dcu_hw_vers_moresignificant, dcu_hw_vers_lessersignificant );

    uint64_t dcu_hw_uid_64 = (
        ((uint64_t)adcu->dcu_hw_uid[0] << 56) |
        ((uint64_t)adcu->dcu_hw_uid[1] << 48) |
        ((uint64_t)adcu->dcu_hw_uid[2] << 40) |
        ((uint64_t)adcu->dcu_hw_uid[3] << 32) |
        ((uint64_t)adcu->dcu_hw_uid[4] << 24) |
        ((uint64_t)adcu->dcu_hw_uid[5] << 16) |
        ((uint64_t)adcu->dcu_hw_uid[6] << 8)  |
        ((uint64_t)adcu->dcu_hw_uid[7]));

    uint64_t dcu_hw_unique_id_res    = (dcu_hw_uid_64 >> 52) & 0x3FF;
    uint64_t dcu_hw_unique_id_bilgem = (dcu_hw_uid_64 >> 16) & 0xFFFFFFFFF;
    uint64_t dcu_hw_unique_id_serial = (dcu_hw_uid_64) & 0xFFFF;

    fprintf(outfp, "%llu.%llu.%llu,", 
        dcu_hw_unique_id_res, 
        dcu_hw_unique_id_bilgem, 
        dcu_hw_unique_id_serial);
    
    fprintf(outfp, "%u,", ntohl(adcu->channel_config_id));
    fprintf(outfp, "%u,", ntohl(adcu->channel_vl_mapping_config_id));

    for (int i = 0; i < ADCU_PBIT_REGISTER_COUNT; i++) {
        fprintf(outfp, "%u,", ntohl(adcu->pbit_registers[i]));
    }

    for (int i = 0; i < ADCU_PBIT_REGISTER_COUNT; i++) {
        fprintf(outfp, "%u", ntohl(adcu->pbit_timeout[i]));
        
        fprintf(outfp, "%s", (i == ADCU_PBIT_REGISTER_COUNT - 1) ? "\n" : ",");
    }
    
    return 0;
}

static int adcu_dtnes_log_parser(FILE *outfp, dtn_es_mon *dtnes)
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
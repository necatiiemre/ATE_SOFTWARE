#pragma once
#include <stdint.h>

/**
 * @brief Maximum number of VLs that can be configured on DTN Switch.
 */
#define A664_SW_MAX_VL_COUNT 	        16384

/**
 * @brief Maximum number of ports that can be configured on DTN Switch.
 */
#define A664_SW_MAX_PORT_COUNT 	        12

/**
 * @brief Maximum number of MACs that can be configured on DTN Switch.
 */
#define A664_SW_MAX_MAC_BASED_COUNT 	12

/**
 * @brief This structure contains the DTN Switch Status monitoring parameters.
 */
typedef struct __attribute__((packed)){
	/**
	 * @brief Total number of packets DTN Switch sent.
	 */
	uint64_t A664_SW_TOT_TX_DATA_NUM;

	/**
	 * @brief Total number of packets DTN Switch received.
	 */
	uint64_t A664_SW_TOT_RX_DATA_NUM;

	/**
	 * @brief DTN SW F/O Transceiver temperature.
	 */
	uint64_t A664_SW_TRANSCEIVER_TEMP;

	/**
	 * @brief DTN SW shared F/O Transceiver temperature.
	 */
	uint64_t A664_SW_SHARED_TRANSCEIVER_TEMP;

	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint32_t A664_SW_PADDING :23;

	/**
	 * @brief Heart-beat signal of DTN Switch.
	 */
	uint32_t A664_SW_HEARTBEAT :1;

	/**
	 * @brief Device ID of DTN Switch.
	 */
	uint32_t A664_SW_DEVICE_ID_MSB:8;
	uint8_t A664_SW_DEVICE_ID_LSB;

	/**
	 * @brief Number of physical ports DTN Switch has.
	 */
	uint8_t A664_SW_PORT_NUM;

	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint8_t A664_SW_PADDING1 :7;

	/**
	 * @brief Status of the token bucket activation.
	 */
	uint8_t A664_SW_TOKEN_BUCKET_STATUS :1;

	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint64_t A664_SW_PADDING2 :2;

	/**
	 * @brief Current operation mode of DTN Switch.
	 */
	uint64_t A664_SW_CURRENT_MODE :6;

	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint64_t A664_SW_PADDING3 :47;

	/**
	 * @brief This field indicates the hardware vendor of the DTN ES Device.
	 */
	uint64_t A664_SW_VENDOR_TYPE :1;

	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint64_t A664_SW_PADDING4 :7;

	/**
	 * @brief Status of the automatic MAC list update activation.
	 */
	uint64_t A664_SW_AUTOMAC_UPDATE_STATUS :1;

	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint8_t A664_SW_PADDING5 :7;

	/**
	 * @brief Status of the upstream mode activation.
	 */
	uint8_t A664_SW_UPSTREAM_MODE_STATUS :1;

	/**
	 * @brief Firmware version of DTN Switch.
	 */
	uint64_t A664_SW_VERSION;

	/**
	 * @brief Firmware version of the DTN End System inside the DTN Switch.
	 */
	uint64_t A664_SW_ES_VERSION;

	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint64_t A664_SW_PADDING6;

	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint64_t A664_SW_PADDING7;

	/**
	 * @brief Nano-seconds portion of PTP Time of Day.
	 */
	uint64_t A664_SW_TIME_OF_DAY_NS;

	/**
	 * @brief Seconds portion of PTP Time of Day.
	 */
	uint64_t A664_SW_TIME_OF_DAY_S;

	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint64_t A664_SW_PADDING8;

	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint64_t A664_SW_PADDING9;

	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint64_t A664_SW_PADDING10;

	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint32_t A664_SW_PADDING11;

	/**
	 * @brief Internal Voltage value of DTN Switch.
	 */
	uint16_t A664_SW_INTERNAL_VOLTAGE;

	/**
	 * @brief Hardware Temperature value of DTN Switch.
	 */
	uint16_t A664_SW_TEMPERATURE;

	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint16_t A664_SW_PADDING12;

	/**
	 * @brief ID of the configuration applied to DTN Switch.
	 */
	uint16_t A664_SW_CONFIG_ID;

	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint32_t A664_SW_PADDING13;
} a664SWMonitoringStatus;

/**
 * @brief This structure contains the DTN Switch Status monitoring parameters.
 */
typedef struct a664SWMonitoringPort {
	/**
	 * @brief ID of the DTN Switch Port.
	 */
	uint64_t A664_SW_PORT_ID;

	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint64_t A664_SW_PADDING : 58;

	/**
	 * @brief Built-in test status of DTN Switch.
	 */
	uint64_t A664_SW_BIT_STATUS : 2;

	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint64_t A664_SW_PADDING2 : 3;

	/**
	 * @brief Liveliness indication DTN Switch Port.
	 */
	uint64_t A664_SW_PORT_LINK : 1;

	/**
	 * @brief Total number of dropped packets due to CRC error on the port.
	 */
	uint64_t A664_SW_CRC_ERR_CNT;

	/**
	 * @brief Total number of dropped packets due to alignment error on the port.
	 */
	uint64_t A664_SW_ALIGNMENT_ERR_CNT;

	/**
	 * @brief Total number of dropped packets due to minimum allowed ARINC664 length error on the port.
	 */
	uint64_t A664_SW_LMIN_ERR_CNT;

	/**
	 * @brief Total number of dropped packets due to maximum allowed ARINC664  length error on the port.
	 */
	uint64_t A664_SW_LMAX_ERR_CNT;

	/**
	 * @brief Total number of dropped packets due to configured minimum allowed length error on the port.
	 */
	uint64_t A664_SW_VLMIN_ERR_CNT;

	/**
	 * @brief Total number of dropped packets due to configured maximum allowed length error on the port.
	 */
	uint64_t A664_SW_VLMAX_ERR_CNT;

	/**
	 * @brief Total number of dropped packets due to MAC error on the port.
	 */
	uint64_t A664_SW_MAC_ERR_CNT;

	/**
	 * @brief Total number of dropped packets due to token bucket error on the port.
	 */
	uint64_t A664_SW_TOKEN_ERR_CNT;

	/**
	 * @brief Total number of Best-Effort frames transmitted from the port.
	 */
	uint64_t A664_SW_BE_FRAME;

	/**
	 * @brief Total number of ARINC664 packets transmitted from the port.
	 */
	uint64_t A664_SW_TX_FRAME_CNT;

	/**
	 * @brief Total number of ARINC664 packets received on the port.
	 */
	uint64_t A664_SW_RX_FRAME_CNT;

	/**
	 * @brief Total number of packets dropped on the port due to coming from another VL to which
	 * the port has not assigned.
	 */
	uint64_t A664_SW_VL_RX_PORT_ERR;

	/**
	 * @brief Total number of packets dropped on the port due to time between the arrival of a packet on the
     * input port of the DTN Switch and the exit of the packet on the output port has exceeded the allowed
     * maximum time.
	 */
	uint64_t A664_SW_MAX_DELAY_ERR_CNT;

	/**
	 * @brief Total number of packets dropped on the port due to input buffer overflow.
	 */
	uint64_t A664_SW_IN_PORT_Q_OVERFLOW_CNT;

	/**
	 * @brief Total number of packets dropped on the port due to packet coming from an undefined VL.
	 */
	uint64_t A664_SW_UNDEF_VL_ERR;

	/**
	 * @brief Total number of packets dropped on the port due to Best-Effort packet coming from undefined
	 * device.
	 */
	uint64_t A664_SW_UNDEF_BE_ERR;

	/**
	 * @brief Total number of dropped packets on port due to high priority queue buffer overflow.
	 */
	uint64_t A664_SW_HP_Q_OVERFLOW;

	/**
	 * @brief Total number of dropped packets on port due to low priority queue buffer overflow.
	 */
	uint64_t A664_SW_LP_Q_OVERFLOW;

	/**
	 * @brief Total number of dropped packets on port due to best effort queue buffer overflow.
	 */
	uint64_t A664_SW_BE_Q_OVERFLOW;

	/**
	 * @brief Configured maximum allowed delay of DTN Switch port.
	 */
	uint64_t A664_SW_CONF_MAX_DELAY_PARAM;

	/**
	 * @brief Configured port speed of DTN Switch port.
	 */
	uint64_t A664_SW_PORT_SPEED;
}a664SWMonitoringPort;

/**
 * @brief This is the main DTN Switch monitoring structure. This structure contains DTN Switch status monitoring
 * structure and DTN Switch port monitoring structure for every DTN Switch port.
 */
typedef struct tA664SWMonitoring{
	/**
	 * @brief This field contains information related to DTN Switch such as health-monitoring, statistics, operation, configuration status etc..
	 */
	a664SWMonitoringStatus status;

	/**
	 * @brief This field contains information and statistics about the DTN Switch Port.
	 */
	a664SWMonitoringPort port[A664_SW_MAX_PORT_COUNT];
} tA664SWMonitoring;

static int dsm_dtnsw_log_print_header(FILE **fp) {
	fprintf(*fp,  "DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP,LOG PAYLOAD LENGTH,");

	fprintf(*fp, "A664_SW_TOT_TX_DATA_NUM,");
	fprintf(*fp, "A664_SW_TOT_RX_DATA_NUM,");
	fprintf(*fp, "A664_SW_TRANSCEIVER_TEMP,");
	fprintf(*fp, "A664_SW_SHARED_TRANSCEIVER_TEMP,");
	fprintf(*fp, "A664_SW_HEARTBEAT,");
	fprintf(*fp, "A664_SW_DEVICE_ID,");
	fprintf(*fp, "A664_SW_PORT_NUM,");
	fprintf(*fp, "A664_SW_TOKEN_BUCKET_STATUS,");
	fprintf(*fp, "A664_SW_CURRENT_MODE,");
	fprintf(*fp, "A664_SW_VENDOR_TYPE,");
	fprintf(*fp, "A664_SW_AUTOMAC_UPDATE_STATUS,");
	fprintf(*fp, "A664_SW_UPSTREAM_MODE_STATUS,");
	fprintf(*fp, "A664_SW_VERSION,");
	fprintf(*fp, "A664_SW_ES_VERSION,");
	fprintf(*fp, "A664_SW_TIME_OF_DAY_NS,");
	fprintf(*fp, "A664_SW_TIME_OF_DAY_S,");
	fprintf(*fp, "A664_SW_INTERNAL_VOLTAGE,");
	fprintf(*fp, "A664_SW_TEMPERATURE,");
	fprintf(*fp, "A664_SW_CONFIG_ID,");

	for(int i = 0; i < 12; i++) // Note: Fixed comma to semicolon
	{
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_PORT_ID");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_BIT_STATUS");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_PORT_LINK");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_CRC_ERR_CNT");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_ALIGNMENT_ERR_CNT");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_LMIN_ERR_CNT");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_LMAX_ERR_CNT");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_VLMIN_ERR_CNT");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_VLMAX_ERR_CNT");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_MAC_ERR_CNT");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_TOKEN_ERR_CNT");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_BE_FRAME");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_TX_FRAME_CNT");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_RX_FRAME_CNT");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_VL_RX_PORT_ERR");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_MAX_DELAY_ERR_CNT");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_IN_PORT_Q_OVERFLOW_CNT");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_UNDEF_VL_ERR");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_UNDEF_BE_ERR");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_HP_Q_OVERFLOW");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_LP_Q_OVERFLOW");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_BE_Q_OVERFLOW");
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_CONF_MAX_DELAY_PARAM");
		if(12 == (i+1))
		fprintf(*fp, "PORT[%d].%s\n", i, "A664_SW_PORT_SPEED");
		else
		fprintf(*fp, "PORT[%d].%s,", i, "A664_SW_PORT_SPEED");
	}
}

static int dsm_dtnsw_log_parser(FILE *outfp, tA664SWMonitoring *dtnswmon) {
    // Convert 64-bit fields from network byte order to host byte order
    dtnswmon->status.A664_SW_TOT_TX_DATA_NUM = ntohll(dtnswmon->status.A664_SW_TOT_TX_DATA_NUM);
    dtnswmon->status.A664_SW_TOT_RX_DATA_NUM = ntohll(dtnswmon->status.A664_SW_TOT_RX_DATA_NUM);
    dtnswmon->status.A664_SW_TRANSCEIVER_TEMP = ntohll(dtnswmon->status.A664_SW_TRANSCEIVER_TEMP);
    dtnswmon->status.A664_SW_SHARED_TRANSCEIVER_TEMP = ntohll(dtnswmon->status.A664_SW_SHARED_TRANSCEIVER_TEMP);
    dtnswmon->status.A664_SW_VERSION = ntohll(dtnswmon->status.A664_SW_VERSION);
    dtnswmon->status.A664_SW_ES_VERSION = ntohll(dtnswmon->status.A664_SW_ES_VERSION);
    dtnswmon->status.A664_SW_TIME_OF_DAY_NS = ntohll(dtnswmon->status.A664_SW_TIME_OF_DAY_NS);
    dtnswmon->status.A664_SW_TIME_OF_DAY_S = ntohll(dtnswmon->status.A664_SW_TIME_OF_DAY_S);
    dtnswmon->status.A664_SW_INTERNAL_VOLTAGE = ntohs(dtnswmon->status.A664_SW_INTERNAL_VOLTAGE);
    dtnswmon->status.A664_SW_TEMPERATURE = ntohs(dtnswmon->status.A664_SW_TEMPERATURE);
    dtnswmon->status.A664_SW_CONFIG_ID = ntohs(dtnswmon->status.A664_SW_CONFIG_ID);

	uint16_t dev_id = (uint16_t) dtnswmon->status.A664_SW_DEVICE_ID_MSB << 8 + dtnswmon->status.A664_SW_DEVICE_ID_LSB;
	
	for(int i = 0; i < A664_SW_MAX_PORT_COUNT; i++)
	{
		dtnswmon->port[i].A664_SW_PORT_ID = ntohll(dtnswmon->port[i].A664_SW_PORT_ID);
		dtnswmon->port[i].A664_SW_CRC_ERR_CNT = ntohll(dtnswmon->port[i].A664_SW_CRC_ERR_CNT);
		dtnswmon->port[i].A664_SW_ALIGNMENT_ERR_CNT = ntohll(dtnswmon->port[i].A664_SW_ALIGNMENT_ERR_CNT);
		dtnswmon->port[i].A664_SW_LMIN_ERR_CNT = ntohll(dtnswmon->port[i].A664_SW_LMIN_ERR_CNT);
		dtnswmon->port[i].A664_SW_LMAX_ERR_CNT = ntohll(dtnswmon->port[i].A664_SW_LMAX_ERR_CNT);
		dtnswmon->port[i].A664_SW_VLMIN_ERR_CNT = ntohll(dtnswmon->port[i].A664_SW_VLMIN_ERR_CNT);
		dtnswmon->port[i].A664_SW_VLMAX_ERR_CNT = ntohll(dtnswmon->port[i].A664_SW_VLMAX_ERR_CNT);
		dtnswmon->port[i].A664_SW_MAC_ERR_CNT = ntohll(dtnswmon->port[i].A664_SW_MAC_ERR_CNT);
		dtnswmon->port[i].A664_SW_TOKEN_ERR_CNT = ntohll(dtnswmon->port[i].A664_SW_TOKEN_ERR_CNT);
		dtnswmon->port[i].A664_SW_BE_FRAME = ntohll(dtnswmon->port[i].A664_SW_BE_FRAME);
		dtnswmon->port[i].A664_SW_TX_FRAME_CNT = ntohll(dtnswmon->port[i].A664_SW_TX_FRAME_CNT);
		dtnswmon->port[i].A664_SW_RX_FRAME_CNT = ntohll(dtnswmon->port[i].A664_SW_RX_FRAME_CNT);
		dtnswmon->port[i].A664_SW_VL_RX_PORT_ERR = ntohll(dtnswmon->port[i].A664_SW_VL_RX_PORT_ERR);
		dtnswmon->port[i].A664_SW_MAX_DELAY_ERR_CNT = ntohll(dtnswmon->port[i].A664_SW_MAX_DELAY_ERR_CNT);
		dtnswmon->port[i].A664_SW_IN_PORT_Q_OVERFLOW_CNT = ntohll(dtnswmon->port[i].A664_SW_IN_PORT_Q_OVERFLOW_CNT);
		dtnswmon->port[i].A664_SW_UNDEF_VL_ERR = ntohll(dtnswmon->port[i].A664_SW_UNDEF_VL_ERR);
		dtnswmon->port[i].A664_SW_UNDEF_BE_ERR = ntohll(dtnswmon->port[i].A664_SW_UNDEF_BE_ERR);
		dtnswmon->port[i].A664_SW_HP_Q_OVERFLOW = ntohll(dtnswmon->port[i].A664_SW_HP_Q_OVERFLOW);
		dtnswmon->port[i].A664_SW_LP_Q_OVERFLOW = ntohll(dtnswmon->port[i].A664_SW_LP_Q_OVERFLOW);
		dtnswmon->port[i].A664_SW_BE_Q_OVERFLOW = ntohll(dtnswmon->port[i].A664_SW_BE_Q_OVERFLOW);
		dtnswmon->port[i].A664_SW_CONF_MAX_DELAY_PARAM = ntohll(dtnswmon->port[i].A664_SW_CONF_MAX_DELAY_PARAM);
		dtnswmon->port[i].A664_SW_PORT_SPEED = ntohll(dtnswmon->port[i].A664_SW_PORT_SPEED);
	}


    // Log all fields
    fprintf(outfp, "%llu,", dtnswmon->status.A664_SW_TOT_TX_DATA_NUM);
    fprintf(outfp, "%llu,", dtnswmon->status.A664_SW_TOT_RX_DATA_NUM);
    fprintf(outfp, "%llu,", dtnswmon->status.A664_SW_TRANSCEIVER_TEMP);
    fprintf(outfp, "%llu,", dtnswmon->status.A664_SW_SHARED_TRANSCEIVER_TEMP);
    fprintf(outfp, "%u,", dev_id);
    fprintf(outfp, "%u,", dtnswmon->status.A664_SW_PORT_NUM);
    fprintf(outfp, "%u,", dtnswmon->status.A664_SW_TOKEN_BUCKET_STATUS);
    fprintf(outfp, "%u,", dtnswmon->status.A664_SW_CURRENT_MODE);
    fprintf(outfp, "%u,", dtnswmon->status.A664_SW_VENDOR_TYPE);
    fprintf(outfp, "%u,", dtnswmon->status.A664_SW_AUTOMAC_UPDATE_STATUS);
    fprintf(outfp, "%u,", dtnswmon->status.A664_SW_UPSTREAM_MODE_STATUS);
    fprintf(outfp, "%llu,", dtnswmon->status.A664_SW_VERSION);
    fprintf(outfp, "%llu,", dtnswmon->status.A664_SW_ES_VERSION);
    fprintf(outfp, "%llu,", dtnswmon->status.A664_SW_TIME_OF_DAY_NS);
    fprintf(outfp, "%llu,", dtnswmon->status.A664_SW_TIME_OF_DAY_S);
    fprintf(outfp, "%u,", dtnswmon->status.A664_SW_INTERNAL_VOLTAGE);
    fprintf(outfp, "%u,", dtnswmon->status.A664_SW_TEMPERATURE);
    fprintf(outfp, "%u,", dtnswmon->status.A664_SW_CONFIG_ID);

    // Log heartbeat (as 0 or 1)
    fprintf(outfp, "%u,", dtnswmon->status.A664_SW_HEARTBEAT);

	for(int i = 0; i < A664_SW_MAX_PORT_COUNT; i++)
	{
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_PORT_ID);
		fprintf(outfp, "%u,", dtnswmon->port[i].A664_SW_BIT_STATUS);
		fprintf(outfp, "%u,", dtnswmon->port[i].A664_SW_PORT_LINK);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_CRC_ERR_CNT);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_ALIGNMENT_ERR_CNT);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_LMIN_ERR_CNT);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_LMAX_ERR_CNT);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_VLMIN_ERR_CNT);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_VLMAX_ERR_CNT);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_MAC_ERR_CNT);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_TOKEN_ERR_CNT);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_BE_FRAME);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_TX_FRAME_CNT);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_RX_FRAME_CNT);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_VL_RX_PORT_ERR);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_MAX_DELAY_ERR_CNT);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_IN_PORT_Q_OVERFLOW_CNT);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_UNDEF_VL_ERR);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_UNDEF_BE_ERR);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_HP_Q_OVERFLOW);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_LP_Q_OVERFLOW);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_BE_Q_OVERFLOW);
		fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_CONF_MAX_DELAY_PARAM);

		if(12 == (i+1))
		{
			fprintf(outfp, "%llu\n", dtnswmon->port[i].A664_SW_PORT_SPEED);
		}
		else
		{
			fprintf(outfp, "%llu,", dtnswmon->port[i].A664_SW_PORT_SPEED);
		}
	}	
    return 0;
}


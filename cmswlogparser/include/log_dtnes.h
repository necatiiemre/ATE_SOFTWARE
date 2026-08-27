#pragma once

#if 0
/**
 * @file a664ESConfigDefs.h
 * @brief This file contains the DTN End System Configuration and Monitoring structures/parameters and definitions.
 *
 */

#ifndef ES_CONFIGURATION_A664ESCONFIGDEFS_H_
#define ES_CONFIGURATION_A664ESCONFIGDEFS_H_

#include <gzisSync.h>
#include <gzisTypes.h>
#include <arinc653/inner/apexTypes.h>

/**
 * @brief Maximum allowed number of TX VLs that can be configured in the DTN End System.
 */
#define A664_MAX_ALLOWED_TX_SIDE_VL_COUNT	128

/**
 * @brief Maximum allowed number of RX VLs that can be configured in the DTN End System.
 */
#define A664_MAX_ALLOWED_RX_SIDE_VL_COUNT	512

/**
 * @brief Maximum allowed number of ports per Queuing VL that can be used by the user.
 *
 * @details VL Port configuration is a driver-side configuration rather than DTN End System
 * device configuration. VL ports are multiple ARINC653 ports defined in GzIS653.xml by user.
 *
 *
 */
#define A664_MAX_VL_PORT_CNT                4U

/**
 * @brief Physical port count of DTN End System.
 * @details Each port is connected to a DTN Network. One to Network A, other to Network B.
 */
#define A664_ES_PORT_COUNT                  2U

/**
 * @brief This enumeration is used when selecting the network in which a VL operates in.
 */
typedef enum eA664NetworkInterface
{
	/**
	 * @brief Disable both DTN A and B networks.
	 */
	A664_NET_BOTH_DISABLE = 0,

	/**
	 * @brief DTN A Network.
	 */
	A664_NET_A = 1,

	/**
	 * @brief DTN B Network.
	 */
	A664_NET_B = 2,

	/**
	 * @brief Enable both DTN A and B Networks.
	 */
	A664_NET_A_B_BOTH = 3
} eA664NetworkInterface;

/**
 * @brief This enumeration is used to select the Band Allocation Gap (BAG) value between transmitted packets
 * for DTN ES TX VL.
 */
typedef enum eA664BAG
{
	/**
	 * @brief BAG: 1 milli-seconds.
	 */
	A664_BAG_1_MS   = 0,

	/**
	 * @brief BAG: 2 milli-seconds.
	 */
	A664_BAG_2_MS   = 1,

	/**
	 * @brief BAG: 4 milli-seconds.
	 */
	A664_BAG_4_MS   = 2,

	/**
	 * @brief BAG: 8 milli-seconds.
	 */
	A664_BAG_8_MS   = 3,

	/**
	 * @brief BAG: 16 milli-seconds.
	 */
	A664_BAG_16_MS  = 4,

	/**
	 * @brief BAG: 32 milli-seconds.
	 */
	A664_BAG_32_MS  = 5,

	/**
	 * @brief BAG: 64 milli-seconds.
	 */
	A664_BAG_64_MS  = 6,

	/**
	 * @brief BAG: 128 milli-seconds.
	 */
	A664_BAG_128_MS = 7,

	/**
	 * @brief BAG: No BAG.
	 */
	A664_NO_BAG     = 15
} eA664BAG;

/**
 * @brief This enumeration is used to select the Monitoring Period for the Feedback VL
 * for DTN ES TX VL.
 */
typedef enum e_dtn_mon_period
{
	/**
	 * @brief MONITORING PERIOD: 1 milli-seconds.
	 */
	DTN_MON_PERIOD_1_MS   = 0,

	/**
	 * @brief MONITORING PERIOD: 2 milli-seconds.
	 */
	DTN_MON_PERIOD_2_MS   = 1,

	/**
	 * @brief MONITORING PERIOD: 4 milli-seconds.
	 */
	DTN_MON_PERIOD_4_MS   = 2,

	/**
	 * @brief MONITORING PERIOD: 8 milli-seconds.
	 */
	DTN_MON_PERIOD_8_MS   = 3,

	/**
	 * @brief MONITORING PERIOD: 16 milli-seconds.
	 */
	DTN_MON_PERIOD_16_MS  = 4,

	/**
	 * @brief MONITORING PERIOD: 32 milli-seconds.
	 */
	DTN_MON_PERIOD_32_MS  = 5,

	/**
	 * @brief MONITORING PERIOD: 64 milli-seconds.
	 */
	DTN_MON_PERIOD_64_MS  = 6,

	/**
	 * @brief MONITORING PERIOD: 128 milli-seconds.
	 */
	DTN_MON_PERIOD_MS = 7,
} e_dtn_mon_period;

/**
 * @brief Communication type of the ARINC653 port assigned to DTN ES VL.
 */
typedef enum eA653ComPortType
{
  /**
  * @brief Queuing port type.
  */
  A653_COMPORT_SAMPLING = 0,

  /**
  * @brief Sampling port type.
  */
  A653_COMPORT_QUEUING = 1,
} eA653ComPortType;

/**
 * @brief Direction of the ARINC653 port assigned to a DTN VL.
 */
typedef enum eA653ComPortDirection
{
	/**
	* @brief Source (TX VL).
	*/
	A653_COMPORT_SOURCE = 0,

	/**
	* @brief Destination (RX VL).
	*/
	A653_COMPORT_DESTINATION = 1
} eA653ComPortDirection;

/**
 * @brief This enumeration is used for selecting which type of configuration is going to be applied
 * to DTN ES, initial configuration or re-configuration?
 */
typedef enum eA664ConfigType
{
	/**
	 * @brief Initial configuration.
	 */
	A664_CONFIG_INIT = 0,

	/**
	 * @brief Re-configuration.
	 */
	A664_CONFIG_RECONF = 1
} eA664ConfigType;

/**
 * @brief This enumeration is used for selecting the link speed of the DTN End System network ports.
 */
typedef enum eA664ESPortSpeed {
	/**
	 * @brief 10 mbps.
	 */
	MBPS_10 = 0,

	/**
	 * @brief 100 mbps.
	 */
	MBPS_100 = 1,

	/**
	 * @brief 1 gbps.
	 */
	MBPS_1000 = 2
} eA664ESPortSpeed;

/**
 * @brief This enumeration is used for selecting the communication mode of a DTN End System port.
 */
typedef enum eA664ESPortType {
  /**
   * @brief ARINC664 mode.
   */
	ES_A664 = 0,

  /**
   * @brief Best-Effort mode.
   */
	ES_BEST_EFFORT = 1,

  /**
   * @brief Both ARINC664 and Best-Effort modes.
   */
	ES_BOTH_A664_BE = 2,
} eA664ESPortType;

/**
 * @brief The network in which the DTN End System will be operating.
 */
typedef enum eA664ESNetworkType{
	/**
	* @brief Network A.
	*/
	ES_A = 0xA,

	/**
	* @brief Network B.
	*/
	ES_B = 0xB
} eA664ESNetworkType;

/**
 * @brief Type of the configured DTN End System.
 */
typedef enum eA664ESDevType{
	/**
	 * @brief Stand-alone DTN End System.
	 */
	ES_ES = 0,

	/**
	 * @brief DTN End System of DTN Switch.
	 */
	ES_SW_ES = 1,
} eA664ESDevType;

/**
 * @brief This structure contains the DTN End System monitoring parameters.
 */
typedef struct tA664ESMonitoring
{
	/**
	 * @brief Hardware firmware version.
	 */
	uint8_gt A664_ES_FW_VER;

	/**
	 * @brief Device ID of DTN End System.
	 */
	uint8_gt A664_ES_DEV_ID;

	/**
	 * @brief Current operation mode of DTN End System.
	 */
	uint8_gt A664_ES_MODE;

	/**
	 * @brief Configuration ID of DTN End System.
	 */
	uint8_gt A664_ES_CONFIG_ID;

	/**
	 * @brief Result of the built-in test of DTN Hardware.
	 */
	uint8_gt A664_ES_BIT_STATUS;

	/**
 	 * @brief Status of DTN End System Configuration. If it's value is 3, that means configuration is successfully applied.
	 */
	uint8_gt A664_ES_CONFIG_STATUS;

	/**
	 * @brief DTN Driver Configuration Status
	 *
	 * This field uses bit-mapped encoding to indicate configuration completion status:
	 * - Bit 0 (LSB): End System (ES) configuration status
	 * - Bit 1: Switch (SW) configuration status
	 * - Bit 2: Switch-End System (SW-ES) configuration status
	 *
	 * Each bit value of '1' indicates that the corresponding configuration is complete.
	 */
	uint1_gt A664_BSP_CONFIG_STATUS;

	/**
	 * @brief ID of the PTP configuration.
	 */
	uint2_gt A664_PTP_CONFIG_ID;

	/**
	 * @brief PTP device type. 0: Slave, 1: Master, 3: Grand Master.
	 */
	uint1_gt A664_PTP_DEVICE_TYPE;

	/**
	 * @brief Status of PTP Rate-Correction. 0: disabled, 1: enabled.
	 */
	uint1_gt A664_PTP_RC_STATUS;

	/**
	 * @brief PTP synchronization status of DTN End System’s Port A. This parameter is valid for PTP Slave.
	 */
	uint1_gt A664_PTP_PORT_A_SYNC;

	/**
	 * @brief PTP synchronization status of DTN End System’s Port A. This parameter is valid for PTP Slave.
	 */
	uint1_gt A664_PTP_PORT_B_SYNC;

	/**
	 * @brief Synchronization VL ID of PTP Master device. This parameter is valid for PTP Master/PTP Grand-master devices.
	 */
	uint2_gt A664_PTP_SYNC_VL_ID;

	/**
	 * @brief Request VL ID of PTP Slave device. This parameter is valid for PTP Slave.
	 */
	uint2_gt A664_PTP_REQ_VL_ID;

	/**
	 * @brief Response VL ID of PTP Slave device. This parameter is valid for PTP Slave.
	 */
	uint2_gt A664_PTP_RES_VL_ID;

	/**
	 * @brief This parameter indicates from which network the PTP TOD information is received.
	 */
	uint1_gt A664_PTP_TOD_NETWORK;

	/**
	 * @brief Hardware temperature of DTN End System.
	 */
	uint4_gt A664_ES_HW_TEMP;

	/**
	 * @brief Internal Voltage value of DTN End System.
	 */
	uint4_gt A664_ES_HW_VCC_INT;

	/**
	 * @brief Assigned port speed of DTN ES. 2 --> 1000Mbps, 1 --> 100Mbps and 0 --> 10Mbps.
	 */
	uint8_gt A664_ES_PORT_SPEED;

	/**
	 * @brief Status of DTN ES Port A. If connection is provided then 1 = ONLINE , 0 = NO_LINK.
	 */
	uint8_gt A664_ES_PORT_A_STATUS;

	/**
	 * @brief Status of DTN ES Port B. If connection is provided then 1 = ONLINE , 0 = NO_LINK.
	 */
	uint8_gt A664_ES_PORT_B_STATUS;

	/**
	 * @brief Incoming packet count from DTN Driver on DTN End System TX buffer.
	 */
	uint8_gt A664_ES_TX_INCOMING_COUNT;

	/**
	 * @brief Sent packet count on DTN End System Port A.
	 */
	uint8_gt A664_ES_TX_A_OUTGOING_COUNT;

	/**
	 * @brief Sent packet count on DTN End System Port B.
	 */
	uint8_gt A664_ES_TX_B_OUTGOING_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System due to VL ID error.
	 */
	uint8_gt A664_ES_TX_VLID_DROP_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System due to minimum/maximum length error in TX direction.
	 */
	uint8_gt A664_ES_TX_LMIN_LMAX_DROP_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System TX buffer due to packet delay exceeding maximum jitter.
	 */
	uint8_gt A664_ES_TX_MAX_JITTER_DROP_COUNT;

	/**
	 * @brief Received packet count on DTN End System Port A.
	 */
	uint8_gt A664_ES_RX_A_INCOMING_COUNT;

	/**
	 * @brief Received packet count on DTN End System Port B.
	 */
	uint8_gt A664_ES_RX_B_INCOMING_COUNT;

	/**
	 * @brief Sent packet count to DTN Driver from DTN End System.
	 */
	uint8_gt A664_ES_RX_OUTGOING_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port A due to VL ID error in RX direction.
	 */
	uint8_gt A664_ES_RX_A_VLID_DROP_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port A due to minimum/maximum length error in RX direction.
	 */
	uint8_gt A664_ES_RX_A_LMIN_LMAX_DROP_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port A due to network type error in RX direction.
	 */
	uint8_gt A664_ES_RX_A_NET_ERR_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port A due to sequence number error in RX direction.
	 */
	uint8_gt A664_ES_RX_A_SEQ_ERR_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port A due to CRC error in RX direction.
	 */
	uint8_gt A664_ES_RX_A_CRC_ERROR_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port A due to IP checksum error in RX direction.
	 */
	uint8_gt A664_ES_RX_A_IP_CHECKSUM_ERROR_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port B due to VL ID error in RX direction.
	 */
	uint8_gt A664_ES_RX_B_VLID_DROP_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port B due to minimum/maximum length error in RX direction.
	 */
	uint8_gt A664_ES_RX_B_LMIN_LMAX_DROP_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port B due to sequence number error in RX direction.
	 */
	uint8_gt A664_ES_RX_B_SEQ_ERR_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port B due to network type error in RX direction.
	 */
	uint8_gt A664_ES_RX_B_NET_ERR_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port B due to CRC error in RX direction.
	 */
	uint8_gt A664_ES_RX_B_CRC_ERROR_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port B due to IP checksum error in RX direction.
	 */
	uint8_gt A664_ES_RX_B_IP_CHECKSUM_ERROR_COUNT;

	/**
	 * @brief Sent packet count from DTN Driver to DTN End System.
	 */
	uint8_gt A664_BSP_TX_PACKET_COUNT;

	/**
	 * @brief Sent byte count from DTN Driver to DTN End System.
	 */
	uint8_gt A664_BSP_TX_BYTE_COUNT;

	/**
	 * @brief Number of errors when sending packets from DTN Driver.
	 */
	uint8_gt A664_BSP_TX_ERROR_COUNT;

	/**
	 * @brief Number of packets read by DTN Driver from DTN End System.
	 */
	uint8_gt A664_BSP_RX_PACKET_COUNT;

	/**
	 * @brief Number of bytes received by DTN Driver from DTN End System.
	 */
	uint8_gt A664_BSP_RX_BYTE_COUNT;

	/**
	 * @brief Number of errors when reading packets from DTN End System by DTN Driver.
	 */
	uint8_gt A664_BSP_RX_ERROR_COUNT;

	/**
	 * @brief Number of packets dropped due to errors when reading packets from DTN End System by DTN Driver.
	 */
	uint8_gt A664_BSP_RX_MISSED_FRAME_COUNT;

	/**
	 * @brief DTN Driver version number.
	 */
	uint8_gt A664_BSP_VER;

	/**
	 * @brief This field indicates the hardware vendor of the DTN ES Device.
	 */
	uint8_gt A664_ES_VENDOR_TYPE;

	/**
	 * @brief Number of packets dropped from the DTN Driver VL port buffers due to errors when reading packets from DTN End System by DTN Driver.
	 */
	uint8_gt A664_ES_BSP_QUEUING_RX_VL_PORT_DROP_COUNT;

	/**
	 * @brief This parameter indicates the End System monitoring source. TRUE for Switch-End System, FALSE for End System.
	 */
	bool_gt A664_SW_ES_ENABLE;
} tA664ESMonitoring;


/**
 * @brief This structure contains configuration parameters of ARINC653 ports assigned to DTN VLs.
 */
typedef struct tA653ComPort
{
	/**
	 * @brief Name of the ARINC653 port defined in the GzIS_653.xml.
	 *
	 * @note User must enter the exact same name defined for an ARINC653 port in the GzIS_653.xml.
	 */
	NAME_TYPE a653PortName;

	/**
	 * @brief Partition ID of the partition in which the ARINC653 port defined and created in.
	 *
	 * @note User must enter the exact same Partition Identifier value defined for the partition in
	 * which the ARINC653 port is defined.
	 */
	int4_gt partitionID;

	/**
	 * @brief UDP Source Port ID assigned to the ARINC653 port by user.
	 *
	 * @details Values in the range of [0, 1023] are reserved, cannot be assigned: except the value 100.
	 * 100 can be used for DCU and VL 0 communications.
     *
     * @note Ensure the value is within the valid range to avoid undefined behavior:
     * - The exact value: 100
     * - A range of values: 1024 to 65535 (inclusive)
	 */
	uint2_gt srcUdpPortID;

	/**
	 * @brief UDP Destination Port ID assigned to the ARINC653 port by user.
	 *
	 * @details Values in the range of [0, 1023] are reserved, cannot be assigned: except the value 100.
	 * 100 can be used for DCU and communications.
     *
     * @note Ensure the value is within the valid range to avoid undefined behavior:
     * - The exact value: 100
     * - A range of values: 1024 to 65535 (inclusive)
	 */
	uint2_gt dstUdpPortID;
}tA653ComPort;

/**
 * @brief This structure contains configuration parameters of DTN TX VLs.
 */
typedef struct a664ESTXVirtualLink
{
  /**
   * @brief 16 bits VL ID value of TX VL.
   */
  uint2_gt virtualLinkID;

  /**
   * @brief Configuration parameter defining the BAG (Bandwidth Allocation Gap) value between packets.
   *
   * @details This parameter represents the BAG value in logarithmic base 2. It can take integer values ranging from 0 to 7,
   * where each step corresponds to an exponentially increasing interval. (e.g., 2^0=1, 2^1=2, ..., 2^7=128).
   *
   * @note Ensure the value is within the valid range [0, 7] to avoid undefined behavior.
   */
  eA664BAG BAG;

  /**
   * @brief Configuration parameter defining the maximum bytes of ARINC664 frame can be sent over a TX VL.
   *
   * @note Ensure the value is within the valid range [64, 1518] to avoid errors.
   * @note Ensure that Lmin value is not greater than Lmax value to avoid errors.
   *
   */
  uint2_gt Lmax;

  /**
   * @brief Configuration parameter defining the minimum bytes of ARINC664 frame can be sent over a TX VL.
   *
   * @note Ensure the value is within the valid range [64, 1518] to avoid errors.
   * @note Ensure that Lmax value is not less than Lmin value to avoid errors.
   */
  uint2_gt Lmin;

  /**
   * @brief Configuration parameter for setting the TX VL enabled or disabled. TRUE: enabled, FALSE: disabled.
   */
  bool_gt enable;

  /**
   * @brief Configuration parameter for setting the priority of TX VL for DTN End System scheduler to prioritize the TX VLs.
   *
   * @note Ensure the value is within the valid range [0, 255] to avoid undefined behavior.
   */
  uint1_gt priority;

  /**
   * @brief Configuration parameter for selecting the DTN End System physical port from which the packets of the TX VL will be sent.
   */
  eA664NetworkInterface interface;

  /**
   * @brief Configuration parameter for defining the DTN End System channel ID assigned to the TX VL.
   *
   * @note Ensure the value is within the valid range [0, 127] to avoid errors.
   */
  uint2_gt endSystemTXChannelID;

  /**
   * @brief Configuration parameter for setting the Feedback VL feature for the TX VL enabled or disabled.
   * TRUE: enabled, FALSE: disabled.
   */
  bool_gt feedbackVLEnabled;

  /**
   * @brief Configuration parameter for setting the Periodic Monitoring feature for the TX VL.
   * TRUE: enabled, FALSE: disabled.
   */
  bool_gt periodic_mon_enable;

  /**
   * @brief Configuration parameter defining the Monitoring Period value between packets.
   *
   * @details This parameter represents the Monitoring Period value in logarithmic base 2. It can take integer values ranging from 0 to 7,
   * where each step corresponds to an exponentially increasing interval. (e.g., 2^0=1, 2^1=2, ..., 2^7=128).
   *
   * @note Ensure the value is within the valid range [0, 7] to avoid undefined behavior.
   */
  e_dtn_mon_period monitoring_period;

  /**
   * @brief Configuration parameter for defining communication type of the ARINC653 port assigned to TX VL.
   */
  eA653ComPortType a653ComPortType;

  /**
   * @brief Configuration parameter for defining number of ARINC653 ports assigned to TX VL.
   *
   * @details Minimum 1, maximum 4 ports can be assigned to TX VL if the ARINC653 comm. port type is Queuing.
   * If the ARINC653 comm. port type is Sampling, only 1 port can be assigned to TX VL.
   *
   * @note Ensure the value is within the valid range to avoid errors:
   * - [1, 4] for Queuing.
   * - The exact value: 1 for Sampling.
   */
  uint2_gt portCount;

  /**
   * @brief Configuration structure for defining the properties of the ARINC653 ports assigned to TX VL.
   */
  tA653ComPort ports[A664_MAX_VL_PORT_CNT];
}a664ESTXVirtualLink;

/**
 * @brief This structure contains configuration parameters of DTN RX VLs.
 */
typedef struct a664ESRXVirtualLink
{
	/**
	 * @brief 16 bits VL ID value of RX VL.
	 */
	uint2_gt virtualLinkID;

	/**
	 * @brief Configuration parameter for setting the maximum skew value for RX VL, in micro-seconds.
	 *
	 * @details This parameter is
	 * dynamically set to 16 times the given input value (e.g., when user sets the value of skewMax parameter, the maximum skew
	 * is set to 16 micro-seconds).
	 *
	 * @note This parameter should be set in the range between 0 and the value which makes the maximum skew to be set to
	 * 5*BAG value.
	 */
	uint2_gt skewMax;

	/**
	 * @brief Configuration parameter for enabling/disabling the integrity checker of the RX VL. TRUE: enabled, FALSE: disabled.
	 */
	bool_gt integrityCheckerEnable;


	/**
	 * @brief Configuration parameter for enabling/disabling the redundancy checker of the RX VL. TRUE: enabled, FALSE: disabled.
	 */
	bool_gt redundancyCheckerEnable;

	/**
	 * @brief Configuration parameter for setting the RX VL enabled or disabled. TRUE: enabled, FALSE: disabled.
	 */
	bool_gt enable;

	/**
	 * @brief Configuration parameter for defining the DTN End System channel ID assigned to the RX VL.
	 *
	 * @details Channels 0,1,2 are used for configuration packets.
	 *
	 * @note Ensure the value is within the valid range [3, 511] to avoid errors.
	 */
	uint2_gt endSystemRXChannelID;

	/**
	 * @brief Configuration parameter for defining communication type of the ARINC653 port assigned to RX VL.
	 */
	eA653ComPortType a653ComPortType;

	/**
	 * @brief Configuration parameter for defining number of ARINC653 ports assigned to RX VL.
	 *
	 * @details Minimum 1, maximum 4 ports can be
	 * assigned to TX VL if the ARINC653 comm. port type is Queuing. If the ARINC653 comm. port type is Sampling, only 1 port can
	 * be assigned to RX VL.
	 *
	 * @note Ensure the value is within the valid range to avoid errors:
	 * - [1, 4] for Queuing.
	 * - The exact value: 1 for Sampling.
	 */
	uint2_gt portCount;

	/**
	 * @brief Configuration structure for defining the properties of the ARINC653 ports assigned to RX VL.
	 */
	tA653ComPort ports[A664_MAX_VL_PORT_CNT];
}a664ESRXVirtualLink;

/**
 * @brief Configuration structure for setting the properties of Network A Port and Network B Port.
 */
typedef struct a664ESPort{
	/**
	 * @brief Configuration parameter for setting the communication type of network ports of DTN End System.
	 * (ARINC664, Best-Efort, both ARINC664 and Best-Efort)
	 */
	eA664ESPortType Type :2;
}a664ESPort;

/**
 * @brief Configuration structure for setting the device configuration parameters of DTN End System.
 */
typedef struct a664ESDevCFG{
	/**
	 * @brief Configuration parameter for setting the device type of the configured DTN End System.
	 * (Stand-Alone End System or End System of DTN Switch)
	 */
	eA664ESDevType DeviceType :2;

	/**
	 * @brief Configuration for selecting the network (Network A/Network B) for the DTN End System device.
	 */
	eA664ESNetworkType NetworkType :4;

	/**
	 * @brief Configuration ID for the DTN End System.
	 */
	uint2_gt ConfigID :16;
}a664ESDevCFG;

/**
 * @brief This is the main configuration structure for configuring DTN End System Device. This structure
 * contains configuration parameters and sub structures for TX VL, RX VL, device and port configurations.
 */
typedef struct tA664EndSystem
{
  /**
   * @brief Configuration parameter for selecting the configuration type (initial configuration/re-configuration)
   * to be applied on DTN End System.
   */
  eA664ConfigType configType;

  /**
   * @brief Configuration parameter for setting the number of DTN End System TX VLs.
   */
  uint2_gt txVLCount;

  /**
   * @brief Configuration structure that contains DTN End System TX VL configuration parameters.
   */
  a664ESTXVirtualLink txVLs[A664_MAX_ALLOWED_TX_SIDE_VL_COUNT];

  /**
   * @brief Configuration parameter for setting the number of DTN End System RX VLs.
   */
  uint2_gt rxVLCount;

  /**
   * @brief Configuration structure that contains DTN End System RX VL configuration parameters.
   */
  a664ESRXVirtualLink rxVLs[A664_MAX_ALLOWED_RX_SIDE_VL_COUNT];

  /**
   * @brief Configuration parameter for setting the data-rate of DTN End System physical ports.
   */
  eA664ESPortSpeed portSpeed;

  /**
   * @brief Configuration structure that contains DTN End System physical port configuration parameters.
   */
  a664ESPort ports[A664_ES_PORT_COUNT];

  /**
   * @brief Configuration structure that contains DTN End System device configuration parameters.
   */
  a664ESDevCFG device;
} tA664EndSystem;

#endif /* ES_CONFIGURATION_A664ESCONFIGDEFS_H_ */
#else

// Replaced GzIS-specific types with standard integer types to ensure compatibility.
// Mapping:
// uint1_gt  -> uint8_t
// uint2_gt  -> uint16_t
// uint4_gt  -> uint32_t
// uint8_gt  -> uint64_t
// bool_gt   -> uint32_t

/**
 * @brief This structure contains the DTN End System monitoring parameters.
 */
typedef struct tA664ESMonitoring
{
	/**
	 * @brief Hardware firmware version.
	 */
	uint64_t A664_ES_FW_VER;

	/**
	 * @brief Device ID of DTN End System.
	 */
	uint64_t A664_ES_DEV_ID;

	/**
	 * @brief Current operation mode of DTN End System.
	 */
	uint64_t A664_ES_MODE;

	/**
	 * @brief Configuration ID of DTN End System.
	 */
	uint64_t A664_ES_CONFIG_ID;

	/**
	 * @brief Result of the built-in test of DTN Hardware.
	 */
	uint64_t A664_ES_BIT_STATUS;

	/**
 	 * @brief Status of DTN End System Configuration. If it's value is 3, that means configuration is successfully applied.
	 */
	uint64_t A664_ES_CONFIG_STATUS;

	/**
	 * @brief DTN Driver Configuration Status
	 *
	 * This field uses bit-mapped encoding to indicate configuration completion status:
	 * - Bit 0 (LSB): End System (ES) configuration status
	 * - Bit 1: Switch (SW) configuration status
	 * - Bit 2: Switch-End System (SW-ES) configuration status
	 *
	 * Each bit value of '1' indicates that the corresponding configuration is complete.
	 */
	uint8_t A664_BSP_CONFIG_STATUS;

	/**
	 * @brief ID of the PTP configuration.
	 */
	uint16_t A664_PTP_CONFIG_ID;

	/**
	 * @brief PTP device type. 0: Slave, 1: Master, 3: Grand Master.
	 */
	uint8_t A664_PTP_DEVICE_TYPE;

	/**
	 * @brief Status of PTP Rate-Correction. 0: disabled, 1: enabled.
	 */
	uint8_t A664_PTP_RC_STATUS;

	/**
	 * @brief PTP synchronization status of DTN End System’s Port A. This parameter is valid for PTP Slave.
	 */
	uint8_t A664_PTP_PORT_A_SYNC;

	/**
	 * @brief PTP synchronization status of DTN End System’s Port A. This parameter is valid for PTP Slave.
	 */
	uint8_t A664_PTP_PORT_B_SYNC;

	/**
	 * @brief Synchronization VL ID of PTP Master device. This parameter is valid for PTP Master/PTP Grand-master devices.
	 */
	uint16_t A664_PTP_SYNC_VL_ID;

	/**
	 * @brief Request VL ID of PTP Slave device. This parameter is valid for PTP Slave.
	 */
	uint16_t A664_PTP_REQ_VL_ID;

	/**
	 * @brief Response VL ID of PTP Slave device. This parameter is valid for PTP Slave.
	 */
	uint16_t A664_PTP_RES_VL_ID;

	/**
	 * @brief This parameter indicates from which network the PTP TOD information is received.
	 */
	uint8_t A664_PTP_TOD_NETWORK;

	/**
	 * @brief Hardware temperature of DTN End System.
	 */
	uint32_t A664_ES_HW_TEMP;

	/**
	 * @brief Internal Voltage value of DTN End System.
	 */
	uint32_t A664_ES_HW_VCC_INT;

	/**
	 * @brief Assigned port speed of DTN ES. 2 --> 1000Mbps, 1 --> 100Mbps and 0 --> 10Mbps.
	 */
	uint64_t A664_ES_PORT_SPEED;

	/**
	 * @brief Status of DTN ES Port A. If connection is provided then 1 = ONLINE , 0 = NO_LINK.
	 */
	uint64_t A664_ES_PORT_A_STATUS;

	/**
	 * @brief Status of DTN ES Port B. If connection is provided then 1 = ONLINE , 0 = NO_LINK.
	 */
	uint64_t A664_ES_PORT_B_STATUS;

	/**
	 * @brief Incoming packet count from DTN Driver on DTN End System TX buffer.
	 */
	uint64_t A664_ES_TX_INCOMING_COUNT;

	/**
	 * @brief Sent packet count on DTN End System Port A.
	 */
	uint64_t A664_ES_TX_A_OUTGOING_COUNT;

	/**
	 * @brief Sent packet count on DTN End System Port B.
	 */
	uint64_t A664_ES_TX_B_OUTGOING_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System due to VL ID error.
	 */
	uint64_t A664_ES_TX_VLID_DROP_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System due to minimum/maximum length error in TX direction.
	 */
	uint64_t A664_ES_TX_LMIN_LMAX_DROP_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System TX buffer due to packet delay exceeding maximum jitter.
	 */
	uint64_t A664_ES_TX_MAX_JITTER_DROP_COUNT;

	/**
	 * @brief Received packet count on DTN End System Port A.
	 */
	uint64_t A664_ES_RX_A_INCOMING_COUNT;

	/**
	 * @brief Received packet count on DTN End System Port B.
	 */
	uint64_t A664_ES_RX_B_INCOMING_COUNT;

	/**
	 * @brief Sent packet count to DTN Driver from DTN End System.
	 */
	uint64_t A664_ES_RX_OUTGOING_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port A due to VL ID error in RX direction.
	 */
	uint64_t A664_ES_RX_A_VLID_DROP_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port A due to minimum/maximum length error in RX direction.
	 */
	uint64_t A664_ES_RX_A_LMIN_LMAX_DROP_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port A due to network type error in RX direction.
	 */
	uint64_t A664_ES_RX_A_NET_ERR_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port A due to sequence number error in RX direction.
	 */
	uint64_t A664_ES_RX_A_SEQ_ERR_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port A due to CRC error in RX direction.
	 */
	uint64_t A664_ES_RX_A_CRC_ERROR_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port A due to IP checksum error in RX direction.
	 */
	uint64_t A664_ES_RX_A_IP_CHECKSUM_ERROR_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port B due to VL ID error in RX direction.
	 */
	uint64_t A664_ES_RX_B_VLID_DROP_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port B due to minimum/maximum length error in RX direction.
	 */
	uint64_t A664_ES_RX_B_LMIN_LMAX_DROP_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port B due to sequence number error in RX direction.
	 */
	uint64_t A664_ES_RX_B_SEQ_ERR_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port B due to network type error in RX direction.
	 */
	uint64_t A664_ES_RX_B_NET_ERR_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port B due to CRC error in RX direction.
	 */
	uint64_t A664_ES_RX_B_CRC_ERROR_COUNT;

	/**
	 * @brief Dropped packet count on DTN End System Port B due to IP checksum error in RX direction.
	 */
	uint64_t A664_ES_RX_B_IP_CHECKSUM_ERROR_COUNT;

	/**
	 * @brief Sent packet count from DTN Driver to DTN End System.
	 */
	uint64_t A664_BSP_TX_PACKET_COUNT;

	/**
	 * @brief Sent byte count from DTN Driver to DTN End System.
	 */
	uint64_t A664_BSP_TX_BYTE_COUNT;

	/**
	 * @brief Number of errors when sending packets from DTN Driver.
	 */
	uint64_t A664_BSP_TX_ERROR_COUNT;

	/**
	 * @brief Number of packets read by DTN Driver from DTN End System.
	 */
	uint64_t A664_BSP_RX_PACKET_COUNT;

	/**
	 * @brief Number of bytes received by DTN Driver from DTN End System.
	 */
	uint64_t A664_BSP_RX_BYTE_COUNT;

	/**
	 * @brief Number of errors when reading packets from DTN End System by DTN Driver.
	 */
	uint64_t A664_BSP_RX_ERROR_COUNT;

	/**
	 * @brief Number of packets dropped due to errors when reading packets from DTN End System by DTN Driver.
	 */
	uint64_t A664_BSP_RX_MISSED_FRAME_COUNT;

	/**
	 * @brief DTN Driver version number.
	 */
	uint64_t A664_BSP_VER;

	/**
	 * @brief This field indicates the hardware vendor of the DTN ES Device.
	 */
	uint64_t A664_ES_VENDOR_TYPE;

	/**
	 * @brief Number of packets dropped from the DTN Driver VL port buffers due to errors when reading packets from DTN End System by DTN Driver.
	 */
	uint64_t A664_ES_BSP_QUEUING_RX_VL_PORT_DROP_COUNT;

	/**
	 * @brief This parameter indicates the End System monitoring source. TRUE for Switch-End System, FALSE for End System.
	 */
	uint32_t A664_SW_ES_ENABLE;

} tA664ESMonitoring;


static inline void dsm_dtnes_endian_convert(tA664ESMonitoring *data) {
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

static inline void dsm_dtnes_header_print(FILE *fp) {
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

static inline void dsm_dtnes_data_print(FILE *fp, tA664ESMonitoring *data) {
	dsm_dtnes_endian_convert(data);
	// BEGIN tA664ESMonitoring
    fprintf(fp, "%llu,", data->A664_ES_FW_VER);
    fprintf(fp, "%llu,", data->A664_ES_DEV_ID);
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
    fprintf(fp, "%llu,", data->A664_BSP_VER);
    fprintf(fp, "%llu,", data->A664_ES_VENDOR_TYPE);
    fprintf(fp, "%llu,", data->A664_ES_BSP_QUEUING_RX_VL_PORT_DROP_COUNT);
    fprintf(fp, "%u\n", data->A664_SW_ES_ENABLE);
    // END tA664ESMonitoring
}

#endif /* ES_CONFIGURATION_A664ESCONFIGDEFS_H_ */

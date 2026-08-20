#ifndef CMC_SW_MONITORING_H
#define CMC_SW_MONITORING_H

#include <stdint.h>
#include <stdbool.h>

#ifndef CMC_BOOL_GT_DEFINED
#define CMC_BOOL_GT_DEFINED
typedef enum
{
#ifndef FALSE
    FALSE,
#else
    __FALSE__,
#endif
#ifndef TRUE
    TRUE
#else
    __TRUE__
#endif
} bool_gt;
#endif /* CMC_BOOL_GT_DEFINED */

/**uint64_t
 * @brief Maximum number of ports that can be configured on DTN Switch.
 */
#define A664_SW_MAX_PORT_COUNT 	        12U

/**
 * @brief Maximum number of MACs that can be configured on DTN Switch.
 */
#define A664_SW_MAX_MAC_BASED_COUNT 	12U

/**
 * @brief Maximum number of VLs that can be configured on DTN Switch.
 */
#define A664_SW_MAX_VL_COUNT 	        16384U

// ============================================================================
// ============================================================================

// SW MONITORING STATUS
/* ===========================================================================
 * BITFIELD BIT SIRASI — KRİTİK
 * ---------------------------------------------------------------------------
 * DTN SW monitoring mesajını üreten firmware big-endian bir hedefte derlenir;
 * bitfield'lar storage unit içinde MSB'den başlayarak yerleşir. x86_64 (LE)
 * derleyici LSB'den başlatır, dolayısıyla aynı ICD sırası aynı struct'ı VERMEZ.
 *
 * Örnek (port struct, PORT_ID sonrası 8 baytlık bitfield word'ü):
 *   BE üretici  → PORT_LINK = son byte'ın bit0'ı
 *   LE tüketici → PORT_LINK = son byte'ın bit7'si  (padding biti okunuyordu:
 *                 tüm portlar sürekli aynı değeri gösteriyordu)
 *
 * Çözüm: bit sırası bayrağına göre alanlar iki kez tanımlanır. LE dalında sıra
 * HER BAYT İÇİNDE ters çevrilir (bayt sırası korunur); bunun için çok baytlı
 * padding'ler bayt sınırından ikiye bölünür (ör. 58 = 56 + 2). Toplam bit
 * sayısı her iki dalda da aynıdır, struct boyutları değişmez.
 * =========================================================================== */
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define A664_SW_BITFIELD_MSB_FIRST 1
#else
#define A664_SW_BITFIELD_MSB_FIRST 0
#endif

typedef struct a664SWMonitoringStatus {
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

#if A664_SW_BITFIELD_MSB_FIRST
	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint32_t A664_SW_PADDING :23;

	/**
	 * @brief Heart-beat signal of DTN Switch.
	 */
	uint8_t A664_SW_HEARTBEAT :1;
#else
	/** @brief Padding (ilk 2 tam bayt — bayt sırası korunur). */
	uint32_t A664_SW_PADDING :16;

	/**
	 * @brief Heart-beat signal of DTN Switch. (wire: son baytın bit0'ı)
	 */
	uint8_t A664_SW_HEARTBEAT :1;

	/** @brief Padding (son baytın kalan üst bitleri). */
	uint8_t A664_SW_PADDING_HI :7;
#endif

	/**
	 * @brief Device ID of DTN Switch.
	 */
	uint16_t A664_SW_DEVICE_ID;

	/**
	 * @brief Number of physical ports DTN Switch has.
	 */
	uint8_t A664_SW_PORT_NUM;

#if A664_SW_BITFIELD_MSB_FIRST
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
	uint8_t A664_SW_PADDING2 :2;

	/**
	 * @brief Current operation mode of DTN Switch.
	 */
	uint8_t A664_SW_CURRENT_MODE :6;

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
	uint8_t A664_SW_AUTOMAC_UPDATE_STATUS :1;

	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint8_t A664_SW_PADDING5 :7;

	/**
	 * @brief Status of the upstream mode activation.
	 */
	uint8_t A664_SW_UPSTREAM_MODE_STATUS :1;
#else
	/* 80 bit / 10 bayt kesintisiz akış — her bayt kendi içinde ters. */

	/* bayt 0 */
	/**
	 * @brief Status of the token bucket activation. (wire: bayt0 bit0)
	 */
	uint8_t A664_SW_TOKEN_BUCKET_STATUS :1;
	/** @brief Padding added to ensure proper alignment within the structure. */
	uint8_t A664_SW_PADDING1 :7;

	/* bayt 1 */
	/**
	 * @brief Current operation mode of DTN Switch. (wire: bayt1 bit5..0)
	 */
	uint8_t A664_SW_CURRENT_MODE :6;
	/** @brief Padding added to ensure proper alignment within the structure. */
	uint8_t A664_SW_PADDING2 :2;

	/* bayt 2..6 — tam bayt padding, bayt sırası korunur */
	/** @brief Padding added to ensure proper alignment within the structure. */
	uint64_t A664_SW_PADDING3 :40;

	/* bayt 7 */
	/**
	 * @brief This field indicates the hardware vendor of the DTN ES Device.
	 *        (wire: bayt7 bit0)
	 */
	uint64_t A664_SW_VENDOR_TYPE :1;
	/** @brief Padding (PADDING3'ün bayt sınırına düşen üst 7 biti). */
	uint64_t A664_SW_PADDING3_HI :7;

	/* bayt 8 */
	/**
	 * @brief Status of the automatic MAC list update activation.
	 *        (wire: bayt8 bit0)
	 */
	uint8_t A664_SW_AUTOMAC_UPDATE_STATUS :1;
	/** @brief Padding added to ensure proper alignment within the structure. */
	uint8_t A664_SW_PADDING4 :7;

	/* bayt 9 */
	/**
	 * @brief Status of the upstream mode activation. (wire: bayt9 bit0)
	 */
	uint8_t A664_SW_UPSTREAM_MODE_STATUS :1;
	/** @brief Padding added to ensure proper alignment within the structure. */
	uint8_t A664_SW_PADDING5 :7;
#endif

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
}__attribute__((packed))a664SWMonitoringStatus;

// ============================================================================
// ============================================================================

// SW MONITORING PORT STATUS
typedef struct __attribute__((packed)) a664SWMonitoringPort {
	/**
	 * @brief ID of the DTN Switch Port.
	 */
	uint64_t A664_SW_PORT_ID;

#if A664_SW_BITFIELD_MSB_FIRST
	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint64_t A664_SW_PADDING : 58;

	/**
	 * @brief Built-in test status of DTN Switch.
	 */
	uint8_t A664_SW_BIT_STATUS : 2;

	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint8_t A664_SW_PADDING2 : 3;

	/**
	 * @brief Liveliness indication DTN Switch Port.
	 *        0: LINK_UP, 1: LINK_DOWN.
	 */
	uint8_t A664_SW_PORT_LINK : 1;
#else
	/* 64 bit / 8 bayt — anlamlı alanlar son bayttadır, o bayt ters çevrilir. */

	/** @brief Padding (ilk 7 tam bayt — bayt sırası korunur). */
	uint64_t A664_SW_PADDING : 56;

	/**
	 * @brief Liveliness indication DTN Switch Port.
	 *        0: LINK_UP, 1: LINK_DOWN. (wire: son baytın bit0'ı)
	 */
	uint8_t A664_SW_PORT_LINK : 1;

	/**
	 * @brief Padding added to ensure proper alignment within the structure.
	 */
	uint8_t A664_SW_PADDING2 : 3;

	/**
	 * @brief Built-in test status of DTN Switch. (wire: son baytın bit5..4'ü)
	 */
	uint8_t A664_SW_BIT_STATUS : 2;

	/** @brief Padding (PADDING'in bayt sınırına düşen üst 2 biti). */
	uint8_t A664_SW_PADDING_HI : 2;
#endif

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
typedef struct __attribute__((packed)) tA664SWMonitoring{
	/**
	 * @brief This field contains information related to DTN Switch such as health-monitoring, statistics, operation, configuration status etc..
	 */
	a664SWMonitoringStatus status;

	/**
	 * @brief This field contains information and statistics about the DTN Switch Port.
	 */
	a664SWMonitoringPort port[A664_SW_MAX_PORT_COUNT];
} tA664SWMonitoring;

/* Wire boyutlari ICD ile sabit — bitfield sirasi degisse de degismemeli. */
_Static_assert(sizeof(a664SWMonitoringStatus) == 136,  "a664SWMonitoringStatus size mismatch");
_Static_assert(sizeof(a664SWMonitoringPort)   == 184,  "a664SWMonitoringPort size mismatch");
_Static_assert(sizeof(tA664SWMonitoring)      == 2344, "tA664SWMonitoring size mismatch");

#endif /* CMC_SW_MONITORING_H */
#ifndef CL_CMSW_MESSAGE_TYPES_H
#define CL_CMSW_MESSAGE_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* ===========================================================================
 * BITFIELD BIT SIRASI — KRİTİK
 * ---------------------------------------------------------------------------
 * Bu mesajı üreten CMC firmware big-endian bir hedefte (T2080/PowerPC) derlenir.
 * Big-endian ABI'de bitfield'lar storage unit içinde MSB'den başlayarak
 * yerleştirilir; ATE tarafındaki x86_64 (little-endian) derleyici ise LSB'den
 * başlar. Aynı ICD sırası iki tarafta AYNI struct'ı vermez:
 *
 *   ICD:  slot_id:5 | ipmc_data_validity:1 | operation_mode:1 | module_status:1
 *   wire byte 0x19 = 0b00011001
 *     BE üretici  → slot_id = bit7..3 = 3,  module_status = bit0 = 1
 *     LE tüketici → slot_id = bit4..0 = 25, module_status = bit7 = 0   (YANLIŞ)
 *
 * Bu yüzden her bitfield grubu iki kez tanımlanır: BE derlemede ICD sırası,
 * LE derlemede grup içi sıra TERS. Buradaki tüm gruplar tam 8 bit (1 byte)
 * olduğu için ters sıralama, wire yerleşimini birebir karşılar.
 *
 * YENİ ALAN EKLERKEN: alanı HER İKİ bloğa da ekleyin (LE bloğunda ters yerine)
 * ve grubun toplamını 8 bitin katı tutun; aksi halde mesajın tamamı kayar.
 * =========================================================================== */
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define CL_CMSW_BITFIELD_MSB_FIRST 1
#else
#define CL_CMSW_BITFIELD_MSB_FIRST 0
#endif

#define DPM_COUNT_B0 5U
#define DSM_COUNT_B0 2U
#define HSM_COUNT_B0 2U
#define GPM_COUNT_B0 2U
#define PSM_COUNT_B0 2U
#define HUM_COUNT_B0 2U

/* ICD (cl_cmsw_message_interface.h) değer sabitleri. #ifndef ile korunur:
 * bu header başka ICD header'larıyla birlikte include edilebilir. */
#ifndef MAXIMUM_MODULES_IN_A_RACK
#define MAXIMUM_MODULES_IN_A_RACK 17U
#endif
#ifndef CMSW_INVALID
#define CMSW_INVALID 1U
#endif
#ifndef CMSW_VALID
#define CMSW_VALID 0
#endif
#ifndef TEMP_OK
#define TEMP_OK 0U
#endif
#ifndef TEMP_HOT
#define TEMP_HOT 1U
#endif
#ifndef MODULE_ALIVE
#define MODULE_ALIVE 0U
#endif
#ifndef MODULE_LOSS
#define MODULE_LOSS 1U
#endif
#ifndef SYNCHED
#define SYNCHED 0
#endif
#ifndef NOT_SYNCHED
#define NOT_SYNCHED 1U
#endif
#ifndef REDUNDANCY_OK
#define REDUNDANCY_OK 0
#endif
#ifndef REDUNDANCY_LOSS
#define REDUNDANCY_LOSS 1U
#endif
#ifndef DATA_AVAILABLE
#define DATA_AVAILABLE 0
#endif
#ifndef DATA_LOSS
#define DATA_LOSS 1U
#endif
#ifndef TEMP_GOOD
#define TEMP_GOOD 0
#endif
#ifndef TEMP_DATA_LOSS
#define TEMP_DATA_LOSS 1U
#endif
#ifndef TEMP_ORANGE
#define TEMP_ORANGE 2U
#endif
#ifndef TEMP_RED
#define TEMP_RED 3U
#endif
#ifndef CURRENT_GOOD
#define CURRENT_GOOD 0
#endif
#ifndef CURRENT_LOSS
#define CURRENT_LOSS 1U
#endif
#ifndef CURRENT_ORANGE
#define CURRENT_ORANGE 2U
#endif
#ifndef CURRENT_RED
#define CURRENT_RED 3U
#endif
#ifndef VOLTAGE_GOOD
#define VOLTAGE_GOOD 0
#endif
#ifndef VOLTAGE_LOSS
#define VOLTAGE_LOSS 1U
#endif
#ifndef VOLTAGE_ORANGE
#define VOLTAGE_ORANGE 2U
#endif
#ifndef VOLTAGE_RED
#define VOLTAGE_RED 3U
#endif
#ifndef GOOD_LINK
#define GOOD_LINK 0
#endif
#ifndef BAD_LINK
#define BAD_LINK 1U
#endif
#ifndef GOOD
#define GOOD 0
#endif
#ifndef BAD
#define BAD 1U
#endif
#ifndef ADVB_TX_AVAILABLE
#define ADVB_TX_AVAILABLE 0
#endif
#ifndef ADVB_TX_LOSS
#define ADVB_TX_LOSS 1U
#endif
#ifndef ADVB_TX_WARNING
#define ADVB_TX_WARNING 2U
#endif
#ifndef DVI_AVAILABLE
#define DVI_AVAILABLE 0
#endif
#ifndef DVI_LOSS
#define DVI_LOSS 1U
#endif
#ifndef DVI_WARNING
#define DVI_WARNING 2U
#endif
#ifndef ALIGNED
#define ALIGNED 0
#endif
#ifndef NOT_ALIGNED
#define NOT_ALIGNED 1U
#endif
#ifndef OFP_MODE
#define OFP_MODE 0
#endif
#ifndef MAINTENANCE_MODE
#define MAINTENANCE_MODE 1U
#endif
#ifndef LINK_OK
#define LINK_OK 0
#endif
#ifndef LINK_LOSS
#define LINK_LOSS 1U
#endif

typedef struct __attribute__((packed))
{
	uint8_t lru_id;
#if CL_CMSW_BITFIELD_MSB_FIRST
	uint8_t slot_id:5;
	uint8_t reserved:1;
	uint8_t xmc_fpga:2;
#else
	uint8_t xmc_fpga:2;
	uint8_t reserved:1;
	uint8_t slot_id:5;
#endif
}Cl_cmsw_device_id_type;

typedef struct __attribute__((packed))
{
    uint8_t patch;
    uint8_t minor;
    uint8_t major;
} pcs_firmware_version_type;

typedef struct __attribute__((packed)) {

#if CL_CMSW_BITFIELD_MSB_FIRST
	uint8_t slot_id:5;
	uint8_t ipmc_data_validity:1; //0: CMSW_VALID 1: CMSW_INVALID
	uint8_t operation_mode:1;   //0 : OFP_MODE, 1: MAINTENANCE_MODE
	uint8_t module_status:1;	//0: MODULE_ALIVE, 1: MODULE LOSS
#else
	uint8_t module_status:1;	//0: MODULE_ALIVE, 1: MODULE LOSS
	uint8_t operation_mode:1;   //0 : OFP_MODE, 1: MAINTENANCE_MODE
	uint8_t ipmc_data_validity:1; //0: CMSW_VALID 1: CMSW_INVALID
	uint8_t slot_id:5;
#endif

	pcs_firmware_version_type  firmware_version;
	uint8_t hardware_type;
	uint8_t serial_id[8U];
	uint8_t bilgem_id[9U];
	uint8_t reserved[3U];
	uint8_t reset_counter;
	uint8_t power_status;

}Cl_cmsw_generic_lrm_status_type;


/** Indicates the result of T2080_DPM_BOOTLOADER_PBIT. */
typedef struct
{
#if CL_CMSW_BITFIELD_MSB_FIRST
    /** Indicates the result of DDR test. 0: GOOD 1: BAD */
    uint8_t ddr_test:1;
    /** Indicates the result of IFC NAND test. 0: GOOD 1: BAD */
    uint8_t ifc_nand_test:1;
    /** Indicates the result of SERDES1 PLL1 test. 0: GOOD 1: BAD */
    uint8_t serdes1_pll1_test:1;
    /** Indicates the result of SERDES2 PLL1 test. 0: GOOD 1: BAD */
    uint8_t serdes2_pll1_test:1;
    /** Indicates the result of SERDES2 PLL2 test. 0: GOOD 1: BAD */
    uint8_t serdes2_pll2_test:1;
    uint8_t reserved:3;
#else
    uint8_t reserved:3;
    /** Indicates the result of SERDES2 PLL2 test. 0: GOOD 1: BAD */
    uint8_t serdes2_pll2_test:1;
    /** Indicates the result of SERDES2 PLL1 test. 0: GOOD 1: BAD */
    uint8_t serdes2_pll1_test:1;
    /** Indicates the result of SERDES1 PLL1 test. 0: GOOD 1: BAD */
    uint8_t serdes1_pll1_test:1;
    /** Indicates the result of IFC NAND test. 0: GOOD 1: BAD */
    uint8_t ifc_nand_test:1;
    /** Indicates the result of DDR test. 0: GOOD 1: BAD */
    uint8_t ddr_test:1;
#endif
} __attribute__((packed)) t2080_dpm_bootloader_pbit_t;

/** Indicates the result of T2080_DSM_BOOTLOADER_PBIT. */
typedef struct
{
#if CL_CMSW_BITFIELD_MSB_FIRST
    /** Indicates the result of DDR test. 0: GOOD 1: BAD */
    uint8_t ddr_test:1;
    /** Indicates the result of IFC NAND test. 0: GOOD 1: BAD */
    uint8_t ifc_nand_test:1;
    /** Indicates the result of SERDES1 PLL1 test. 0: GOOD 1: BAD */
    uint8_t serdes1_pll1_test:1;
    /** Indicates the result of SERDES1 PLL2 test. 0: GOOD 1: BAD */
    uint8_t serdes1_pll2_test:1;
    uint8_t reserved:4;
#else
    uint8_t reserved:4;
    /** Indicates the result of SERDES1 PLL2 test. 0: GOOD 1: BAD */
    uint8_t serdes1_pll2_test:1;
    /** Indicates the result of SERDES1 PLL1 test. 0: GOOD 1: BAD */
    uint8_t serdes1_pll1_test:1;
    /** Indicates the result of IFC NAND test. 0: GOOD 1: BAD */
    uint8_t ifc_nand_test:1;
    /** Indicates the result of DDR test. 0: GOOD 1: BAD */
    uint8_t ddr_test:1;
#endif
} __attribute__((packed)) t2080_dsm_bootloader_pbit_t;

typedef struct __attribute__((packed)) {

	Cl_cmsw_generic_lrm_status_type lrm_status;

#if CL_CMSW_BITFIELD_MSB_FIRST
	uint8_t dvi_status:1; 			   		 //0: DVI_AVAILABLE, 1: DVI_LOSS	//FMECA OK
	uint8_t voltage_data_status:1;    		 //0: DATA_AVAILABLE, 1: DATA_LOSS	//FMECA OK
	uint8_t temperature_data_status:1;		 //0: DATA_AVAILABLE, 1: DATA_LOSS	//FMEKA OK
	uint8_t reserved:5;
#else
	uint8_t reserved:5;
	uint8_t temperature_data_status:1;		 //0: DATA_AVAILABLE, 1: DATA_LOSS	//FMEKA OK
	uint8_t voltage_data_status:1;    		 //0: DATA_AVAILABLE, 1: DATA_LOSS	//FMECA OK
	uint8_t dvi_status:1; 			   		 //0: DVI_AVAILABLE, 1: DVI_LOSS	//FMECA OK
#endif

}Cl_cmsw_iocm_status_type;


typedef struct __attribute__((packed)) {

	Cl_cmsw_generic_lrm_status_type lrm_status; 

#if CL_CMSW_BITFIELD_MSB_FIRST
	uint8_t advb_tx_1_status:2; 	   		 //0: ADVB_TX_AVAILABLE 1: ADVB_TX_LOSS 2: ADVB_TX_WARNING 	//FMECA OK
	uint8_t advb_tx_2_status:2;        		 //0: ADVB_TX_AVAILABLE 1: ADVB_TX_LOSS	2: ADVB_TX_WARNING 	//FMECA OK
	uint8_t advb_tx_3_status:2;        		 //0: ADVB_TX_AVAILABLE 1: ADVB_TX_LOSS 2: ADVB_TX_WARNING 	//FMECA OK
	uint8_t advb_hm_data_status:1;     		 //0: DATA_AVAILABLE, 1: DATA_LOSS							//FMECA OK
	uint8_t advb_status:1;					 //0: DATA_AVAILABLE, 1: DATA_LOSS							//FMECA OK

	uint8_t dvi_status:2; 			   		 //0: DVI_AVAILABLE, 1: DVI_LOSS 2: DVI_WARNING 			//FMECA OK
	uint8_t voltage_data_status:1;    		 //0: DATA_AVAILABLE, 1: DATA_LOSS							//FMECA OK
	uint8_t temperature_data_status:1;		 //0: DATA_AVAILABLE, 1: DATA_LOSS							//FMECA OK
	uint8_t reserved:4;
#else
	uint8_t advb_status:1;					 //0: DATA_AVAILABLE, 1: DATA_LOSS							//FMECA OK
	uint8_t advb_hm_data_status:1;     		 //0: DATA_AVAILABLE, 1: DATA_LOSS							//FMECA OK
	uint8_t advb_tx_3_status:2;        		 //0: ADVB_TX_AVAILABLE 1: ADVB_TX_LOSS 2: ADVB_TX_WARNING 	//FMECA OK
	uint8_t advb_tx_2_status:2;        		 //0: ADVB_TX_AVAILABLE 1: ADVB_TX_LOSS	2: ADVB_TX_WARNING 	//FMECA OK
	uint8_t advb_tx_1_status:2; 	   		 //0: ADVB_TX_AVAILABLE 1: ADVB_TX_LOSS 2: ADVB_TX_WARNING 	//FMECA OK

	uint8_t reserved:4;
	uint8_t temperature_data_status:1;		 //0: DATA_AVAILABLE, 1: DATA_LOSS							//FMECA OK
	uint8_t voltage_data_status:1;    		 //0: DATA_AVAILABLE, 1: DATA_LOSS							//FMECA OK
	uint8_t dvi_status:2; 			   		 //0: DVI_AVAILABLE, 1: DVI_LOSS 2: DVI_WARNING 			//FMECA OK
#endif

}Cl_cmsw_gpm_status_type;


typedef struct __attribute__((packed)) {

	Cl_cmsw_generic_lrm_status_type lrm_status;

	int32_t a653_schedule_id; 		/*Applicable when  ml_cmsw_data_validity:CMSW_VALID*/		//OK

#if CL_CMSW_BITFIELD_MSB_FIRST
	uint8_t voltage_data_status:1;     /*0: DATA_AVAILABLE, 1: DATA_LOSS*/	//FMECA OK
	uint8_t temperature_data_status:1; /*0: DATA_AVAILABLE, 1: DATA_LOSS*/	//FMECA OK
	uint8_t dtn_es_data_validity:1; 		/*0: CMSW_VALID, 1: CMSW_INVALID, Applicable when  ml_cmsw_data_validity:CMSW_VALID*/	//OK
	uint8_t ptp_sync_status:1; 		   		/*0: SYNCHED 1: NOT_SYNCHED, Applicable when dtn_es_validity:CMSW_VALID*/				//OK
	uint8_t hsn_link_status:1; 		   		/*0:GOOD 1:BAD, Applicable when ml_cmsw_data_validity:CMSW_VALID*/						//OK
	uint8_t major_frame_alignment_status:1; /*0: Aligned 1:Not Aligned, Applicable when  ml_cmsw_data_validity:CMSW_VALID*/			//OK
	uint8_t dtn_es_redundancy_status:1;     /*0: REDUNDANCY_OK 1: REDUNDANCY_LOSS, Applicable when dtn_es_validity:CMSW_VALID*/		//OK
	uint8_t client_mount_validity:1;	 	/*0: VALID, 1: INVALID */
#else
	uint8_t client_mount_validity:1;	 	/*0: VALID, 1: INVALID */
	uint8_t dtn_es_redundancy_status:1;     /*0: REDUNDANCY_OK 1: REDUNDANCY_LOSS, Applicable when dtn_es_validity:CMSW_VALID*/		//OK
	uint8_t major_frame_alignment_status:1; /*0: Aligned 1:Not Aligned, Applicable when  ml_cmsw_data_validity:CMSW_VALID*/			//OK
	uint8_t hsn_link_status:1; 		   		/*0:GOOD 1:BAD, Applicable when ml_cmsw_data_validity:CMSW_VALID*/						//OK
	uint8_t ptp_sync_status:1; 		   		/*0: SYNCHED 1: NOT_SYNCHED, Applicable when dtn_es_validity:CMSW_VALID*/				//OK
	uint8_t dtn_es_data_validity:1; 		/*0: CMSW_VALID, 1: CMSW_INVALID, Applicable when  ml_cmsw_data_validity:CMSW_VALID*/	//OK
	uint8_t temperature_data_status:1; /*0: DATA_AVAILABLE, 1: DATA_LOSS*/	//FMECA OK
	uint8_t voltage_data_status:1;     /*0: DATA_AVAILABLE, 1: DATA_LOSS*/	//FMECA OK
#endif

	uint16_t dtn_es_cfg_id;   /*Applicable when dtn_es_validity:CMSW_VALID*/		//OK
	uint16_t ptp_cfg_id;      /*Applicable when dtn_es_validity:CMSW_VALID*/		// OK
	uint8_t ptp_device_type;  /*Applicable when dtn_es_validity:CMSW_VALID*/		//OK
	uint8_t ptp_tod_network;

	uint8_t dtn_es_fw_vers_major;  /*Applicable when dtn_es_validity:CMSW_VALID*/	//OK
	uint8_t dtn_es_fw_vers_minor;  /*Applicable when dtn_es_validity:CMSW_VALID*/	//OK
	uint8_t dtn_es_fw_vers_bugfix; /*Applicable when dtn_es_validity:CMSW_VALID*/	//OK

	uint16_t monolith_id; 	/*Applicable when ml_cmsw_data_validity:CMSW_VALID*/   	//OK
	uint64_t timestamp; 	/*Applicable when ml_cmsw_data_validity:CMSW_VALID*/	//OK

#if CL_CMSW_BITFIELD_MSB_FIRST
	uint8_t operation_mode:2;  /* 0: IDLE, 1: COLD_START, 2: WARM_START, 3: NORMAL*/		//OK
	uint8_t reserved:5;
	uint8_t ml_cmsw_msg_data_validity:1;   /*0: CMSW_VALID, 1: CMSW_INVALID */ 		//OK
#else
	uint8_t ml_cmsw_msg_data_validity:1;   /*0: CMSW_VALID, 1: CMSW_INVALID */ 		//OK
	uint8_t reserved:5;
	uint8_t operation_mode:2;  /* 0: IDLE, 1: COLD_START, 2: WARM_START, 3: NORMAL*/		//OK
#endif

	t2080_dpm_bootloader_pbit_t t2080_dpm_bootloader_pbit;

}Cl_cmsw_dpm_status_type;


typedef struct __attribute__((packed)) {

	Cl_cmsw_generic_lrm_status_type lrm_status;

#if CL_CMSW_BITFIELD_MSB_FIRST
	uint8_t voltage_data_status:1;     	/*0: DATA_AVAILABLE, 1: DATA_LOSS*/   	//FMECA OK
	uint8_t temperature_data_status:1; 	/*0: DATA_AVAILABLE, 1: DATA_LOSS*/  	//FMECA OK
	uint8_t dtn_es_data_validity:1;  	/*0: CMSW_VALID, 1: CMSW_INVALID*/		//OK
	uint8_t dtn_sw_data_validity:1;  	/*0: CMSW_VALID, 1: CMSW_INVALID*/   	//OK
	uint8_t ptp_sync_status:1; 		 	/*0: SYNCHED 1: NOT_SYNCHED*/    		//OK
	uint8_t major_frame_alignment_status:1; /*0: Aligned 1:Not Aligned */	//OK
	uint8_t dtn_es_redundancy_status:1;     /*0: REDUNDANCY_OK 1: REDUNDANCY_LOSS, Applicable when dtn_es_validity:CMSW_VALID*/  //OK
	uint8_t reserved:1;
#else
	uint8_t reserved:1;
	uint8_t dtn_es_redundancy_status:1;     /*0: REDUNDANCY_OK 1: REDUNDANCY_LOSS, Applicable when dtn_es_validity:CMSW_VALID*/  //OK
	uint8_t major_frame_alignment_status:1; /*0: Aligned 1:Not Aligned */	//OK
	uint8_t ptp_sync_status:1; 		 	/*0: SYNCHED 1: NOT_SYNCHED*/    		//OK
	uint8_t dtn_sw_data_validity:1;  	/*0: CMSW_VALID, 1: CMSW_INVALID*/   	//OK
	uint8_t dtn_es_data_validity:1;  	/*0: CMSW_VALID, 1: CMSW_INVALID*/		//OK
	uint8_t temperature_data_status:1; 	/*0: DATA_AVAILABLE, 1: DATA_LOSS*/  	//FMECA OK
	uint8_t voltage_data_status:1;     	/*0: DATA_AVAILABLE, 1: DATA_LOSS*/   	//FMECA OK
#endif

	int32_t a653_schedule_id; /*Applicable when ml_cmsw_data_validity:CMSW_VALID*/	//OK
	uint16_t dtn_es_cfg_id;   /*Applicable when dtn_es_validity:CMSW_VALID*/ 	//OK
	uint16_t dtn_sw_cfg_id;   /*Applicable when dtn_es_validity:CMSW_VALID*/ 	//OK
	uint16_t ptp_cfg_id;      /*Applicable when dtn_es_validity:CMSW_VALID*/ 	//OK
	uint8_t ptp_device_type;  /*Applicable when dtn_es_validity:CMSW_VALID*/ 	//OK

	uint8_t dtn_es_fw_vers_major;  /*Applicable when dtn_es_validity:CMSW_VALID*/ 	//OK
	uint8_t dtn_es_fw_vers_minor;  /*Applicable when dtn_es_validity:CMSW_VALID*/ 	//OK
	uint8_t dtn_es_fw_vers_bugfix; /*Applicable when dtn_es_validity:CMSW_VALID*/ 	//OK

	uint16_t dtn_sw_port_link_status; 	/*0: GOOD_LINK 1: BAD_LINK*/    				//OK
	uint8_t dtn_sw_fw_vers_major;  	  	/*Applicable when dtn_sw_validity:CMSW_VALID*/  //OK
	uint8_t dtn_sw_fw_vers_minor;  /*Applicable when dtn_sw_validity:CMSW_VALID*/  		//OK
	uint8_t dtn_sw_fw_vers_bugfix; /*Applicable when dtn_sw_validity:CMSW_VALID*/  		//OK

	t2080_dsm_bootloader_pbit_t t2080_dsm_bootloader_pbit;

}Cl_cmsw_dsm_status_type;

typedef struct __attribute__((packed)) {

#if CL_CMSW_BITFIELD_MSB_FIRST
	uint8_t port_number:4;										//OK
	uint8_t data_validity:1;   //0: CMSW_VALID, 1: CMSW_INVALID	//OK
	uint8_t hsn_link_status:1; //0: GOOD_LINK, 1:BAD_LINK		//OK
	uint8_t reserved:2;
#else
	uint8_t reserved:2;
	uint8_t hsn_link_status:1; //0: GOOD_LINK, 1:BAD_LINK		//OK
	uint8_t data_validity:1;   //0: CMSW_VALID, 1: CMSW_INVALID	//OK
	uint8_t port_number:4;										//OK
#endif

}Cl_cmsw_hsn_port_link_status_type;

typedef struct __attribute__((packed)) {

	Cl_cmsw_generic_lrm_status_type lrm_status;

#if CL_CMSW_BITFIELD_MSB_FIRST
	uint8_t hsm_data_validity:1;		/*0: CMSW_VALID, 1: CMSW_INVALID*/											//OK
	uint8_t dsm_hsm_pcie_link_status:1;	/*0: GOOD_LINK, 1:BAD_LINK*/												//OK
	uint8_t temperature_data_status:2; 	/*0: TEMP_GOOD, 1: TEMP_DATA_LOSS, 2: TEMP_ORANGE, 3: TEMP_RED*/			//FMECA OK
	uint8_t voltage_data_status:2;     	/*0: VOLTAGE_GOOD, 1: VOLTAGE_LOSS, 2: VOLTAGE_ORANGE, 3: VOLTAGE_RED*/   	//FMECA OK
	uint8_t current_data_status:2;     	/*0: CURRENT_GOOD, 2: CURRENT_LOSS, 2: CURRENT_ORANGE, 3: CURRENT_RED*/   	//FMECA OK
#else
	uint8_t current_data_status:2;     	/*0: CURRENT_GOOD, 2: CURRENT_LOSS, 2: CURRENT_ORANGE, 3: CURRENT_RED*/   	//FMECA OK
	uint8_t voltage_data_status:2;     	/*0: VOLTAGE_GOOD, 1: VOLTAGE_LOSS, 2: VOLTAGE_ORANGE, 3: VOLTAGE_RED*/   	//FMECA OK
	uint8_t temperature_data_status:2; 	/*0: TEMP_GOOD, 1: TEMP_DATA_LOSS, 2: TEMP_ORANGE, 3: TEMP_RED*/			//FMECA OK
	uint8_t dsm_hsm_pcie_link_status:1;	/*0: GOOD_LINK, 1:BAD_LINK*/												//OK
	uint8_t hsm_data_validity:1;		/*0: CMSW_VALID, 1: CMSW_INVALID*/											//OK
#endif

	Cl_cmsw_hsn_port_link_status_type backplane_hsn_port_status_list[8];

}Cl_cmsw_hsm_status_type;


typedef struct __attribute__((packed))
{
#if CL_CMSW_BITFIELD_MSB_FIRST
	uint8_t chassis_manager_validity:1; //0: CMSW_VALID 1: CMSW_INVALID  //OK
	uint8_t chassis_manager_mode:2; 	//1: Standby 2:Active 3:Unknown, applicable when chasis_manager_validity:CMSW_VALID //	OK
	uint8_t temperature_status:1; 		//0: TEMP_OK 1: TEMP_HOT, applicable when chasis_manager_validity:CMSW_VALID  //OK
	uint8_t lrm_configuration_status:1; //0: CONFIGURATION_COMPLIANT 1: CONFIGURATION_NOT_COMPLIANT, applicable when chasis_manager_validity:CMSW_VALID //OK
	uint8_t reserved:3;
#else
	uint8_t reserved:3;
	uint8_t lrm_configuration_status:1; //0: CONFIGURATION_COMPLIANT 1: CONFIGURATION_NOT_COMPLIANT, applicable when chasis_manager_validity:CMSW_VALID //OK
	uint8_t temperature_status:1; 		//0: TEMP_OK 1: TEMP_HOT, applicable when chasis_manager_validity:CMSW_VALID  //OK
	uint8_t chassis_manager_mode:2; 	//1: Standby 2:Active 3:Unknown, applicable when chasis_manager_validity:CMSW_VALID //	OK
	uint8_t chassis_manager_validity:1; //0: CMSW_VALID 1: CMSW_INVALID  //OK
#endif

	uint32_t computer_config_mismatch_slot_list; /*Each bit for slot index, 0: LRM_OK, 1: LRM_MISMATCH*/ //OK

}Cl_cmsw_computer_status_type;

typedef struct __attribute__((packed)) {

	uint8_t message_type; //CL_CMSW_STATUS_REPORT_MSG // 1 B
	Cl_cmsw_device_id_type device_id;  //OK // 2 B

	Cl_cmsw_computer_status_type computer_status; //OK // 5 B

	Cl_cmsw_generic_lrm_status_type psm_status[PSM_COUNT_B0];  	//OK // 27 * 2 = 54 B
	Cl_cmsw_generic_lrm_status_type hum_status[HUM_COUNT_B0]; 	//OK // 27 * 2 = 54 B
	Cl_cmsw_generic_lrm_status_type smmm_status; 				//OK // 27 B
	Cl_cmsw_iocm_status_type iocm_status; 						//OK // 28 B
	Cl_cmsw_dpm_status_type dpm_status[DPM_COUNT_B0]; 			//OK // 53 * 5 = 265 B
	Cl_cmsw_dsm_status_type dsm_status[DSM_COUNT_B0]; 			//OK // 48 * 2 = 96 B
	Cl_cmsw_gpm_status_type gpm_status[GPM_COUNT_B0];  			//OK // 29 * 2 = 58 B
	Cl_cmsw_hsm_status_type hsm_status[HSM_COUNT_B0];			//OK // 36 * 2 = 72 B

}Cl_cmsw_status_report_msg_type;

/* Wire boyutu ICD ile sabit — bitfield sırası değişse de toplam değişmemeli. */
_Static_assert(sizeof(Cl_cmsw_device_id_type)          == 2,  "Cl_cmsw_device_id_type size mismatch");
_Static_assert(sizeof(Cl_cmsw_computer_status_type)    == 5,  "Cl_cmsw_computer_status_type size mismatch");
_Static_assert(sizeof(Cl_cmsw_generic_lrm_status_type) == 27, "Cl_cmsw_generic_lrm_status_type size mismatch");
_Static_assert(sizeof(Cl_cmsw_iocm_status_type)        == 28, "Cl_cmsw_iocm_status_type size mismatch");
_Static_assert(sizeof(Cl_cmsw_dpm_status_type)         == 53, "Cl_cmsw_dpm_status_type size mismatch");
_Static_assert(sizeof(Cl_cmsw_dsm_status_type)         == 48, "Cl_cmsw_dsm_status_type size mismatch");
_Static_assert(sizeof(Cl_cmsw_gpm_status_type)         == 29, "Cl_cmsw_gpm_status_type size mismatch");
_Static_assert(sizeof(Cl_cmsw_hsm_status_type)         == 36, "Cl_cmsw_hsm_status_type size mismatch");
_Static_assert(sizeof(Cl_cmsw_status_report_msg_type)  == 662, "Cl_cmsw_status_report_msg_type size mismatch");

#endif /* CL_CMSW_MESSAGE_TYPES_H */
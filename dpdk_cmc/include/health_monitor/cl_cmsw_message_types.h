#ifndef CL_CMSW_MESSAGE_TYPES_H
#define CL_CMSW_MESSAGE_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#define DPM_COUNT_B0 5U
#define DSM_COUNT_B0 2U
#define HSM_COUNT_B0 2U
#define GPM_COUNT_B0 2U
#define PSM_COUNT_B0 2U
#define HUM_COUNT_B0 2U

typedef struct __attribute__((packed))
{
	uint8_t lru_id;
	uint8_t slot_id:5;
	uint8_t reserved:1;
	uint8_t xmc_fpga:2;
}Cl_cmsw_device_id_type;

typedef struct __attribute__((packed))
{
    uint8_t patch;
    uint8_t minor;
    uint8_t major;
} pcs_firmware_version_type;

typedef struct __attribute__((packed)) {

	uint8_t slot_id:5;
	uint8_t ipmc_data_validity:1; //0: CMSW_VALID 1: CMSW_INVALID
	uint8_t operation_mode:1;   //0 : OFP_MODE, 1: MAINTENANCE_MODE
	uint8_t module_status:1;	//0: MODULE_ALIVE, 1: MODULE LOSS

	pcs_firmware_version_type  firmware_version;
	uint8_t hardware_type;
	uint32_t pcb_revision_number;
	uint32_t pcb_assembly_id;
	uint32_t mechanical_revision_number;
	uint64_t serial_id;
	uint8_t reset_counter;
	uint8_t power_status;

}Cl_cmsw_generic_lrm_status_type;


/** Indicates the result of T2080_DPM_BOOTLOADER_PBIT. */
typedef struct
{
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
} __attribute__((packed)) t2080_dpm_bootloader_pbit_t;

/** Indicates the result of T2080_DSM_BOOTLOADER_PBIT. */
typedef struct
{
    /** Indicates the result of DDR test. 0: GOOD 1: BAD */
    uint8_t ddr_test:1;
    /** Indicates the result of IFC NAND test. 0: GOOD 1: BAD */
    uint8_t ifc_nand_test:1;
    /** Indicates the result of SERDES1 PLL1 test. 0: GOOD 1: BAD */
    uint8_t serdes1_pll1_test:1;
    /** Indicates the result of SERDES1 PLL2 test. 0: GOOD 1: BAD */
    uint8_t serdes1_pll2_test:1;
    uint8_t reserved:4;
} __attribute__((packed)) t2080_dsm_bootloader_pbit_t;

typedef struct __attribute__((packed)) {

	Cl_cmsw_generic_lrm_status_type lrm_status;

	uint8_t dvi_status:1; 			   		 //0: DVI_AVAILABLE, 1: DVI_LOSS	//FMECA OK
	uint8_t voltage_data_status:1;    		 //0: DATA_AVAILABLE, 1: DATA_LOSS	//FMECA OK
	uint8_t temperature_data_status:1;		 //0: DATA_AVAILABLE, 1: DATA_LOSS	//FMEKA OK
	uint8_t reserved:5;

}Cl_cmsw_iocm_status_type;


typedef struct __attribute__((packed)) {

	Cl_cmsw_generic_lrm_status_type lrm_status; 

	uint8_t advb_tx_1_status:2; 	   		 //0: ADVB_TX_AVAILABLE 1: ADVB_TX_LOSS 2: ADVB_TX_WARNING 	//FMECA OK
	uint8_t advb_tx_2_status:2;        		 //0: ADVB_TX_AVAILABLE 1: ADVB_TX_LOSS	2: ADVB_TX_WARNING 	//FMECA OK
	uint8_t advb_tx_3_status:2;        		 //0: ADVB_TX_AVAILABLE 1: ADVB_TX_LOSS 2: ADVB_TX_WARNING 	//FMECA OK
	uint8_t advb_hm_data_status:1;     		 //0: DATA_AVAILABLE, 1: DATA_LOSS							//FMECA OK
	uint8_t advb_status:1;					 //0: DATA_AVAILABLE, 1: DATA_LOSS							//FMECA OK
	uint8_t dvi_status:2; 			   		 //0: DVI_AVAILABLE, 1: DVI_LOSS 2: DVI_WARNING 			//FMECA OK
	uint8_t voltage_data_status:1;    		 //0: DATA_AVAILABLE, 1: DATA_LOSS							//FMECA OK
	uint8_t temperature_data_status:1;		 //0: DATA_AVAILABLE, 1: DATA_LOSS							//FMECA OK

	uint8_t reserved:4;

}Cl_cmsw_gpm_status_type;


typedef struct __attribute__((packed)) {

	Cl_cmsw_generic_lrm_status_type lrm_status;

	int32_t a653_schedule_id; 		/*Applicable when  ml_cmsw_data_validity:CMSW_VALID*/		//OK

	uint8_t voltage_data_status:1;     /*0: DATA_AVAILABLE, 1: DATA_LOSS*/	//FMECA OK
	uint8_t temperature_data_status:1; /*0: DATA_AVAILABLE, 1: DATA_LOSS*/	//FMECA OK
	uint8_t dtn_es_data_validity:1; 		/*0: CMSW_VALID, 1: CMSW_INVALID, Applicable when  ml_cmsw_data_validity:CMSW_VALID*/	//OK
	uint8_t ptp_sync_status:1; 		   		/*0: SYNCHED 1: NOT_SYNCHED, Applicable when dtn_es_validity:CMSW_VALID*/				//OK
	uint8_t hsn_link_status:1; 		   		/*0:GOOD 1:BAD, Applicable when ml_cmsw_data_validity:CMSW_VALID*/						//OK
	uint8_t major_frame_alignment_status:1; /*0: Aligned 1:Not Aligned, Applicable when  ml_cmsw_data_validity:CMSW_VALID*/			//OK
	uint8_t dtn_es_redundancy_status:1;     /*0: REDUNDANCY_OK 1: REDUNDANCY_LOSS, Applicable when dtn_es_validity:CMSW_VALID*/		//OK
	uint8_t client_mount_validity:1;	 	/*0: VALID, 1: INVALID */

	uint16_t dtn_es_cfg_id;   /*Applicable when dtn_es_validity:CMSW_VALID*/		//OK
	uint16_t ptp_cfg_id;      /*Applicable when dtn_es_validity:CMSW_VALID*/		// OK
	uint8_t ptp_device_type;  /*Applicable when dtn_es_validity:CMSW_VALID*/		//OK
	uint8_t ptp_tod_network;

	uint8_t dtn_es_fw_vers_major;  /*Applicable when dtn_es_validity:CMSW_VALID*/	//OK
	uint8_t dtn_es_fw_vers_minor;  /*Applicable when dtn_es_validity:CMSW_VALID*/	//OK
	uint8_t dtn_es_fw_vers_bugfix; /*Applicable when dtn_es_validity:CMSW_VALID*/	//OK

	uint16_t monolith_id; 	/*Applicable when ml_cmsw_data_validity:CMSW_VALID*/   	//OK
	uint64_t timestamp; 	/*Applicable when ml_cmsw_data_validity:CMSW_VALID*/	//OK

	uint8_t operation_mode:2;  /* 0: IDLE, 1: COLD_START, 2: WARM_START, 3: NORMAL*/		//OK
	uint8_t reserved:5;
	uint8_t ml_cmsw_msg_data_validity:1;   /*0: CMSW_VALID, 1: CMSW_INVALID */ 		//OK

	t2080_dpm_bootloader_pbit_t t2080_dpm_bootloader_pbit;

}Cl_cmsw_dpm_status_type;


typedef struct __attribute__((packed)) {

	Cl_cmsw_generic_lrm_status_type lrm_status;

	uint8_t voltage_data_status:1;     	/*0: DATA_AVAILABLE, 1: DATA_LOSS*/   	//FMECA OK
	uint8_t temperature_data_status:1; 	/*0: DATA_AVAILABLE, 1: DATA_LOSS*/  	//FMECA OK
	uint8_t dtn_es_data_validity:1;  	/*0: CMSW_VALID, 1: CMSW_INVALID*/		//OK
	uint8_t dtn_sw_data_validity:1;  	/*0: CMSW_VALID, 1: CMSW_INVALID*/   	//OK
	uint8_t ptp_sync_status:1; 		 	/*0: SYNCHED 1: NOT_SYNCHED*/    		//OK
	uint8_t major_frame_alignment_status:1; /*0: Aligned 1:Not Aligned */	//OK
	uint8_t dtn_es_redundancy_status:1;     /*0: REDUNDANCY_OK 1: REDUNDANCY_LOSS, Applicable when dtn_es_validity:CMSW_VALID*/  //OK
	uint8_t reserved:1;

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

	uint8_t port_number:4;										//OK
	uint8_t data_validity:1;   //0: CMSW_VALID, 1: CMSW_INVALID	//OK
	uint8_t hsn_link_status:1; //0: GOOD_LINK, 1:BAD_LINK		//OK
	uint8_t reserved:2;

}Cl_cmsw_hsn_port_link_status_type;

typedef struct __attribute__((packed)) {

	Cl_cmsw_generic_lrm_status_type lrm_status;

	uint8_t hsm_data_validity:1;		/*0: CMSW_VALID, 1: CMSW_INVALID*/											//OK
	uint8_t dsm_hsm_pcie_link_status:1;	/*0: GOOD_LINK, 1:BAD_LINK*/												//OK
	uint8_t temperature_data_status:2; 	/*0: DATA_AVAILABLE, 1: DATA_LOSS, 2: TEMP_ORANGE, 3: TEMP_RED*/			//FMECA OK
	uint8_t voltage_data_status:2;     	/*0: VOLTAGE_GOOD, 1: VOLTAGE_LOSS, 2: VOLTAGE_ORANGE, 3: VOLTAGE_RED*/   	//FMECA OK
	uint8_t current_data_status:2;     	/*0: CURRENT_GOOD, 2: CURRENT_LOSS, 2: CURRENT_ORANGE, 3: CURRENT_RED*/   	//FMECA OK

	Cl_cmsw_hsn_port_link_status_type backplane_hsn_port_status_list[8];

}Cl_cmsw_hsm_status_type;


typedef struct __attribute__((packed))
{
	uint8_t chassis_manager_validity:1; //0: CMSW_VALID 1: CMSW_INVALID  //OK
	uint8_t chassis_manager_mode:2; 	//1: Standby 2:Active 3:Unknown, applicable when chasis_manager_validity:CMSW_VALID //	OK
	uint8_t temperature_status:1; 		//0: TEMP_OK 1: TEMP_HOT, applicable when chasis_manager_validity:CMSW_VALID  //OK
	uint8_t lrm_configuration_status:1; //0: CONFIGURATION_COMPLIANT 1: CONFIGURATION_NOT_COMPLIANT, applicable when chasis_manager_validity:CMSW_VALID //OK
	uint8_t reserved:3;

	uint32_t computer_config_mismatch_slot_list; /*Each bit for slot index, 0: LRM_OK, 1: LRM_MISMATCH*/ //OK

}Cl_cmsw_computer_status_type;

typedef struct __attribute__((packed)) {

	uint8_t message_type; //CL_CMSW_STATUS_REPORT_MSG // 1
	Cl_cmsw_device_id_type device_id;  //OK // 4

	Cl_cmsw_computer_status_type computer_status; //OK // 9

	Cl_cmsw_generic_lrm_status_type psm_status[PSM_COUNT_B0];  	//OK // (27 + pcs) * 2 
	Cl_cmsw_generic_lrm_status_type hum_status[HUM_COUNT_B0]; 	//OK // (27 + pcs) * 2 
	Cl_cmsw_generic_lrm_status_type smmm_status; 				//OK // (27 + pcs)
	Cl_cmsw_iocm_status_type iocm_status; 						//OK // 36 + pcs
	Cl_cmsw_dpm_status_type dpm_status[DPM_COUNT_B0]; 			//OK // (59 + pcs) * 5
	Cl_cmsw_dsm_status_type dsm_status[DSM_COUNT_B0]; 			//OK // (59 + pcs) * 2
	Cl_cmsw_gpm_status_type gpm_status[GPM_COUNT_B0];  			//OK // (59 + pcs) * 2
	Cl_cmsw_hsm_status_type hsm_status[HSM_COUNT_B0];			//OK // (59 + pcs) * 2

}Cl_cmsw_status_report_msg_type;

#endif /* CL_CMSW_MESSAGE_TYPES_H */

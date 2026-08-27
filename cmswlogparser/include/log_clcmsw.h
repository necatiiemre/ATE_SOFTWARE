#pragma once
#define MAXIMUM_MODULES_IN_A_RACK 17U

#define DPM_COUNT_B0 5U
#define DSM_COUNT_B0 2U
#define HSM_COUNT_B0 2U
#define GPM_COUNT_B0 2U
#define PSM_COUNT_B0 2U
#define HUM_COUNT_B0 2U

#define CMSW_INVALID 1U
#define CMSW_VALID 0

#define TEMP_OK 0U
#define TEMP_HOT 1U

#define MODULE_ALIVE 0U
#define MODULE_LOSS 1U

#define SYNCHED 0
#define NOT_SYNCHED 1U

#define REDUNDANCY_OK 0
#define REDUNDANCY_LOSS 1U

#define DATA_AVAILABLE 0
#define DATA_LOSS 1U
#define TEMP_ORANGE 2U
#define TEMP_RED 3U

#define CURRENT_GOOD 0
#define CURRENT_LOSS 1U
#define CURRENT_ORANGE 2U
#define CURRENT_RED 3U

#define VOLTAGE_GOOD 0
#define VOLTAGE_LOSS 1U
#define VOLTAGE_ORANGE 2U
#define VOLTAGE_RED 3U

#define GOOD_LINK 0
#define BAD_LINK 1U

#define GOOD 0
#define BAD 1U

#define ADVB_TX_AVAILABLE 0
#define ADVB_TX_LOSS 1U
#define ADVB_TX_WARNING 2U

#define DVI_AVAILABLE 0
#define DVI_LOSS 1U
#define DVI_WARNING 2U

#define ALIGNED 0
#define NOT_ALIGNED 1U

#define OFP_MODE 0
#define MAINTENANCE_MODE 1U

#define LINK_OK 0
#define LINK_LOSS 1U



#include <stdint.h>
typedef enum
{
	CLCMSW_LOG_INIT_ERROR,
	CLCMSW_LOG_START_GENERAL_TASK_ERROR,
	CLCMSW_LOG_START_APERIODIC_TASK_ERROR,
	CLCMSW_LOG_SET_INITIAL_SCHEDULE_ERROR,
	CLCMSW_LOG_START_PCIE_RCFG_TASK_ERROR,
	CLCMSW_ENTRY_POINT_IPMC_TASK,
	CLCMSW_ENTRY_POINT_PERIODIC_PROCESS,
	CLCMSW_ENRTY_POINT_PCIE_RECONFIG_TASK,
    CLCMSW_LOGTYPE_MAX
} CLCMSW_LOG_TYPE;

const char *CLCMSW_LOG_TYPE_strings[] = {
	"CLCMSW_LOG_INIT_ERROR",
	"CLCMSW_LOG_START_GENERAL_TASK_ERROR",
	"CLCMSW_LOG_START_APERIODIC_TASK_ERROR",
	"CLCMSW_LOG_SET_INITIAL_SCHEDULE_ERROR",
	"CLCMSW_LOG_START_PCIE_RCFG_TASK_ERROR",
	"CLCMSW_ENTRY_POINT_IPMC_TASK",
	"CLCMSW_ENTRY_POINT_PERIODIC_PROCESS",
	"CLCMSW_ENRTY_POINT_PCIE_RECONFIG_TASK",
    "UNKNOWN"
};


typedef struct __attribute__((packed))
{
    CLCMSW_LOG_TYPE log_type;
    uint8_t log_severity;
    int64_t first_param;
    int64_t second_param;
} clcmsw_log_entry_t;

typedef struct __attribute__((packed))
{
    uint64_t log_timestamp;
    uint32_t log_entry_count;
    uint32_t log_drop_count;
} clcmsw_log_frame_header_t;

typedef struct __attribute__((packed))
{
    clcmsw_log_frame_header_t log_frame_header;
    clcmsw_log_entry_t log_entry_list[256];
} CLCMSW_LOG_FRAME_TYPE;


typedef struct __attribute__((packed))
{
	uint8_t lru_id;
	uint8_t slot_id:5;
	uint8_t reserved:1;
	uint8_t xmc_fpga:2;
}Cl_cmsw_device_id_type;

typedef struct __attribute__((packed)) pcs_firmware_version_type
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

	uint8_t port_number:4;											//OK
	uint8_t data_validity:1;   /*0: CMSW_VALID, 1: CMSW_INVALID*/	//OK
	uint8_t hsn_link_status:1; /*0: GOOD_LINK, 1:BAD_LINK*/ 		//OK
	uint8_t reserved:2;

}Cl_cmsw_hsn_port_link_status_type;

typedef struct __attribute__((packed)) {

	Cl_cmsw_generic_lrm_status_type lrm_status;

	uint8_t hsm_data_validity:1;		/*0: CMSW_VALID, 1: CMSW_INVALID*/ 											//OK
	uint8_t dsm_hsm_pcie_link_status:1;	/*0: GOOD_LINK, 1:BAD_LINK*/												//OK
	uint8_t temperature_data_status:2; 	/*0: DATA_AVAILABLE, 1: DATA_LOSS, 2: TEMP_ORANGE, 3: TEMP_RED*/			//FMECA OK
	uint8_t voltage_data_status:2;     	/*0: VOLTAGE_GOOD, 1: VOLTAGE_LOSS, 2: VOLTAGE_ORANGE, 3: VOLTAGE_RED*/   	//FMECA OK
	uint8_t current_data_status:2;     	/*0: CURRENT_GOOD, 2: CURRENT_LOSS, 2: CURRENT_ORANGE, 3: CURRENT_RED*/   	//FMECA OK

	Cl_cmsw_hsn_port_link_status_type backplane_hsn_port_status_list[8];

}Cl_cmsw_hsm_status_type;


typedef struct __attribute__((packed))
{
	uint8_t chassis_manager_validity:1; /*0: CMSW_VALID 1: CMSW_INVALID*/  																					//OK
	uint8_t chassis_manager_mode:2; 	/*1: Standby 2:Active 3:Unknown, applicable when chasis_manager_validity:CMSW_VALID*/ 								//OK
	uint8_t temperature_status:1; 		/*0: TEMP_OK 1: TEMP_HOT, applicable when chasis_manager_validity:CMSW_VALID*/  									//OK
	uint8_t lrm_configuration_status:1; /*0: CONFIGURATION_COMPLIANT 1: CONFIGURATION_NOT_COMPLIANT, applicable when chasis_manager_validity:CMSW_VALID*/	//OK
	uint8_t reserved:3;

	uint32_t computer_config_mismatch_slot_list; /*Each bit for slot index, 0: LRM_OK, 1: LRM_MISMATCH*/ 													//OK

}Cl_cmsw_computer_status_type;

typedef struct __attribute__((packed)) {

	uint8_t message_type; //CL_CMSW_STATUS_REPORT_MSG
	Cl_cmsw_device_id_type device_id;  							//OK

	Cl_cmsw_computer_status_type computer_status; 				//OK

	Cl_cmsw_generic_lrm_status_type psm_status[PSM_COUNT_B0];  	//OK
	Cl_cmsw_generic_lrm_status_type hum_status[HUM_COUNT_B0]; 	//OK
	Cl_cmsw_generic_lrm_status_type smmm_status; 				//OK
	Cl_cmsw_iocm_status_type iocm_status; 						//OK
	Cl_cmsw_dpm_status_type dpm_status[DPM_COUNT_B0]; 			//OK
	Cl_cmsw_dsm_status_type dsm_status[DSM_COUNT_B0]; 			//OK
	Cl_cmsw_gpm_status_type gpm_status[GPM_COUNT_B0];  			//OK
	Cl_cmsw_hsm_status_type hsm_status[HSM_COUNT_B0];			//OK

}Cl_cmsw_status_report_msg_type;

static int cl_cmsw_log_parser_header(FILE *outfp) {
	fprintf(outfp,  "DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP,LOG PAYLOAD LENGTH,");

	fprintf(outfp, "MESSAGE_TYPE,");
	fprintf(outfp, "LRU_ID,");
	fprintf(outfp, "SLOT_ID,");
	fprintf(outfp, "RESERVED,");
	fprintf(outfp, "XMC_FPGA,");
	fprintf(outfp, "CHASSIS_MANAGER_VALIDITY,");
	fprintf(outfp, "CHASSIS_MANAGER_MODE,");
	fprintf(outfp, "TEMPERATURE_STATUS,");
	fprintf(outfp, "LRM_CONFIGURATION_STATUS,");
	fprintf(outfp, "RESERVED,");
	fprintf(outfp, "COMPUTER_CONFIG_MISMATCH_SLOT_LIST,");
	
	for(int i = 0; i < PSM_COUNT_B0; ++i) {
		fprintf(outfp, "PSM_STATUS[%u].slot_id,", i);
		fprintf(outfp, "PSM_STATUS[%u].ipmc_data_validity,", i);
		fprintf(outfp, "PSM_STATUS[%u].operation_mode,", i);
		fprintf(outfp, "PSM_STATUS[%u].module_status,", i);
		fprintf(outfp, "PSM_STATUS[%u].firmware_version.patch,", i);
		fprintf(outfp, "PSM_STATUS[%u].firmware_version.minor,", i);
		fprintf(outfp, "PSM_STATUS[%u].firmware_version.major,", i);
		fprintf(outfp, "PSM_STATUS[%u].hardware_type,", i);
		fprintf(outfp, "PSM_STATUS[%u].pcb_revision_number,", i);
		fprintf(outfp, "PSM_STATUS[%u].pcb_assembly_id,", i);
		fprintf(outfp, "PSM_STATUS[%u].mechanical_revision_number,", i);
		fprintf(outfp, "PSM_STATUS[%u].serial_id,", i);
		fprintf(outfp, "PSM_STATUS[%u].reset_counter,", i);
		fprintf(outfp, "PSM_STATUS[%u].power_status,", i);
	}

	for(int i = 0; i < HUM_COUNT_B0; ++i) {
		fprintf(outfp, "HUM_STATUS[%u].slot_id,", i);
		fprintf(outfp, "HUM_STATUS[%u].ipmc_data_validity,", i);
		fprintf(outfp, "HUM_STATUS[%u].operation_mode,", i);
		fprintf(outfp, "HUM_STATUS[%u].module_status,", i);
		fprintf(outfp, "HUM_STATUS[%u].firmware_version.patch,", i);
		fprintf(outfp, "HUM_STATUS[%u].firmware_version.minor,", i);
		fprintf(outfp, "HUM_STATUS[%u].firmware_version.major,", i);
		fprintf(outfp, "HUM_STATUS[%u].hardware_type,", i);
		fprintf(outfp, "HUM_STATUS[%u].pcb_revision_number,", i);
		fprintf(outfp, "HUM_STATUS[%u].pcb_assembly_id,", i);
		fprintf(outfp, "HUM_STATUS[%u].mechanical_revision_number,", i);
		fprintf(outfp, "HUM_STATUS[%u].serial_id,", i);
		fprintf(outfp, "HUM_STATUS[%u].reset_counter,", i);
		fprintf(outfp, "HUM_STATUS[%u].power_status,", i);
	}

	fprintf(outfp, "smmm_status.slot_id,");
	fprintf(outfp, "smmm_status.ipmc_data_validity,");
	fprintf(outfp, "smmm_status.operation_mode,");
	fprintf(outfp, "smmm_status.module_status,");
	fprintf(outfp, "smmm_status.firmware_version.patch,");
	fprintf(outfp, "smmm_status.firmware_version.minor,");
	fprintf(outfp, "smmm_status.firmware_version.major,");
	fprintf(outfp, "smmm_status.hardware_type,");
	fprintf(outfp, "smmm_status.pcb_revision_number,");
	fprintf(outfp, "smmm_status.pcb_assembly_id,");
	fprintf(outfp, "smmm_status.mechanical_revision_number,");
	fprintf(outfp, "smmm_status.serial_id,");
	fprintf(outfp, "smmm_status.reset_counter,");
	fprintf(outfp, "smmm_status.power_status,");

	fprintf(outfp, "iocm_status.lrm_status.slot_id,");
	fprintf(outfp, "iocm_status.lrm_status.ipmc_data_validity,");
	fprintf(outfp, "iocm_status.lrm_status.operation_mode,");
	fprintf(outfp, "iocm_status.lrm_status.module_status,");
	fprintf(outfp, "iocm_status.lrm_status.firmware_version.patch,");
	fprintf(outfp, "iocm_status.lrm_status.firmware_version.minor,");
	fprintf(outfp, "iocm_status.lrm_status.firmware_version.major,");
	fprintf(outfp, "iocm_status.lrm_status.hardware_type,");
	fprintf(outfp, "iocm_status.lrm_status.pcb_revision_number,");
	fprintf(outfp, "iocm_status.lrm_status.pcb_assembly_id,");
	fprintf(outfp, "iocm_status.lrm_status.mechanical_revision_number,");
	fprintf(outfp, "iocm_status.lrm_status.serial_id,");
	fprintf(outfp, "iocm_status.lrm_status.reset_counter,");
	fprintf(outfp, "iocm_status.lrm_status.power_status,");
	fprintf(outfp, "iocm_status.dvi_status,");
	fprintf(outfp, "iocm_status.voltage_data_status,");
	fprintf(outfp, "iocm_status.temperature_data_status,");
	fprintf(outfp, "iocm_status.reserved,");

	for(int i = 0; i < DPM_COUNT_B0; ++i) {
		fprintf(outfp, "dpm_status[%u].lrm_status.slot_id,", i);
		fprintf(outfp, "dpm_status[%u].lrm_status.ipmc_data_validity,", i);
		fprintf(outfp, "dpm_status[%u].lrm_status.operation_mode,", i);
		fprintf(outfp, "dpm_status[%u].lrm_status.module_status,", i);
		fprintf(outfp, "dpm_status[%u].lrm_status.firmware_version.patch,", i);
		fprintf(outfp, "dpm_status[%u].lrm_status.firmware_version.minor,", i);
		fprintf(outfp, "dpm_status[%u].lrm_status.firmware_version.major,", i);
		fprintf(outfp, "dpm_status[%u].lrm_status.hardware_type,", i);
		fprintf(outfp, "dpm_status[%u].lrm_status.pcb_revision_number,", i);
		fprintf(outfp, "dpm_status[%u].lrm_status.pcb_assembly_id,", i);
		fprintf(outfp, "dpm_status[%u].lrm_status.mechanical_revision_number,", i);
		fprintf(outfp, "dpm_status[%u].lrm_status.serial_id,", i);
		fprintf(outfp, "dpm_status[%u].lrm_status.reset_counter,", i);
		fprintf(outfp, "dpm_status[%u].lrm_status.power_status,", i);
		fprintf(outfp, "dpm_status[%u].a653_schedule_id,", i);
		fprintf(outfp, "dpm_status[%u].voltage_data_status,", i);
		fprintf(outfp, "dpm_status[%u].temperature_data_status,", i);
		fprintf(outfp, "dpm_status[%u].dtn_es_data_validity,", i);
		fprintf(outfp, "dpm_status[%u].ptp_sync_status,", i);
		fprintf(outfp, "dpm_status[%u].hsn_link_status,", i);
		fprintf(outfp, "dpm_status[%u].major_frame_alignment_status,", i);
		fprintf(outfp, "dpm_status[%u].dtn_es_redundancy_status,", i);
		fprintf(outfp, "dpm_status[%u].client_mount_validity,", i);
		fprintf(outfp, "dpm_status[%u].dtn_es_cfg_id,", i);
		fprintf(outfp, "dpm_status[%u].ptp_cfg_id,", i);
		fprintf(outfp, "dpm_status[%u].ptp_device_type,", i);
		fprintf(outfp, "dpm_status[%u].ptp_tod_network,", i);
		fprintf(outfp, "dpm_status[%u].dtn_es_fw_vers_major,", i);
		fprintf(outfp, "dpm_status[%u].dtn_es_fw_vers_minor,", i);
		fprintf(outfp, "dpm_status[%u].dtn_es_fw_vers_bugfix,", i);
		fprintf(outfp, "dpm_status[%u].monolith_id,", i);
		fprintf(outfp, "dpm_status[%u].timestamp,", i);
		fprintf(outfp, "dpm_status[%u].operation_mode,", i);
		fprintf(outfp, "dpm_status[%u].reserved,", i);
		fprintf(outfp, "dpm_status[%u].ml_cmsw_msg_data_validity,", i);
		fprintf(outfp, "dpm_status[%u].t2080_dpm_bootloader_pbit.ddr_test,", i);
		fprintf(outfp, "dpm_status[%u].t2080_dpm_bootloader_pbit.ifc_nand_test,", i);
		fprintf(outfp, "dpm_status[%u].t2080_dpm_bootloader_pbit.serdes1_pll1_test,", i);
		fprintf(outfp, "dpm_status[%u].t2080_dpm_bootloader_pbit.serdes2_pll1_test,", i);
		fprintf(outfp, "dpm_status[%u].t2080_dpm_bootloader_pbit.serdes2_pll2_test,", i);
		fprintf(outfp, "dpm_status[%u].t2080_dpm_bootloader_pbit.reserved,", i);
	}

	for(int i = 0; i < DSM_COUNT_B0; ++i) {
		fprintf(outfp, "dsm_status[%u].lrm_status.slot_id,", i);
		fprintf(outfp, "dsm_status[%u].lrm_status.ipmc_data_validity,", i);
		fprintf(outfp, "dsm_status[%u].lrm_status.operation_mode,", i);
		fprintf(outfp, "dsm_status[%u].lrm_status.module_status,", i);
		fprintf(outfp, "dsm_status[%u].lrm_status.firmware_version.patch,", i);
		fprintf(outfp, "dsm_status[%u].lrm_status.firmware_version.minor,", i);
		fprintf(outfp, "dsm_status[%u].lrm_status.firmware_version.major,", i);
		fprintf(outfp, "dsm_status[%u].lrm_status.hardware_type,", i);
		fprintf(outfp, "dsm_status[%u].lrm_status.pcb_revision_number,", i);
		fprintf(outfp, "dsm_status[%u].lrm_status.pcb_assembly_id,", i);
		fprintf(outfp, "dsm_status[%u].lrm_status.mechanical_revision_number,", i);
		fprintf(outfp, "dsm_status[%u].lrm_status.serial_id,", i);
		fprintf(outfp, "dsm_status[%u].lrm_status.reset_counter,", i);
		fprintf(outfp, "dsm_status[%u].lrm_status.power_status,", i);
		fprintf(outfp, "dsm_status[%u].voltage_data_status,", i);
		fprintf(outfp, "dsm_status[%u].temperature_data_status,", i);
		fprintf(outfp, "dsm_status[%u].dtn_es_data_validity,", i);
		fprintf(outfp, "dsm_status[%u].dtn_sw_data_validity,", i);
		fprintf(outfp, "dsm_status[%u].ptp_sync_status,", i);
		fprintf(outfp, "dsm_status[%u].major_frame_alignment_status,", i);
		fprintf(outfp, "dsm_status[%u].dtn_es_redundancy_status,", i);
		fprintf(outfp, "dsm_status[%u].reserved,", i);
		fprintf(outfp, "dsm_status[%u].a653_schedule_id,", i);
		fprintf(outfp, "dsm_status[%u].dtn_es_cfg_id,", i);
		fprintf(outfp, "dsm_status[%u].dtn_sw_cfg_id,", i);
		fprintf(outfp, "dsm_status[%u].ptp_cfg_id,", i);
		fprintf(outfp, "dsm_status[%u].ptp_device_type,", i);
		fprintf(outfp, "dsm_status[%u].dtn_es_fw_vers_major,", i);
		fprintf(outfp, "dsm_status[%u].dtn_es_fw_vers_minor,", i);
		fprintf(outfp, "dsm_status[%u].dtn_es_fw_vers_bugfix,", i);
		fprintf(outfp, "dsm_status[%u].dtn_sw_port_link_status,", i);
		fprintf(outfp, "dsm_status[%u].dtn_sw_fw_vers_major,", i);
		fprintf(outfp, "dsm_status[%u].dtn_sw_fw_vers_minor,", i);
		fprintf(outfp, "dsm_status[%u].dtn_sw_fw_vers_bugfix,", i);
		fprintf(outfp, "dsm_status[%u].t2080_dsm_bootloader_pbit.ddr_test,", i);
		fprintf(outfp, "dsm_status[%u].t2080_dsm_bootloader_pbit.ifc_nand_test,", i);
		fprintf(outfp, "dsm_status[%u].t2080_dsm_bootloader_pbit.serdes1_pll1_test,", i);
		fprintf(outfp, "dsm_status[%u].t2080_dsm_bootloader_pbit.serdes1_pll2_test,", i);
		fprintf(outfp, "dsm_status[%u].t2080_dsm_bootloader_pbit.reserved,", i);
	}

	for(int i = 0; i < GPM_COUNT_B0; ++i) {
		fprintf(outfp, "gpm_status[%u].lrm_status.slot_id,", i);
		fprintf(outfp, "gpm_status[%u].lrm_status.ipmc_data_validity,", i);
		fprintf(outfp, "gpm_status[%u].lrm_status.operation_mode,", i);
		fprintf(outfp, "gpm_status[%u].lrm_status.module_status,", i);
		fprintf(outfp, "gpm_status[%u].lrm_status.firmware_version.patch,", i);
		fprintf(outfp, "gpm_status[%u].lrm_status.firmware_version.minor,", i);
		fprintf(outfp, "gpm_status[%u].lrm_status.firmware_version.major,", i);
		fprintf(outfp, "gpm_status[%u].lrm_status.hardware_type,", i);
		fprintf(outfp, "gpm_status[%u].lrm_status.pcb_revision_number,", i);
		fprintf(outfp, "gpm_status[%u].lrm_status.pcb_assembly_id,", i);
		fprintf(outfp, "gpm_status[%u].lrm_status.mechanical_revision_number,", i);
		fprintf(outfp, "gpm_status[%u].lrm_status.serial_id,", i);
		fprintf(outfp, "gpm_status[%u].lrm_status.reset_counter,", i);
		fprintf(outfp, "gpm_status[%u].lrm_status.power_status,", i);
		fprintf(outfp, "gpm_status[%u].advb_tx_1_status,", i);
		fprintf(outfp, "gpm_status[%u].advb_tx_2_status,", i);
		fprintf(outfp, "gpm_status[%u].advb_tx_3_status,", i);
		fprintf(outfp, "gpm_status[%u].advb_hm_data_status,", i);
		fprintf(outfp, "gpm_status[%u].advb_status,", i);
		fprintf(outfp, "gpm_status[%u].dvi_status,", i);
		fprintf(outfp, "gpm_status[%u].voltage_data_status,", i);
		fprintf(outfp, "gpm_status[%u].temperature_data_status,", i);
		fprintf(outfp, "gpm_status[%u].reserved,", i);
	}

	for(int i = 0; i < HSM_COUNT_B0; ++i) {
		fprintf(outfp, "hsm_status[%u].lrm_status.slot_id,", i);
		fprintf(outfp, "hsm_status[%u].lrm_status.ipmc_data_validity,", i);
		fprintf(outfp, "hsm_status[%u].lrm_status.operation_mode,", i);
		fprintf(outfp, "hsm_status[%u].lrm_status.module_status,", i);
		fprintf(outfp, "hsm_status[%u].lrm_status.firmware_version.patch,", i);
		fprintf(outfp, "hsm_status[%u].lrm_status.firmware_version.minor,", i);
		fprintf(outfp, "hsm_status[%u].lrm_status.firmware_version.major,", i);
		fprintf(outfp, "hsm_status[%u].lrm_status.hardware_type,", i);
		fprintf(outfp, "hsm_status[%u].lrm_status.pcb_revision_number,", i);
		fprintf(outfp, "hsm_status[%u].lrm_status.pcb_assembly_id,", i);
		fprintf(outfp, "hsm_status[%u].lrm_status.mechanical_revision_number,", i);
		fprintf(outfp, "hsm_status[%u].lrm_status.serial_id,", i);
		fprintf(outfp, "hsm_status[%u].lrm_status.reset_counter,", i);
		fprintf(outfp, "hsm_status[%u].lrm_status.power_status,", i);
		fprintf(outfp, "hsm_status[%u].hsm_data_validity,", i);
		fprintf(outfp, "hsm_status[%u].dsm_hsm_pcie_link_status,", i);
		fprintf(outfp, "hsm_status[%u].temperature_data_status,", i);
		fprintf(outfp, "hsm_status[%u].voltage_data_status,", i);
		fprintf(outfp, "hsm_status[%u].current_data_status,", i);
		for(int j = 0; j < 8; ++j) {
			fprintf(outfp, "hsm_status[%u].backplane_hsn_port_status_list[%u].port_number,", i, j);
			fprintf(outfp, "hsm_status[%u].backplane_hsn_port_status_list[%u].data_validity,", i, j);
			fprintf(outfp, "hsm_status[%u].backplane_hsn_port_status_list[%u].hsn_link_status,", i, j);
			fprintf(outfp, "hsm_status[%u].backplane_hsn_port_status_list[%u].reserved%s", i, j, ((j == 7) && (i == (HSM_COUNT_B0 - 1))) ? "\n" : ",");
		}
	}
}

static int cl_cmsw_log_endiannes_conversion(Cl_cmsw_status_report_msg_type *cl_cmsw) {

	cl_cmsw->computer_status.computer_config_mismatch_slot_list = ntohl(cl_cmsw->computer_status.computer_config_mismatch_slot_list);

	for(int i = 0; i < PSM_COUNT_B0; ++i) {
		cl_cmsw->psm_status[i].pcb_revision_number = ntohl(cl_cmsw->psm_status[i].pcb_revision_number);
		cl_cmsw->psm_status[i].pcb_assembly_id = ntohl(cl_cmsw->psm_status[i].pcb_assembly_id);
		cl_cmsw->psm_status[i].mechanical_revision_number = ntohl(cl_cmsw->psm_status[i].mechanical_revision_number);
		cl_cmsw->psm_status[i].serial_id = ntohll(cl_cmsw->psm_status[i].serial_id);
	}

	for(int i = 0; i < HUM_COUNT_B0; ++i) {
		cl_cmsw->hum_status[i].pcb_revision_number = ntohl(cl_cmsw->hum_status[i].pcb_revision_number);
		cl_cmsw->hum_status[i].pcb_assembly_id = ntohl(cl_cmsw->hum_status[i].pcb_assembly_id);
		cl_cmsw->hum_status[i].mechanical_revision_number = ntohl(cl_cmsw->hum_status[i].mechanical_revision_number);
		cl_cmsw->hum_status[i].serial_id = ntohll(cl_cmsw->hum_status[i].serial_id);
	}

	cl_cmsw->smmm_status.pcb_revision_number = ntohl(cl_cmsw->smmm_status.pcb_revision_number);
	cl_cmsw->smmm_status.pcb_assembly_id = ntohl(cl_cmsw->smmm_status.pcb_assembly_id);
	cl_cmsw->smmm_status.mechanical_revision_number = ntohl(cl_cmsw->smmm_status.mechanical_revision_number);
	cl_cmsw->smmm_status.serial_id = ntohll(cl_cmsw->smmm_status.serial_id);

	cl_cmsw->iocm_status.lrm_status.pcb_revision_number = ntohl(cl_cmsw->iocm_status.lrm_status.pcb_revision_number);
	cl_cmsw->iocm_status.lrm_status.pcb_assembly_id = ntohl(cl_cmsw->iocm_status.lrm_status.pcb_assembly_id);
	cl_cmsw->iocm_status.lrm_status.mechanical_revision_number = ntohl(cl_cmsw->iocm_status.lrm_status.mechanical_revision_number);
	cl_cmsw->iocm_status.lrm_status.serial_id = ntohll(cl_cmsw->iocm_status.lrm_status.serial_id);

	for(int i = 0; i < DPM_COUNT_B0; ++i) {
		cl_cmsw->dpm_status[i].lrm_status.pcb_revision_number = ntohl(cl_cmsw->dpm_status[i].lrm_status.pcb_revision_number);
		cl_cmsw->dpm_status[i].lrm_status.pcb_assembly_id = ntohl(cl_cmsw->dpm_status[i].lrm_status.pcb_assembly_id);
		cl_cmsw->dpm_status[i].lrm_status.mechanical_revision_number = ntohl(cl_cmsw->dpm_status[i].lrm_status.mechanical_revision_number);
		cl_cmsw->dpm_status[i].lrm_status.serial_id = ntohll(cl_cmsw->dpm_status[i].lrm_status.serial_id);
		cl_cmsw->dpm_status[i].a653_schedule_id = ntohl(cl_cmsw->dpm_status[i].a653_schedule_id);
		cl_cmsw->dpm_status[i].dtn_es_cfg_id = ntohs(cl_cmsw->dpm_status[i].dtn_es_cfg_id);
		cl_cmsw->dpm_status[i].ptp_cfg_id = ntohs(cl_cmsw->dpm_status[i].ptp_cfg_id);
		cl_cmsw->dpm_status[i].monolith_id = ntohs(cl_cmsw->dpm_status[i].monolith_id);
		cl_cmsw->dpm_status[i].timestamp = ntohll(cl_cmsw->dpm_status[i].timestamp);
	}

	for(int i = 0; i < DSM_COUNT_B0; ++i) {
		cl_cmsw->dsm_status[i].lrm_status.pcb_revision_number = ntohl(cl_cmsw->dsm_status[i].lrm_status.pcb_revision_number);
		cl_cmsw->dsm_status[i].lrm_status.pcb_assembly_id = ntohl(cl_cmsw->dsm_status[i].lrm_status.pcb_assembly_id);
		cl_cmsw->dsm_status[i].lrm_status.mechanical_revision_number = ntohl(cl_cmsw->dsm_status[i].lrm_status.mechanical_revision_number);
		cl_cmsw->dsm_status[i].lrm_status.serial_id = ntohll(cl_cmsw->dsm_status[i].lrm_status.serial_id);
		cl_cmsw->dsm_status[i].dtn_sw_port_link_status = ntohs(cl_cmsw->dsm_status[i].dtn_sw_port_link_status);
		cl_cmsw->dsm_status[i].a653_schedule_id = ntohl(cl_cmsw->dsm_status[i].a653_schedule_id);
		cl_cmsw->dsm_status[i].dtn_es_cfg_id = ntohs(cl_cmsw->dsm_status[i].dtn_es_cfg_id);
		cl_cmsw->dsm_status[i].dtn_sw_cfg_id = ntohs(cl_cmsw->dsm_status[i].dtn_sw_cfg_id);
		cl_cmsw->dsm_status[i].ptp_cfg_id = ntohs(cl_cmsw->dsm_status[i].ptp_cfg_id);
	}

	for(int i = 0; i < GPM_COUNT_B0; ++i) {
		cl_cmsw->gpm_status[i].lrm_status.pcb_revision_number = ntohl(cl_cmsw->gpm_status[i].lrm_status.pcb_revision_number);
		cl_cmsw->gpm_status[i].lrm_status.pcb_assembly_id = ntohl(cl_cmsw->gpm_status[i].lrm_status.pcb_assembly_id);
		cl_cmsw->gpm_status[i].lrm_status.mechanical_revision_number = ntohl(cl_cmsw->gpm_status[i].lrm_status.mechanical_revision_number);
		cl_cmsw->gpm_status[i].lrm_status.serial_id = ntohll(cl_cmsw->gpm_status[i].lrm_status.serial_id);
	}

	for(int i = 0; i < HSM_COUNT_B0; ++i) {
		cl_cmsw->hsm_status[i].lrm_status.pcb_revision_number = ntohl(cl_cmsw->hsm_status[i].lrm_status.pcb_revision_number);
		cl_cmsw->hsm_status[i].lrm_status.pcb_assembly_id = ntohl(cl_cmsw->hsm_status[i].lrm_status.pcb_assembly_id);
		cl_cmsw->hsm_status[i].lrm_status.mechanical_revision_number = ntohl(cl_cmsw->hsm_status[i].lrm_status.mechanical_revision_number);
		cl_cmsw->hsm_status[i].lrm_status.serial_id = ntohll(cl_cmsw->hsm_status[i].lrm_status.serial_id);
	}

}

static int cl_cmsw_log_parser_data(FILE *outfp, Cl_cmsw_status_report_msg_type *cl_cmsw) {

	cl_cmsw_log_endiannes_conversion(cl_cmsw);

	fprintf(outfp, "%u,", cl_cmsw->message_type);

	fprintf(outfp, "%u,", cl_cmsw->device_id.lru_id);
	fprintf(outfp, "%u,", cl_cmsw->device_id.slot_id);
	fprintf(outfp, "%u,", cl_cmsw->device_id.reserved);
	fprintf(outfp, "%u,", cl_cmsw->device_id.xmc_fpga);

	fprintf(outfp, "%u,", cl_cmsw->computer_status.chassis_manager_validity);
	fprintf(outfp, "%u,", cl_cmsw->computer_status.chassis_manager_mode);
	fprintf(outfp, "%u,", cl_cmsw->computer_status.temperature_status);
	fprintf(outfp, "%u,", cl_cmsw->computer_status.lrm_configuration_status);
	fprintf(outfp, "%u,", cl_cmsw->computer_status.reserved);
	fprintf(outfp, "%u,", cl_cmsw->computer_status.computer_config_mismatch_slot_list);

	for(int i = 0; i < PSM_COUNT_B0; ++i) {
		fprintf(outfp, "%u,", cl_cmsw->psm_status[i].slot_id);
		fprintf(outfp, "%u,", cl_cmsw->psm_status[i].ipmc_data_validity);
		fprintf(outfp, "%u,", cl_cmsw->psm_status[i].operation_mode);
		fprintf(outfp, "%u,", cl_cmsw->psm_status[i].module_status);
		fprintf(outfp, "%u,", cl_cmsw->psm_status[i].firmware_version.patch);
		fprintf(outfp, "%u,", cl_cmsw->psm_status[i].firmware_version.minor);
		fprintf(outfp, "%u,", cl_cmsw->psm_status[i].firmware_version.major);
		fprintf(outfp, "%u,", cl_cmsw->psm_status[i].hardware_type);
		fprintf(outfp, "%u,", cl_cmsw->psm_status[i].pcb_revision_number);
		fprintf(outfp, "%u,", cl_cmsw->psm_status[i].pcb_assembly_id);
		fprintf(outfp, "%u,", cl_cmsw->psm_status[i].mechanical_revision_number);
		fprintf(outfp, "%llu,", cl_cmsw->psm_status[i].serial_id);
		fprintf(outfp, "%u,", cl_cmsw->psm_status[i].reset_counter);
		fprintf(outfp, "%u,", cl_cmsw->psm_status[i].power_status);
	}

	for(int i = 0; i < HUM_COUNT_B0; ++i) {
		fprintf(outfp, "%u,", cl_cmsw->hum_status[i].slot_id);
		fprintf(outfp, "%u,", cl_cmsw->hum_status[i].ipmc_data_validity);
		fprintf(outfp, "%u,", cl_cmsw->hum_status[i].operation_mode);
		fprintf(outfp, "%u,", cl_cmsw->hum_status[i].module_status);
		fprintf(outfp, "%u,", cl_cmsw->hum_status[i].firmware_version.patch);
		fprintf(outfp, "%u,", cl_cmsw->hum_status[i].firmware_version.minor);
		fprintf(outfp, "%u,", cl_cmsw->hum_status[i].firmware_version.major);
		fprintf(outfp, "%u,", cl_cmsw->hum_status[i].hardware_type);
		fprintf(outfp, "%u,", cl_cmsw->hum_status[i].pcb_revision_number);
		fprintf(outfp, "%u,", cl_cmsw->hum_status[i].pcb_assembly_id);
		fprintf(outfp, "%u,", cl_cmsw->hum_status[i].mechanical_revision_number);
		fprintf(outfp, "%llu,", cl_cmsw->hum_status[i].serial_id);
		fprintf(outfp, "%u,", cl_cmsw->hum_status[i].reset_counter);
		fprintf(outfp, "%u,", cl_cmsw->hum_status[i].power_status);
	}

	fprintf(outfp, "%u,", cl_cmsw->smmm_status.slot_id);
	fprintf(outfp, "%u,", cl_cmsw->smmm_status.ipmc_data_validity);
	fprintf(outfp, "%u,", cl_cmsw->smmm_status.operation_mode);
	fprintf(outfp, "%u,", cl_cmsw->smmm_status.module_status);
	fprintf(outfp, "%u,", cl_cmsw->smmm_status.firmware_version.patch);
	fprintf(outfp, "%u,", cl_cmsw->smmm_status.firmware_version.minor);
	fprintf(outfp, "%u,", cl_cmsw->smmm_status.firmware_version.major);
	fprintf(outfp, "%u,", cl_cmsw->smmm_status.hardware_type);
	fprintf(outfp, "%u,", cl_cmsw->smmm_status.pcb_revision_number);
	fprintf(outfp, "%u,", cl_cmsw->smmm_status.pcb_assembly_id);
	fprintf(outfp, "%u,", cl_cmsw->smmm_status.mechanical_revision_number);
	fprintf(outfp, "%llu,", cl_cmsw->smmm_status.serial_id);
	fprintf(outfp, "%u,", cl_cmsw->smmm_status.reset_counter);
	fprintf(outfp, "%u,", cl_cmsw->smmm_status.power_status);

	fprintf(outfp, "%u,", cl_cmsw->iocm_status.lrm_status.slot_id);
	fprintf(outfp, "%u,", cl_cmsw->iocm_status.lrm_status.ipmc_data_validity);
	fprintf(outfp, "%u,", cl_cmsw->iocm_status.lrm_status.operation_mode);
	fprintf(outfp, "%u,", cl_cmsw->iocm_status.lrm_status.module_status);
	fprintf(outfp, "%u,", cl_cmsw->iocm_status.lrm_status.firmware_version.patch);
	fprintf(outfp, "%u,", cl_cmsw->iocm_status.lrm_status.firmware_version.minor);
	fprintf(outfp, "%u,", cl_cmsw->iocm_status.lrm_status.firmware_version.major);
	fprintf(outfp, "%u,", cl_cmsw->iocm_status.lrm_status.hardware_type);
	fprintf(outfp, "%u,", cl_cmsw->iocm_status.lrm_status.pcb_revision_number);
	fprintf(outfp, "%u,", cl_cmsw->iocm_status.lrm_status.pcb_assembly_id);
	fprintf(outfp, "%u,", cl_cmsw->iocm_status.lrm_status.mechanical_revision_number);
	fprintf(outfp, "%llu,", cl_cmsw->iocm_status.lrm_status.serial_id);
	fprintf(outfp, "%u,", cl_cmsw->iocm_status.lrm_status.reset_counter);
	fprintf(outfp, "%u,", cl_cmsw->iocm_status.lrm_status.power_status);
	fprintf(outfp, "%u,", cl_cmsw->iocm_status.dvi_status);
	fprintf(outfp, "%u,", cl_cmsw->iocm_status.voltage_data_status);
	fprintf(outfp, "%u,", cl_cmsw->iocm_status.temperature_data_status);
	fprintf(outfp, "%u,", cl_cmsw->iocm_status.reserved);

	for(int i = 0; i < DPM_COUNT_B0; ++i) {
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].lrm_status.slot_id);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].lrm_status.ipmc_data_validity);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].lrm_status.operation_mode);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].lrm_status.module_status);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].lrm_status.firmware_version.patch);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].lrm_status.firmware_version.minor);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].lrm_status.firmware_version.major);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].lrm_status.hardware_type);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].lrm_status.pcb_revision_number);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].lrm_status.pcb_assembly_id);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].lrm_status.mechanical_revision_number);
		fprintf(outfp, "%llu,", cl_cmsw->dpm_status[i].lrm_status.serial_id);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].lrm_status.reset_counter);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].lrm_status.power_status);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].a653_schedule_id);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].voltage_data_status);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].temperature_data_status);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].dtn_es_data_validity);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].ptp_sync_status);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].hsn_link_status);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].major_frame_alignment_status);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].dtn_es_redundancy_status);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].client_mount_validity);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].dtn_es_cfg_id);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].ptp_cfg_id);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].ptp_device_type);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].ptp_tod_network);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].dtn_es_fw_vers_major);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].dtn_es_fw_vers_minor);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].dtn_es_fw_vers_bugfix);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].monolith_id);
		fprintf(outfp, "%llu,", cl_cmsw->dpm_status[i].timestamp);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].operation_mode);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].reserved);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].ml_cmsw_msg_data_validity);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].t2080_dpm_bootloader_pbit.ddr_test);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].t2080_dpm_bootloader_pbit.ifc_nand_test);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].t2080_dpm_bootloader_pbit.serdes1_pll1_test);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].t2080_dpm_bootloader_pbit.serdes2_pll1_test);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].t2080_dpm_bootloader_pbit.serdes2_pll2_test);
		fprintf(outfp, "%u,", cl_cmsw->dpm_status[i].t2080_dpm_bootloader_pbit.reserved);
	}

	for(int i = 0; i < DSM_COUNT_B0; ++i) {
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].lrm_status.slot_id);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].lrm_status.ipmc_data_validity);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].lrm_status.operation_mode);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].lrm_status.module_status);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].lrm_status.firmware_version.patch);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].lrm_status.firmware_version.minor);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].lrm_status.firmware_version.major);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].lrm_status.hardware_type);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].lrm_status.pcb_revision_number);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].lrm_status.pcb_assembly_id);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].lrm_status.mechanical_revision_number);
		fprintf(outfp, "%llu,", cl_cmsw->dsm_status[i].lrm_status.serial_id);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].lrm_status.reset_counter);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].lrm_status.power_status);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].voltage_data_status);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].temperature_data_status);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].dtn_es_data_validity);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].dtn_sw_data_validity);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].ptp_sync_status);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].major_frame_alignment_status);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].dtn_es_redundancy_status);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].reserved);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].a653_schedule_id);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].dtn_es_cfg_id);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].dtn_sw_cfg_id);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].ptp_cfg_id);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].ptp_device_type);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].dtn_es_fw_vers_major);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].dtn_es_fw_vers_minor);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].dtn_es_fw_vers_bugfix);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].dtn_sw_port_link_status);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].dtn_sw_fw_vers_major);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].dtn_sw_fw_vers_minor);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].dtn_sw_fw_vers_bugfix);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].t2080_dsm_bootloader_pbit.ddr_test);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].t2080_dsm_bootloader_pbit.ifc_nand_test);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].t2080_dsm_bootloader_pbit.serdes1_pll1_test);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].t2080_dsm_bootloader_pbit.serdes1_pll2_test);
		fprintf(outfp, "%u,", cl_cmsw->dsm_status[i].t2080_dsm_bootloader_pbit.reserved);
	}

	for(int i = 0; i < GPM_COUNT_B0; ++i) {
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].lrm_status.slot_id);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].lrm_status.ipmc_data_validity);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].lrm_status.operation_mode);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].lrm_status.module_status);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].lrm_status.firmware_version.patch);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].lrm_status.firmware_version.minor);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].lrm_status.firmware_version.major);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].lrm_status.hardware_type);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].lrm_status.pcb_revision_number);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].lrm_status.pcb_assembly_id);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].lrm_status.mechanical_revision_number);
		fprintf(outfp, "%llu,", cl_cmsw->gpm_status[i].lrm_status.serial_id);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].lrm_status.reset_counter);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].lrm_status.power_status);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].advb_tx_1_status);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].advb_tx_2_status);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].advb_tx_3_status);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].advb_hm_data_status);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].advb_status);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].dvi_status);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].voltage_data_status);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].temperature_data_status);
		fprintf(outfp, "%u,", cl_cmsw->gpm_status[i].reserved);
	}

	for(int i = 0; i < HSM_COUNT_B0; ++i) {
		fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].lrm_status.slot_id);
		fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].lrm_status.ipmc_data_validity);
		fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].lrm_status.operation_mode);
		fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].lrm_status.module_status);
		fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].lrm_status.firmware_version.patch);
		fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].lrm_status.firmware_version.minor);
		fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].lrm_status.firmware_version.major);
		fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].lrm_status.hardware_type);
		fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].lrm_status.pcb_revision_number);
		fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].lrm_status.pcb_assembly_id);
		fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].lrm_status.mechanical_revision_number);
		fprintf(outfp, "%llu,", cl_cmsw->hsm_status[i].lrm_status.serial_id);
		fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].lrm_status.reset_counter);
		fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].lrm_status.power_status);
		fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].hsm_data_validity);
		fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].dsm_hsm_pcie_link_status);
		fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].temperature_data_status);
		fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].voltage_data_status);
		fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].current_data_status);
		for(int j = 0; j < 8; ++j) {
			fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].backplane_hsn_port_status_list[j].port_number);
			fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].backplane_hsn_port_status_list[j].data_validity);
			fprintf(outfp, "%u,", cl_cmsw->hsm_status[i].backplane_hsn_port_status_list[j].hsn_link_status);
			fprintf(outfp, "%u%s", cl_cmsw->hsm_status[i].backplane_hsn_port_status_list[j].reserved, ((j == 7) && (i == (HSM_COUNT_B0 - 1))) ? "\n" : ",");
		}
	}
}

static int clcmsw_detections_parser(FILE *outfp, CLCMSW_LOG_FRAME_TYPE **frame)
{
    CLCMSW_LOG_FRAME_TYPE *log_frame = *frame;
    uint64_t seconds;
    uint64_t nanoseconds;
    log_frame->log_frame_header.log_timestamp = ntohll(log_frame->log_frame_header.log_timestamp);
    log_frame->log_frame_header.log_entry_count = ntohl(log_frame->log_frame_header.log_entry_count);
    log_frame->log_frame_header.log_drop_count = ntohl(log_frame->log_frame_header.log_drop_count);

    for(int i = 0; i < log_frame->log_frame_header.log_entry_count; i++)
    {
        log_frame->log_entry_list[i].log_type     = ntohl(log_frame->log_entry_list[i].log_type);
        log_frame->log_entry_list[i].first_param  = ntohll(log_frame->log_entry_list[i].first_param);
        log_frame->log_entry_list[i].second_param = ntohll(log_frame->log_entry_list[i].second_param);
    }

    if(0 == log_frame->log_frame_header.log_entry_count)
    {
        seconds = log_frame->log_frame_header.log_timestamp / 1000000000;
        nanoseconds = log_frame->log_frame_header.log_timestamp % 1000000000;
        fprintf(outfp, "%llu sec,%09llu nsec,", seconds, nanoseconds);
        fprintf(outfp, "%u,", log_frame->log_frame_header.log_entry_count);
        fprintf(outfp, "%u,", log_frame->log_frame_header.log_drop_count);

        fprintf(outfp, "N/A,", "");
        fprintf(outfp, "N/A,", "");
        fprintf(outfp, "N/A,", "");
        fprintf(outfp, "N/A\n", "");
    }
    else
    {
        for(int i = 0; i < log_frame->log_frame_header.log_entry_count; i++)
        {
            seconds = log_frame->log_frame_header.log_timestamp / 1000000000;
            nanoseconds = log_frame->log_frame_header.log_timestamp % 1000000000;
            fprintf(outfp, "%llu sec,%09llu nsec,", seconds, nanoseconds);
            fprintf(outfp, "%u,", log_frame->log_frame_header.log_entry_count);
            fprintf(outfp, "%u,", log_frame->log_frame_header.log_drop_count);


            fprintf(outfp, "%d-%s,", log_frame->log_entry_list[i].log_type, log_frame->log_entry_list[i].log_type >= CLCMSW_LOGTYPE_MAX ? CLCMSW_LOG_TYPE_strings[CLCMSW_LOGTYPE_MAX] : CLCMSW_LOG_TYPE_strings[log_frame->log_entry_list[i].log_type]);
            fprintf(outfp, "%s,", log_frame->log_entry_list[i].log_severity >= CMSW_SEVERITY_MAX ? severity_strings[CMSW_SEVERITY_MAX] : severity_strings[log_frame->log_entry_list[i].log_severity]);
            fprintf(outfp, "%lld,", log_frame->log_entry_list[i].first_param);
            fprintf(outfp, "%lld\n", log_frame->log_entry_list[i].second_param);
        }
    }

    return 0;
}


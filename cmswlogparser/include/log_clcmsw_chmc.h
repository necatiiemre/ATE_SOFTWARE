#pragma once
#include <stdint.h>

#define PCS_CMC_SLOT_COUNT 17

typedef enum pcs_slot_id_type
{
    PCS_NO_SLOT = -1,
    PCS_SLOT_1  = 1,
    PCS_SLOT_2  = 2,
    PCS_SLOT_3  = 3,
    PCS_SLOT_4  = 4,
    PCS_SLOT_5  = 5,
    PCS_SLOT_6  = 6,
    PCS_SLOT_7  = 7,
    PCS_SLOT_8  = 8,
    PCS_SLOT_9  = 9,
    PCS_SLOT_10 = 10,
    PCS_SLOT_11 = 11,
    PCS_SLOT_12 = 12,
    PCS_SLOT_13 = 13,
    PCS_SLOT_14 = 14,
    PCS_SLOT_15 = 15,
    PCS_SLOT_16 = 16,
    PCS_SLOT_17 = 17,
    PCS_SLOT_MAX_COUNT
} pcs_slot_id_type;

typedef enum pcs_LRM_dev_type
{
    DSM           = 1,
    DPM           = 2,
    HSM           = 3,
    PSM           = 4,
    HUM           = 5,
    GPM           = 6,
    IO            = 7,
    SMMM          = 8,
    HPM           = 9,
    SPM           = 10,
    GPGPM         = 11,
    TIMEOUT       = 14,
    NOT_REQUESTED = 15
} pcs_LRM_dev_type;

typedef struct __attribute__((packed)) pcs_LRM_dev_arr_type
{
    pcs_LRM_dev_type LRM_dev;
    unsigned char    pcs_LRM_serial_ID[8];
    unsigned char    pcs_LRM_bilgem_ID[9];
} pcs_LRM_dev_arr_type;

typedef enum pcs_chassis_man_mode_type
{
    STANDBY = 1,
    ACTIVE  = 2,
    UNKNOWN = 3
} pcs_chassis_man_mode_type;

typedef enum Pcs_parity_type
{
    PARITY_OKAY  = 0, /*!< Parity check passed successfully. */
    PARITY_ERROR = 1  /*!< Parity error detected. */
} Pcs_parity_type;

typedef struct __attribute__((packed)) pcs_chassis_man_record_table_type
{
    pcs_chassis_man_mode_type chassis_manager_mode;
    uint8_t                   LRU_id;
    Pcs_parity_type           LRU_parity;
    pcs_slot_id_type          slot_id;
    uint8_t                   sequence_number;
    pcs_LRM_dev_arr_type      LRMs_array[PCS_CMC_SLOT_COUNT];
} pcs_chassis_man_record_table_type;

static int chmc_log_print_header(FILE **fp) {
    fprintf(*fp,  "DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP,LOG PAYLOAD LENGTH,");
    fprintf(*fp,  "chassis_manager_mode,");
    fprintf(*fp,  "LRU_id,");
    fprintf(*fp,  "LRU_parity,");
    fprintf(*fp,  "slot_id,");
    fprintf(*fp,  "sequence_number,");

    // Print each LRM's data
    for (int i = 0; i < (PCS_CMC_SLOT_COUNT); i++) {
        fprintf(*fp, "%s[%d],", "LRM_dev",i);
        fprintf(*fp, "%s[%d],", "pcs_LRM_serial_ID",i);
        if(i == PCS_CMC_SLOT_COUNT - 1)
        {
            fprintf(*fp, "%s[%d]\n", "pcs_LRM_bilgem_ID",i);
        }
        else
        {
            fprintf(*fp, "%s[%d],", "pcs_LRM_bilgem_ID",i);
        }
    }
}

static int chmc_log_parser(FILE *outfp, pcs_chassis_man_record_table_type *chmc) {
    chmc->chassis_manager_mode = ntohl(chmc->chassis_manager_mode);
    chmc->LRU_parity = ntohl(chmc->LRU_parity);
    chmc->slot_id = ntohl(chmc->slot_id);

    // Process each LRM in the array
    for (int i = 0; i < PCS_CMC_SLOT_COUNT; i++) {
        chmc->LRMs_array[i].LRM_dev = ntohl(chmc->LRMs_array[i].LRM_dev);
    }

    fprintf(outfp, "%u,", chmc->chassis_manager_mode);
    fprintf(outfp, "%u,", chmc->LRU_id);
    fprintf(outfp, "%u,", chmc->LRU_parity);
    fprintf(outfp, "%u,", chmc->slot_id);
    fprintf(outfp, "%u,", chmc->sequence_number);

    // Print each LRM's data
    for (int i = 0; i < PCS_CMC_SLOT_COUNT; i++) {
        fprintf(outfp, "%u,", chmc->LRMs_array[i].LRM_dev);
        fprintf(outfp, "%u,", chmc->LRMs_array[i].pcs_LRM_serial_ID);
        if (i == PCS_CMC_SLOT_COUNT -1)
        {

            fprintf(outfp, "%u\n", chmc->LRMs_array[i].pcs_LRM_bilgem_ID);
        }
        else
        {
            fprintf(outfp, "%u,", chmc->LRMs_array[i].pcs_LRM_bilgem_ID);

        }
    }

    return 0;
}
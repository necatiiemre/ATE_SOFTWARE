#pragma once
#include <stdint.h>

#define MMPLCMSW_MAX_NB_OF_ADCU 2
#define MAX_FAULT_ENTRY_COUNT 255
typedef uint16_t Cmsw_device_id_type;
typedef uint8_t  Cmsw_configuration_id_type;

typedef enum
{
    CMSW_PL_INIT            = 0, /*Default state*/
    CMSW_PL_OPERATIONAL     = 1, /**/
    CMSW_PL_RECONFIGURATION = 2,
} Cmsw_pl_state_type; // 4 bytes

typedef enum
{
    CMSW_MASTER           = 0,
    CMSW_BACKUP           = 1,
    CMSW_NONE             = 4,
    CMSW_AUTH_LVL_MAX      = 5,
} Cmsw_module_auth_level_type_enum; // 4 bytes

typedef enum
{
    CMSW_HEARTBEAT_MISS              = 0,
    CMSW_LRU_HOT                     = 1,
    CMSW_DTN_ES_CFG_MISMATCH         = 2,
    CMSW_DTN_SW_CFG_MISMATCH         = 3,
    CMSW_SCHEDULE_ID_MISMATCH        = 4,
    CMSW_DCU_MAJOR_FRAME_NOT_SYNCHED = 5,
    CMSW_DCU_M1553_CH_0x110_WATCHDOG_FAULT     = 6,
    CMSW_DCU_M1553_CH_0x111_WATCHDOG_FAULT     = 7,
    CMSW_FAULT_MAX = 8,
    CMSW_INVALID_FAULT = 255
} Cmsw_event_type_enum; // 4 bytes

typedef struct
{
    Cmsw_device_id_type geo_id;
    uint8_t             fault_priority;
    uint8_t             max_event_handle_count;
    Cmsw_event_type_enum     fault_type;
} Cmsw_pl_fault_identifier_type; // 8 bytes

typedef struct __attribute__((packed))
{
    uint8_t adcu_lru_id;
    uint8_t is_ch_0x110_watchdog_enabled :4;
    uint8_t is_ch_0x111_watchdog_enabled :4;
} Mmplcmsw_adcu_watchdog_settings_type; // 2 bytes

/**
 * @category internal_msg
 * @desc Represents the MMP Status Message interface structure. The MMP Status Message is an internal message used to synchronize mmpl-cmsw instances with each other.
 */
typedef struct Cmsw_pl_program_state_type
{
    /**
     * @desc Represents Module Authentication Level
     * @min See Cmsw_module_auth_level_type_enum
     * @max See Cmsw_module_auth_level_type_enum
     * @constraint CMSW_MASTER: 0,CMSW_BACKUP: 1
     */
    Cmsw_module_auth_level_type_enum auth_level;

    /**
     * @desc Represents configuration PDI ID
     * @min 0
     * @max 4294967295
     */
    uint32_t cfg_pdi_id;

    /**
     * @desc Indicates the authorization state of the mmpl-cmsw instance.
     * @min 0
     * @max 1
     * @constraint Active: 1, Standby: 0
     */
    uint8_t is_active;

    /**
     * @desc Represents the last synchronized major frame time.
     * @min 0
     * @max 18446744073709551615
     */
    uint64_t world_start_time;

    /**
     * @desc Represents the MMP state.
     * @min See Cmsw_pl_state_type enum
     * @max See Cmsw_pl_state_type enum
     * @constraint CMSW_PL_INIT: 0, CMSW_PL_OPERATIONAL: 1, CMSW_PL_RECONFIGURATION: 2
     */
    Cmsw_pl_state_type mmp_state;

    /**
     * @desc Represents the Platform Configuration ID.
     * @min 0
     * @max 255
     */
    Cmsw_configuration_id_type platform_cfg_id;

    /**
     * @desc Represents Policy Index
     * @min 0
     * @max 32767
     */
    int16_t policy_index;

    /**
     * @desc Represents Policy Step Index
     * @min 0
     * @max 32767
     */
    int16_t policy_step_index;

    /**
     * @desc Represents Policy Step Sequence Number
     * @min 0
     * @max 32767
     */
    int16_t policy_step_sequence_no;

    /**
     * @desc Represents Policy Step Timeout Time
     * @min 0
     * @max 9223372036854775807
     */
    int64_t policy_step_timeout_time;

    /**
     * @desc Represents the Set TOD with SMMM time.
     * @min 0
     * @max 18446744073709551615
     */
    uint64_t set_tod_w_smmm_time;

    /**
     * @desc Represents Policy Step Retry Count
     * @min 0
     * @max 255
     */
    uint8_t policy_step_retry_count;

    /**
     * @desc Active Fault Identifier
     * @min N/A
     * @max N/A
     */
    Cmsw_pl_fault_identifier_type active_fault_identifier;

    /**
     * @desc New Active MSVR ID
     * @min 0
     * @max 65535
     */
    Cmsw_device_id_type new_active_msvr_id;

    /**
     * @desc Represents Event Management Handle Budget
     * @min 0
     * @max 256, See(MAX_FAULT_ENTRY_COUNT + 1)
     */
    uint8_t event_management_handle_budget[MAX_FAULT_ENTRY_COUNT + 1];

    /**
     * @desc Represents Active Fault Table Index
     * @min 0
     * @max 32767
     */
    int16_t active_fault_table_ind;

    /**
     * @desc Represents the ADCU watchdog settings for each ADCU on the platform.
     * @min N/A
     * @max N/A
     */
    Mmplcmsw_adcu_watchdog_settings_type adcu_watchdog_settings[MMPLCMSW_MAX_NB_OF_ADCU];
} Cmsw_pl_program_state_type;

static void mmpl_state_header_print(FILE **fp)
{
    fprintf(*fp, "DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP,LOG PAYLOAD LENGTH,");
    fprintf(*fp, "%s,", "auth_level");
    fprintf(*fp, "%s,", "cfg_pdi_id");
    fprintf(*fp, "%s,", "is_active");
    fprintf(*fp, "%s,", "world_start_time");
    fprintf(*fp, "%s,", "mmp_state");
    fprintf(*fp, "%s,", "platform_cfg_id");

    fprintf(*fp, "%s,", "policy_index");
    fprintf(*fp, "%s,", "policy_step_index");
    fprintf(*fp, "%s,", "policy_step_sequence_no");
    fprintf(*fp, "%s,", "policy_step_timeout_time");
    fprintf(*fp, "%s,", "set_tod_w_smmm_time");
    fprintf(*fp, "%s,", "policy_step_retry_count");

    fprintf(*fp, "%s,", "active_fault_identifier_geo_id");
    fprintf(*fp, "%s,", "active_fault_identifier_fault_priority");
    fprintf(*fp, "%s,", "active_fault_identifier_max_event_handle_count");
    fprintf(*fp, "%s,", "active_fault_identifier_fault_type");
    fprintf(*fp, "%s,", "new_active_msvr_id");

    for(int i=0; i<MAX_FAULT_ENTRY_COUNT + 1; i++) {
        fprintf(*fp, "%s[%d],", "event_management_handle_budget", i);
    }

    fprintf(*fp, "%s,", "active_fault_table_ind");

    for(int i=0; i<MMPLCMSW_MAX_NB_OF_ADCU; i++) {
        fprintf(*fp, "%s[%d],", "adcu_watchdog_settings_adcu_lru_id", i);
        fprintf(*fp, "%s[%d],", "adcu_watchdog_settings_is_ch_0x110_watchdog_enabled", i);
        if(i==MMPLCMSW_MAX_NB_OF_ADCU-1) {
            fprintf(*fp, "%s[%d]\n", "adcu_watchdog_settings_is_ch_0x111_watchdog_enabled", i);
        } else {
            fprintf(*fp, "%s[%d],", "adcu_watchdog_settings_is_ch_0x111_watchdog_enabled", i);
        }
    }
}

static int mmpl_state_log_parser(FILE *outfp, Cmsw_pl_program_state_type *pl_state) 
{
    pl_state->auth_level = ntohl(pl_state->auth_level);
    pl_state->cfg_pdi_id = ntohl(pl_state->cfg_pdi_id);
    pl_state->is_active = (pl_state->is_active);
    pl_state->world_start_time = ntohll(pl_state->world_start_time);
    pl_state->mmp_state = ntohl(pl_state->mmp_state);
    pl_state->platform_cfg_id = (pl_state->platform_cfg_id);
    
    pl_state->policy_index = ntohs(pl_state->policy_index);

    pl_state->policy_step_index = ntohs(pl_state->policy_step_index);

    pl_state->policy_step_sequence_no = ntohs(pl_state->policy_step_sequence_no);
    pl_state->policy_step_timeout_time = ntohll(pl_state->policy_step_timeout_time);
    pl_state->set_tod_w_smmm_time = ntohll(pl_state->set_tod_w_smmm_time);
    pl_state->policy_step_retry_count = (pl_state->policy_step_retry_count);

    pl_state->active_fault_identifier.geo_id = ntohs(pl_state->active_fault_identifier.geo_id);
    pl_state->active_fault_identifier.fault_priority = (pl_state->active_fault_identifier.fault_priority);
    pl_state->active_fault_identifier.max_event_handle_count = (pl_state->active_fault_identifier.max_event_handle_count);
    pl_state->active_fault_identifier.fault_type = ntohl(pl_state->active_fault_identifier.fault_type);
    pl_state->new_active_msvr_id = ntohs(pl_state->new_active_msvr_id);

    for(int i=0; i<MAX_FAULT_ENTRY_COUNT + 1; i++) {
        pl_state->event_management_handle_budget[i] = (pl_state->event_management_handle_budget[i]);
    }
    pl_state->active_fault_table_ind = ntohs(pl_state->active_fault_table_ind);

    for(int i=0; i < MMPLCMSW_MAX_NB_OF_ADCU; i++) 
    {
        // 1. Get the corrupted data as a raw 16-bit integer
        uint16_t *corrupted_ptr = (uint16_t*)&pl_state->adcu_watchdog_settings[i];
        uint16_t corrected_val = (*corrupted_ptr << 8) | (*corrupted_ptr >> 8);

        // 2. Put the corrected bits back into the struct
        memcpy(&pl_state->adcu_watchdog_settings[i], &corrected_val, 2);
    }

    fprintf(outfp, "%u,", pl_state->auth_level);
    fprintf(outfp, "%u,", pl_state->cfg_pdi_id);
    fprintf(outfp, "%u,", pl_state->is_active);
    fprintf(outfp, "%llu,", pl_state->world_start_time);
    fprintf(outfp, "%u,", pl_state->mmp_state);
    fprintf(outfp, "%u,", pl_state->platform_cfg_id);

    fprintf(outfp, "%u,", pl_state->policy_index);
    fprintf(outfp, "%u,", pl_state->policy_step_index);
    fprintf(outfp, "%u,", pl_state->policy_step_sequence_no);
    fprintf(outfp, "%llu,", pl_state->policy_step_timeout_time);
    fprintf(outfp, "%llu,", pl_state->set_tod_w_smmm_time);
    fprintf(outfp, "%u,", pl_state->policy_step_retry_count);

    fprintf(outfp, "%u,", pl_state->active_fault_identifier.geo_id);
    fprintf(outfp, "%u,", pl_state->active_fault_identifier.fault_priority);
    fprintf(outfp, "%u,", pl_state->active_fault_identifier.max_event_handle_count);
    fprintf(outfp, "%u,", pl_state->active_fault_identifier.fault_type);
    fprintf(outfp, "%u,", pl_state->new_active_msvr_id);

    for(int i=0; i<MAX_FAULT_ENTRY_COUNT + 1; i++) {
        fprintf(outfp, "%u,", pl_state->event_management_handle_budget[i]);
    }

    fprintf(outfp, "%u,", pl_state->active_fault_table_ind);

    for(int i=0; i<MMPLCMSW_MAX_NB_OF_ADCU; i++) {
        fprintf(outfp, "%u,", pl_state->adcu_watchdog_settings[i].adcu_lru_id);
        fprintf(outfp, "%u,", pl_state->adcu_watchdog_settings[i].is_ch_0x110_watchdog_enabled);
        if(i==MMPLCMSW_MAX_NB_OF_ADCU-1) {
            fprintf(outfp, "%u\n", pl_state->adcu_watchdog_settings[i].is_ch_0x111_watchdog_enabled);
        } else {
            fprintf(outfp, "%u,", pl_state->adcu_watchdog_settings[i].is_ch_0x111_watchdog_enabled);
        }
    }

    return 0;
}
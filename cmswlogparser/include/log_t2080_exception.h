#pragma once

#include <stdio.h>
#include <stdint.h>

typedef struct
{
    uint32_t second;   

    uint32_t nanosecond;  

} Pcs_timespec_gt;


typedef struct
{
    uint32_t total_cs_size;  

    uint32_t kernel_cs_size;
      
    uint64_t call_stack_return_addresses[24];

} Pcs_call_stack_info_gt;

typedef struct
{

    uint64_t program_counter;
    
    uint64_t stack_pointer;
    
    uint64_t frame_pointer;
    
    uint64_t return_address;

    uint64_t cpu_state_register;

    uint64_t fault_address;
    
    uint32_t is_fault_addr_valid;
    
    uint32_t core_id;
    
    uint16_t error_id;
    
    uint16_t state;
    
    uint16_t exception_type;

    uint16_t nested_exception_count;

    uint32_t is_exception;

    uint32_t is_kernel;

} Pcs_exception_info_hw_gt;

typedef struct
{
	
    Pcs_exception_info_hw_gt excp_hw;
    
    char message[128];

    uint64_t message_length;
    
    Pcs_call_stack_info_gt call_stack_info;
    
    int32_t task_id;
    
    int32_t partition_id;
    
    char task_name[32];
    
    char partition_name[32];
    
    Pcs_timespec_gt timespec;
    
    int64_t entrypoint;
     
} Pcs_exception_info_gt;

typedef struct
{

    uint64_t nb_of_unread_logs;      
    
    uint64_t nb_of_overwritten_logs;  

    uint64_t nb_of_total_hm_logs;

    uint64_t nb_of_missed_major_frames;

} Pcs_hm_stats_info;

typedef struct
{

    Pcs_hm_stats_info hm_info;

    Pcs_exception_info_gt hm_log;

} Cl_cmsw_exception_data_type;


void strip_non_alphanumerical(char *data) {
    int len = strlen(data);
    for(int i = 0; i < len; i++) {
        if(data[i] == '\n' || data[i] == '\r') {
            data[i] = '\0';
        }
    }
}


static inline void log_cl_cmsw_rtos_exception_major_sync_error_endian_convert(Pcs_hm_stats_info* data) {
    // BEGIN Pcs_hm_stats_info
    data->nb_of_unread_logs = ntohll(data->nb_of_unread_logs);
    data->nb_of_overwritten_logs = ntohll(data->nb_of_overwritten_logs);
    data->nb_of_total_hm_logs = ntohll(data->nb_of_total_hm_logs);
    data->nb_of_missed_major_frames = ntohll(data->nb_of_missed_major_frames);
    // END Pcs_hm_stats_info
}

static inline void log_cl_cmsw_rtos_exception_major_sync_error_header_print(FILE *fp) {
    fprintf(fp, "DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP,LOG PAYLOAD LENGTH,");
    // BEGIN Pcs_hm_stats_info
    fprintf(fp, "nb_of_unread_logs,");
    fprintf(fp, "nb_of_overwritten_logs,");
    fprintf(fp, "nb_of_total_hm_logs,");
    fprintf(fp, "nb_of_missed_major_frames\n");
    // END Pcs_hm_stats_info
}

static inline void log_cl_cmsw_rtos_exception_major_sync_error_data_print(FILE *fp, Pcs_hm_stats_info* data) {
    log_cl_cmsw_rtos_exception_major_sync_error_endian_convert(data);
    // BEGIN Pcs_hm_stats_info
    fprintf(fp, "%llu,", data->nb_of_unread_logs);
    fprintf(fp, "%llu,", data->nb_of_overwritten_logs);
    fprintf(fp, "%llu,", data->nb_of_total_hm_logs);
    fprintf(fp, "%llu\n", data->nb_of_missed_major_frames);
    // END Pcs_hm_stats_info
}


static inline void log_cl_cmsw_rtos_exception_status_endian_convert(Cl_cmsw_exception_data_type* data) {
    // BEGIN Cl_cmsw_exception_data_type
    // BEGIN Pcs_hm_stats_info
    data->hm_info.nb_of_unread_logs = ntohll(data->hm_info.nb_of_unread_logs);
    data->hm_info.nb_of_overwritten_logs = ntohll(data->hm_info.nb_of_overwritten_logs);
    data->hm_info.nb_of_total_hm_logs = ntohll(data->hm_info.nb_of_total_hm_logs);
    data->hm_info.nb_of_missed_major_frames = ntohll(data->hm_info.nb_of_missed_major_frames);
    // END Pcs_hm_stats_info
    // BEGIN Pcs_exception_info_gt
    // BEGIN Pcs_exception_info_hw_gt
    data->hm_log.excp_hw.program_counter = ntohll(data->hm_log.excp_hw.program_counter);
    data->hm_log.excp_hw.stack_pointer = ntohll(data->hm_log.excp_hw.stack_pointer);
    data->hm_log.excp_hw.frame_pointer = ntohll(data->hm_log.excp_hw.frame_pointer);
    data->hm_log.excp_hw.return_address = ntohll(data->hm_log.excp_hw.return_address);
    data->hm_log.excp_hw.cpu_state_register = ntohll(data->hm_log.excp_hw.cpu_state_register);
    data->hm_log.excp_hw.fault_address = ntohll(data->hm_log.excp_hw.fault_address);
    data->hm_log.excp_hw.is_fault_addr_valid = ntohl(data->hm_log.excp_hw.is_fault_addr_valid);
    data->hm_log.excp_hw.core_id = ntohl(data->hm_log.excp_hw.core_id);
    data->hm_log.excp_hw.error_id = ntohs(data->hm_log.excp_hw.error_id);
    data->hm_log.excp_hw.state = ntohs(data->hm_log.excp_hw.state);
    data->hm_log.excp_hw.exception_type = ntohs(data->hm_log.excp_hw.exception_type);
    data->hm_log.excp_hw.nested_exception_count = ntohs(data->hm_log.excp_hw.nested_exception_count);
    data->hm_log.excp_hw.is_exception = ntohl(data->hm_log.excp_hw.is_exception);
    data->hm_log.excp_hw.is_kernel = ntohl(data->hm_log.excp_hw.is_kernel);
    // END Pcs_exception_info_hw_gt
    // for (int i = 0; i < 128; i++) {
    // }
    data->hm_log.message_length = ntohll(data->hm_log.message_length);
    // BEGIN Pcs_call_stack_info_gt
    data->hm_log.call_stack_info.total_cs_size = ntohl(data->hm_log.call_stack_info.total_cs_size);
    data->hm_log.call_stack_info.kernel_cs_size = ntohl(data->hm_log.call_stack_info.kernel_cs_size);
    for (int j = 0; j < 24; j++) {
        data->hm_log.call_stack_info.call_stack_return_addresses[j] = ntohll(data->hm_log.call_stack_info.call_stack_return_addresses[j]);
    }
    // END Pcs_call_stack_info_gt
    data->hm_log.task_id = ntohl(data->hm_log.task_id);
    data->hm_log.partition_id = ntohl(data->hm_log.partition_id);
    // for (int k = 0; k < 32; k++) {
    // }
    // for (int l = 0; l < 32; l++) {
    // }
    // BEGIN Pcs_timespec_gt
    data->hm_log.timespec.second = ntohl(data->hm_log.timespec.second);
    data->hm_log.timespec.nanosecond = ntohl(data->hm_log.timespec.nanosecond);
    // END Pcs_timespec_gt
    data->hm_log.entrypoint = ntohll(data->hm_log.entrypoint);
    // END Pcs_exception_info_gt
    // END Cl_cmsw_exception_data_type
}

static inline void log_cl_cmsw_rtos_exception_status_header_print(FILE *fp) {
    fprintf(fp, "DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP,LOG PAYLOAD LENGTH,");
    // BEGIN Cl_cmsw_exception_data_type
    // BEGIN Pcs_hm_stats_info
    fprintf(fp, "hm_info.nb_of_unread_logs,");
    fprintf(fp, "hm_info.nb_of_overwritten_logs,");
    fprintf(fp, "hm_info.nb_of_total_hm_logs,");
    fprintf(fp, "hm_info.nb_of_missed_major_frames,");
    // END Pcs_hm_stats_info
    // BEGIN Pcs_exception_info_gt
    // BEGIN Pcs_exception_info_hw_gt
    fprintf(fp, "hm_log.excp_hw.program_counter,");
    fprintf(fp, "hm_log.excp_hw.stack_pointer,");
    fprintf(fp, "hm_log.excp_hw.frame_pointer,");
    fprintf(fp, "hm_log.excp_hw.return_address,");
    fprintf(fp, "hm_log.excp_hw.cpu_state_register,");
    fprintf(fp, "hm_log.excp_hw.fault_address,");
    fprintf(fp, "hm_log.excp_hw.is_fault_addr_valid,");
    fprintf(fp, "hm_log.excp_hw.core_id,");
    fprintf(fp, "hm_log.excp_hw.error_id,");
    fprintf(fp, "hm_log.excp_hw.state,");
    fprintf(fp, "hm_log.excp_hw.exception_type,");
    fprintf(fp, "hm_log.excp_hw.nested_exception_count,");
    fprintf(fp, "hm_log.excp_hw.is_exception,");
    fprintf(fp, "hm_log.excp_hw.is_kernel,");
    // END Pcs_exception_info_hw_gt
    fprintf(fp, "%s,", "hm_log.message");
    // for (int i = 0; i < 128; i++) {
    //     fprintf(fp, "hm_log.message[%d],", i);
    // }
    fprintf(fp, "hm_log.message_length,");
    // BEGIN Pcs_call_stack_info_gt
    fprintf(fp, "hm_log.call_stack_info.total_cs_size,");
    fprintf(fp, "hm_log.call_stack_info.kernel_cs_size,");
    for (int j = 0; j < 24; j++) {
        fprintf(fp, "hm_log.call_stack_info.call_stack_return_addresses[%d],", j);
    }
    // END Pcs_call_stack_info_gt
    fprintf(fp, "hm_log.task_id,");
    fprintf(fp, "hm_log.partition_id,");
    fprintf(fp, "%s,", "hm_log.task_name");
    // for (int k = 0; k < 32; k++) {
    //     fprintf(fp, "hm_log.task_name[%d],", k);
    // }
    fprintf(fp, "%s,", "hm_log.partition_name");
    // for (int l = 0; l < 32; l++) { 
    //     fprintf(fp, "hm_log.partition_name[%d],", l);
    // }
    // BEGIN Pcs_timespec_gt
    fprintf(fp, "hm_log.timespec.second,");
    fprintf(fp, "hm_log.timespec.nanosecond,");
    // END Pcs_timespec_gt
    fprintf(fp, "hm_log.entrypoint\n");
    // END Pcs_exception_info_gt
    // END Cl_cmsw_exception_data_type
}

static inline void log_cl_cmsw_rtos_exception_status_data_print(FILE *fp, Cl_cmsw_exception_data_type* data) {

    log_cl_cmsw_rtos_exception_status_endian_convert(data);

    strip_non_alphanumerical(data->hm_log.message);
    strip_non_alphanumerical(data->hm_log.task_name);
    strip_non_alphanumerical(data->hm_log.partition_name);

    // BEGIN Cl_cmsw_exception_data_type
    // BEGIN Pcs_hm_stats_info
    fprintf(fp, "%llu,", data->hm_info.nb_of_unread_logs);
    fprintf(fp, "%llu,", data->hm_info.nb_of_overwritten_logs);
    fprintf(fp, "%llu,", data->hm_info.nb_of_total_hm_logs);
    fprintf(fp, "%llu,", data->hm_info.nb_of_missed_major_frames);
    // END Pcs_hm_stats_info
    // BEGIN Pcs_exception_info_gt
    // BEGIN Pcs_exception_info_hw_gt
    fprintf(fp, "%llu,", data->hm_log.excp_hw.program_counter);
    fprintf(fp, "%llu,", data->hm_log.excp_hw.stack_pointer);
    fprintf(fp, "%llu,", data->hm_log.excp_hw.frame_pointer);
    fprintf(fp, "%llu,", data->hm_log.excp_hw.return_address);
    fprintf(fp, "%llu,", data->hm_log.excp_hw.cpu_state_register);
    fprintf(fp, "%llu,", data->hm_log.excp_hw.fault_address);
    fprintf(fp, "%u,", data->hm_log.excp_hw.is_fault_addr_valid);
    fprintf(fp, "%u,", data->hm_log.excp_hw.core_id);
    fprintf(fp, "%u,", data->hm_log.excp_hw.error_id);
    fprintf(fp, "%u,", data->hm_log.excp_hw.state);
    fprintf(fp, "%u,", data->hm_log.excp_hw.exception_type);
    fprintf(fp, "%u,", data->hm_log.excp_hw.nested_exception_count);
    fprintf(fp, "%u,", data->hm_log.excp_hw.is_exception);
    fprintf(fp, "%u,", data->hm_log.excp_hw.is_kernel);
    // END Pcs_exception_info_hw_gt
    fprintf(fp, "\"%s\",", data->hm_log.message); // Wrap message in quotes to prevent CSV format breakage from commas
    // for (int i = 0; i < 128; i++) {
    //     fprintf(fp, "%u,", (unsigned int)data->hm_log.message[i]);
    // }
    fprintf(fp, "%llu,", data->hm_log.message_length);
    // BEGIN Pcs_call_stack_info_gt
    fprintf(fp, "%u,", data->hm_log.call_stack_info.total_cs_size);
    fprintf(fp, "%u,", data->hm_log.call_stack_info.kernel_cs_size);
    for (int j = 0; j < 24; j++) {
        fprintf(fp, "%llu,", data->hm_log.call_stack_info.call_stack_return_addresses[j]);
    }
    // END Pcs_call_stack_info_gt
    fprintf(fp, "%d,", data->hm_log.task_id);
    fprintf(fp, "%d,", data->hm_log.partition_id);
    fprintf(fp, "%s,", data->hm_log.task_name);
    // for (int k = 0; k < 32; k++) {
    //     fprintf(fp, "%u,", (unsigned int)data->hm_log.task_name[k]);
    // }
    fprintf(fp, "%s,", data->hm_log.partition_name);
    // for (int l = 0; l < 32; l++) {
    //     fprintf(fp, "%u,", (unsigned int)data->hm_log.partition_name[l]);
    // }
    // BEGIN Pcs_timespec_gt
    fprintf(fp, "%u,", data->hm_log.timespec.second);
    fprintf(fp, "%u,", data->hm_log.timespec.nanosecond);
    // END Pcs_timespec_gt
    fprintf(fp, "%lld\n", data->hm_log.entrypoint);
    // END Pcs_exception_info_gt
    // END Cl_cmsw_exception_data_type
}
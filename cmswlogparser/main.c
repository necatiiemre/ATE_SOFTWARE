#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <sys/stat.h>
#include "log_common.h"
#include "log_types.h"
#include "log_mlcmsw.h"
//#include "log_dpm_dtn_es_hm.h"
#include "log_clcmsw_chmc.h"
#include "log_clcmsw_chmc_serializers.h"
#include "log_clcmsw_dtnsw.h"
#include "log_clcmsw_ipmc_generic.h"
#include "log_clcmsw_ipmc_generic_serializers.h"
#include "log_clcmsw_ipmc_dpm.h"
#include "log_clcmsw_ipmc_dsm.h"
#include "log_clcmsw_ipmc_gpm.h"
#include "log_clcmsw_ipmc_hsm.h"
#include "log_clcmsw_ipmc_smmm.h"
#include "log_clcmsw_ipmc_psm_serializers.h"
#include "log_clcmsw_ipmc_hum_serializers.h"
#include "log_clcmsw_ipmc_io_serializers.h"
#include "log_adcu.h"
#include "log_smmm.h"
#include "log_plcmsw.h"
#include "log_plcmsw_mmpl_state.h"
#include "log_clcmsw.h"
#include "log_t2080_exception.h"
#include "mmpl_cmsw_dinter.h"
#include "log_mmpl_cmsw_mmp_status.h"
#include "log_dtnes.h"
#include "dtn_ir_sw_HealthTypes.h"

#define MAX_FILENAME_LEN 128

const char *version="CMSW Log Parser v1.7.0";
char *filename = NULL;
char *filepath = NULL;
char *outfname = NULL;
char *output_folder = NULL;
const char logfilecontent[] = "IPPPLOG";

void ensure_directory(const char *path) {
    if (path == NULL || strlen(path) == 0) return;
    
    mkdir(path); 
}


uint32_t crc32(const uint8_t *data, size_t length) {
    static uint32_t table[256];
    static int initialized = 0;
    
    if (!initialized) {
        for (int i = 0; i < 256; i++) {
            uint32_t crc = i;
            for (int j = 0; j < 8; j++)
                crc = (crc >> 1) ^ (crc & 1 ? 0xEDB88320 : 0);
            table[i] = crc;
        }
        initialized = 1;
    }
    
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++)
        crc = (crc >> 8) ^ table[(crc & 0xFF) ^ data[i]];
    
    return crc ^ 0xFFFFFFFF;
}


uint8_t get_file_pointer(FILE **fp, log_header_type *header)
{
    char path_prefix[MAX_FILENAME_LEN] = "";

    if (output_folder != NULL && strlen(output_folder) > 0) {
        snprintf(path_prefix, MAX_FILENAME_LEN, "%s/", output_folder);
    }

    char current_target_name[MAX_FILENAME_LEN] = "";
    int adcu_msg_chooser = 0;
    
    if(LOG_ADCU == header->log_id.component_type) {
        if(ADCU_HM_CBIT_MSG_SIZE == header->log_payload_len)
            adcu_msg_chooser = 0;
        else if(ADCU_HM_PBIT_MSG_SIZE == header->log_payload_len)
            adcu_msg_chooser = 1;
        else if(ADCU_HM_DCU_ES_MSG_SIZE == header->log_payload_len)
            adcu_msg_chooser = 2;
        else {
            printf("Error: Unknown ADCU message payload size: %u\n", header->log_payload_len);
            exit(1);
        }
        snprintf(current_target_name, MAX_FILENAME_LEN,
            "%sLOG_%s_%s_%u_%u.csv",
            path_prefix,
            log_components[header->log_id.component_type],
            adcu_message[adcu_msg_chooser],
            (header->log_id.device_id >> 8) & 0xFF,
            (header->log_id.device_id >> 3) & 0x1F);
    }
    else if (LOG_DTN_IRSW == header->log_id.component_type) {
        if (DINTER_STATUS_MON_MSG_SIZE == header->log_payload_len)
            adcu_msg_chooser = 0;
        else if (DINTER_PORT_MON_MSG_SIZE == header->log_payload_len)
            adcu_msg_chooser = 1;
        else if (DINTER_PARTIAL_PORT_MON_MSG_SIZE == header->log_payload_len)
            adcu_msg_chooser = 2;
        else if (DINTER_DTN_ES_MON_MSG_SIZE == header->log_payload_len)
            adcu_msg_chooser = 3;
        else if (DINTER_MCU_MON_MSG_SIZE == header->log_payload_len)
            adcu_msg_chooser = 4;
        else {
            printf("Error: Unknown DTN IRSW message payload size: %u\n", header->log_payload_len);
            exit(1);
        }

        snprintf(current_target_name, MAX_FILENAME_LEN,
            "%sLOG_%s_%s_%u_%u.csv",
            path_prefix,
            log_components[header->log_id.component_type],
            dtn_message[adcu_msg_chooser], 
            (header->log_id.device_id >> 8) & 0xFF,
            (header->log_id.device_id >> 3) & 0x1F);
    }
    else {
        snprintf(current_target_name, MAX_FILENAME_LEN,
            "%sLOG_%s_%s_%u_%u.csv",
            path_prefix,
            log_components[header->log_id.component_type],
            log_message[header->message_type],
            (header->log_id.device_id >> 8) & 0xFF,
            (header->log_id.device_id >> 3) & 0x1F);
    }
    
    if (strcmp(current_target_name, outfname) != 0) {
        if (*fp != NULL) {
            fclose(*fp);
            *fp = NULL;
        }
        strncpy(outfname, current_target_name, MAX_FILENAME_LEN);
        
        *fp = fopen(outfname, "a+");
        if (*fp == NULL) {
            printf("Error opening file: %s\n", outfname);
            return 1;
        }
    } 
    else {
        if (*fp == NULL) {
            *fp = fopen(outfname, "a+");
            if (*fp == NULL) return 1;
        }
    }

    return 0;
}


uint8_t write_file_header(FILE **fp, log_header_type *header, int adcu_msg_chooser)
{
    
    fseek(*fp, 0, SEEK_END);
    if (ftell(*fp) > 0) {
        return 1;
    }

    if(LOG_MLCMSW == header->log_id.component_type)
    {
        switch (header->message_type)
        {
            case LOG_STATUS_MSG:
                log_ml_cmsw_status_header_print(*fp);
            break;
            case LOG_DTN_ES_MONITORING:
                log_ml_cmsw_dtn_es_monitoring_header_print(*fp);
            break;
            case LOG_RTOS_EXCEPTIONS:
               log_cl_cmsw_rtos_exception_status_header_print(*fp);
            break;
            case LOG_MAJOR_SYNC_ERROR:
                log_mlcmsw_major_sync_error_header_print(*fp);
            break;
            default:
                printf("UNKNOWN MESSAGE TYPE\n");
            return 1;
        }
    }
    else if(LOG_MMPL_CMSW_MMP_STATUS == header->log_id.component_type)
    {
        switch (header->message_type)
        {
            case LOG_STATUS_MSG:                 
                mmpl_cmsw_mmp_status_header_printer(*fp);
            break;
            default:
                printf("UNKNOWN MESSAGE TYPE\n");
            return 1;
        }
    }
    else if(LOG_DTN_IRSW == header->log_id.component_type)
    {
        switch (header->message_type)
        {
            case LOG_STATUS_MSG:
                dinter_log_print_header(fp, adcu_msg_chooser);
            break;
            default:
                printf("UNKNOWN MESSAGE TYPE\n");
            return 1;
        }
    }
    else if(LOG_ADCU == header->log_id.component_type)
    {
        switch (header->message_type)
        {
            case LOG_STATUS_MSG:
                adcu_log_print_header(fp, adcu_msg_chooser);
            break;
            default:
                printf("UNKNOWN MESSAGE TYPE\n");
            return 1;
        }
    }
    else if(LOG_SMMM  == header->log_id.component_type)
    {
        switch (header->message_type)
        {
            case LOG_STATUS_MSG:
                smmm_log_print_header(fp);
            break;
            default:
                printf("UNKNOWN MESSAGE TYPE\n");
            return 1;
        }
    }
    else if(LOG_MMPL_STATE  == header->log_id.component_type)
    {
        switch (header->message_type)
        {
            case LOG_STATUS_MSG:
                mmpl_state_header_print(fp);
            break;
            default:
                printf("UNKNOWN MESSAGE TYPE\n");
            return 1;
        }
    }
    
    else if(LOG_CL_CMSW == header->log_id.component_type)
    {
        switch (header->message_type)
        {
            case LOG_STATUS_MSG:
                cl_cmsw_log_parser_header(*fp);
            break;
            default:
                printf("UNKNOWN MESSAGE TYPE\n");
            return 1;
        }
    }
    else if(LOG_RTOS_EXCEPTION == header->log_id.component_type )
    {
        switch (header->message_type)
        {
            case LOG_STATUS_MSG:
                log_cl_cmsw_rtos_exception_status_header_print(*fp);
            break;
            case LOG_MAJOR_SYNC_ERROR:
                log_cl_cmsw_rtos_exception_major_sync_error_header_print(*fp);
            break;
            default:
                printf("UNKNOWN MESSAGE TYPE\n");
            return 1;
        }
    }
    else if(LOG_DTNSW == header->log_id.component_type)
    {
        switch (header->message_type)
        {
            case LOG_STATUS_MSG:
                dsm_dtnsw_log_print_header(fp);
            break;
            default:
                printf("UNKNOWN MESSAGE TYPE\n");
            return 1;
        }
    }
    else if(LOG_DTNES == header->log_id.component_type)
    {
        switch (header->message_type)
        {
            case LOG_STATUS_MSG:
                dsm_dtnes_header_print(*fp);
            break;
            default:
                printf("UNKNOWN MESSAGE TYPE\n");
            return 1;
        }
    }
    else if(LOG_CHMC == header->log_id.component_type)
    {
        switch (header->message_type)
        {
            case LOG_STATUS_MSG:
                chmc_log_print_header(fp);
            break;
            default:
                printf("UNKNOWN MESSAGE TYPE\n");
            return 1;
        }
    }    
    else if(LOG_IPMC_DPM == header->log_id.component_type)
    {
        switch (header->message_type)
        {
            case LOG_STATUS_MSG:
                ipmc_dpm_log_print_header(fp);
            break;
            default:
                printf("UNKNOWN MESSAGE TYPE\n");
            return 1;
        }
    }
    else if(LOG_IPMC_DSM  == header->log_id.component_type)
    {
        switch (header->message_type)
        {
            case LOG_STATUS_MSG:
                ipmc_dsm_log_header_print(fp);
            break;
            default:
                printf("UNKNOWN MESSAGE TYPE\n");
            return 1;
        }
    }
    else if(LOG_IPMC_HSM  == header->log_id.component_type)
    {
        switch (header->message_type)
        {
            case LOG_STATUS_MSG:
                ipmc_hsm_log_header_print(fp);
            break;
            default:
                printf("UNKNOWN MESSAGE TYPE\n");
            return 1;
        }
    }
    else if(LOG_IPMC_GPM  == header->log_id.component_type)
    {
        switch (header->message_type)
        {
            case LOG_STATUS_MSG:
                ipmc_gpm_log_print_header(fp);
            break;
            default:
                printf("UNKNOWN MESSAGE TYPE\n");
            return 1;
        }
    }
    
    else if(LOG_IPMC_PSM  == header->log_id.component_type)
    {
        switch (header->message_type)
        {
            case LOG_STATUS_MSG:
                log_clcmsw_ipmc_psm_clcmsw_ipmc_psm_data_log_header_print(fp, 1);
            break;
            default:
                printf("UNKNOWN MESSAGE TYPE\n");
            return 1;
        }
    }
    else if(LOG_IPMC_HUM  == header->log_id.component_type)
    {
        switch (header->message_type)
        {
            case LOG_STATUS_MSG:
                log_clcmsw_ipmc_hum_clcmsw_ipmc_hum_data_log_header_print(fp, 1);
            break;
            default:
                printf("UNKNOWN MESSAGE TYPE\n");
            return 1;
        }
    }
    else if(LOG_IPMC_SMMM  == header->log_id.component_type)
    {
        switch (header->message_type)
        {
            case LOG_STATUS_MSG:
                ipmc_smmm_log_header_print(fp);
            break;
            default:
                printf("UNKNOWN MESSAGE TYPE\n");
            return 1;
        }
    }
    else if(LOG_IPMC_IOCM  == header->log_id.component_type)
    {
        switch (header->message_type)
        {
            case LOG_STATUS_MSG:
                log_clcmsw_ipmc_io_clcmsw_ipmc_io_data_log_header_print(fp, 1);
            break;
            default:
                printf("UNKNOWN MESSAGE TYPE\n");
            return 1;
        }
    }
    return 0;
}


long file_operations(FILE **file, uint8_t **buffer)
{
    long file_size = 0;
    *file = fopen(filepath, "rb");

    if (*file == NULL) {
        perror("Error opening file");
        return 0;
    }

    // 2. Determine the size of the file
    fseek(*file, 0, SEEK_END);   // Seek to the end of the file
    file_size = ftell(*file);     // Get the current file pointer position (which is the file size)
    rewind(*file);                // Reset the file pointer to the beginning of the file

    // 3. Allocate memory for the buffer
    *buffer = (char *)malloc(file_size);

    if (*buffer == NULL) {
        perror("Memory allocation failed");
        fclose(*file);  // Close the file before exiting if malloc fails
        return 0;
    }

    // 4. Read the entire file content into the buffer
    size_t bytes_read = fread(*buffer, 1, file_size, *file);

    if (bytes_read != file_size) {
        if (feof(*file)) {
            fprintf(stderr, "End of file reached before reading all data.\n");
        } else {
            perror("Error reading file");
        }
        free(*buffer);
        fclose(*file);
        return 0;
    }
    return file_size;
}


uint8_t get_detections_file_pointer(FILE **fp)
{
    long fposition = 0;

    if (output_folder != NULL) {
        ensure_directory(output_folder);
        snprintf(outfname, MAX_FILENAME_LEN, "%s/%s.csv", output_folder, filename);
    } else {
        snprintf(outfname, MAX_FILENAME_LEN, "%s.csv", filename);
    }

    *fp = fopen(outfname, "a+");

    if (fp == NULL) {
        printf("Error opening new txt file: %s\n", outfname);
        return 1;
    }

    fseek(*fp, 0, SEEK_END);
    fposition = ftell(*fp);
    if (fposition == 0)
    {
        fprintf(*fp,  "TIMESTAMP SEC,");
        fprintf(*fp,  "TIMESTAMP NSEC,");
        fprintf(*fp,  "ENTRY_COUNT,");
        fprintf(*fp,  "DROP_COUNT,");
        fprintf(*fp,  "EVENT,");
        fprintf(*fp,  "SEVERITY,");
        fprintf(*fp,  "PARAMETER_1,");
        fprintf(*fp,  "PARAMETER_2\n");
    }

    return 0;
}

int handle_detections_log(int is_ml, int is_cl, int is_pl, uint8_t **buffer, long file_size, int cursor)
{
    FILE *outfp;

    while(cursor < file_size)
    {
        if(get_detections_file_pointer(&outfp))
        {
            printf("%d\n\r",cursor);
            printf("HATA\n\r");
            return 1;
        }
        if(is_ml)
        {
            MLCMSW_LOG_FRAME_TYPE *log_frame;
            log_frame = (MLCMSW_LOG_FRAME_TYPE *) (*buffer + cursor);

            size_t header_size = sizeof(mlcmsw_log_frame_header_t);
            size_t entry_size = sizeof(mlcmsw_log_entry_t);
            size_t total_entries_size = ntohl(log_frame->log_frame_header.log_entry_count) * entry_size;
            size_t total_frame_size = header_size + total_entries_size;

            mlcmsw_detections_parser(outfp, &log_frame);
            
            cursor += total_frame_size;
            
        }
        else if(is_pl)
        {
            MMPLCMSW_LOG_FRAME_TYPE *log_frame;
            log_frame = (MMPLCMSW_LOG_FRAME_TYPE *) (*buffer + cursor);
            mmplcmsw_detections_parser(outfp, &log_frame);

            cursor += (sizeof(mmplcmsw_log_frame_header_t) + (sizeof(mmplcmsw_log_entry_t) * log_frame->log_frame_header.log_entry_count));
            
        }
        else if(is_cl)
        {
            CLCMSW_LOG_FRAME_TYPE *log_frame;
            log_frame = (CLCMSW_LOG_FRAME_TYPE *) (*buffer + cursor);
            clcmsw_detections_parser(outfp, &log_frame);

            cursor += (sizeof(clcmsw_log_frame_header_t) + (sizeof(clcmsw_log_entry_t) * log_frame->log_frame_header.log_entry_count));
            
        }
        else
        {
            printf("Unknown log file\n");
            break;
        }
        fclose(outfp);
    }

    if (outfp != NULL) {
        fclose(outfp);
    }

    free(outfname);
    outfname = NULL;

    free(*buffer);

    printf("Conversion Done\n\r");

    
    return 0;
}


int main(int argc, char* argv[])
{
    outfname = malloc(MAX_FILENAME_LEN);
    if (outfname == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    } 

    outfname[0] = '\0';


    printf("%s\n", version);
    printf("This version supports parsing of following log files:\n");
    printf("  - Module Level CMSW log file\n");
    printf("  - Chassis log file\n");
    printf("  - ADCU log file\n");
    printf("  - SMMM log file\n\r");

    printf("Options:\n");
    printf("  -i <input_file>    : Source log file to parse (Required)\n");
    printf("  --o <output_folder>   : Custom output CSV path (Optional)\n\r");

    printf("Warning: .csv log file is opened in append mode, meaning every time this executable is run, logs are appended\n\r");

    if (argc < 2 || strcmp(argv[1], "-i") != 0) {
        fprintf(stderr, "Error: Missing or incorrect input flag (-i is required).\n");
        fprintf(stderr, "Usage: %s -i <source_file> [--o <output_folder>]\n", argv[0]);
        return 1;
    }

    if (argc > 2) {
        filepath = argv[2];
    } else {
        fprintf(stderr, "Error: -i flag provided but no file specified.\n");
        return 1;
    }

    output_folder = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--o") == 0 && i + 1 < argc) {
            output_folder = argv[i + 1];
        }
    }

    char temp_path[512];
    strncpy(temp_path, filepath, sizeof(temp_path) - 1);
    temp_path[sizeof(temp_path) - 1] = '\0';

    char *last_slash = strrchr(temp_path, '/');
    char *last_backslash = strrchr(temp_path, '\\');
    char *found_separator = NULL;

    if (last_slash != NULL && last_backslash != NULL) {
        found_separator = (last_slash > last_backslash) ? last_slash : last_backslash;
    } else {
        found_separator = (last_slash != NULL) ? last_slash : last_backslash;
    }

    if (found_separator != NULL) {
        filename = found_separator + 1;
    } else {
        filename = temp_path;
    }

    printf("Source File Path: %s\n", filepath);
    printf("Source File Name: %s\n", filename);

    FILE *file = NULL;
    char last_written_filename[MAX_FILENAME_LEN] = "";
    uint8_t *buffer = NULL;
    long file_size;

    file_size = file_operations(&file, &buffer);
    // 5. Process the buffer (e.g., print its contents - be careful with large files!)
    if(file_size > 0)
    {
        printf("File '%s' loaded successfully. Size: %ld bytes\n", filename, file_size);
        fclose(file);
    }
    else
    {
        printf("Error loading '%s'\n", filename, file_size);
        fclose(file);
        return 1;
    }

    int cursor = 0;

    // BEGIN TEMP CODE BLOCK (due to mlcmsw log file header)
    // uint64_t log_file_identifier[8]; /* 0x495050504C4F4700 */
    if(0 == memcmp(mlcmsw_log_fname_start, filename, sizeof(mlcmsw_log_fname_start) - 1)) {
        log_file_header_mlcmsw *file_header_mlcmsw_detection = (log_file_header_mlcmsw *) &buffer[cursor];
        cursor += sizeof(log_file_header_mlcmsw);

        file_header_mlcmsw_detection->log_file_type = ntohl(file_header_mlcmsw_detection->log_file_type);

        file_header_mlcmsw_detection->log_file_owner_geographical_address = ntohs(file_header_mlcmsw_detection->log_file_owner_geographical_address);

        file_header_mlcmsw_detection->log_file_owner_partition_id = (int32_t)ntohl((uint32_t)file_header_mlcmsw_detection->log_file_owner_partition_id);

        file_header_mlcmsw_detection->log_file_creation_time = ntohll(file_header_mlcmsw_detection->log_file_creation_time);

        printf("\n--- LOG FILE HEADER DETAILS ---\n");

        printf("Version: %u.%u\n", file_header_mlcmsw_detection->log_file_version_major, file_header_mlcmsw_detection->log_file_version_minor);

        printf("Component Type: %u\n", file_header_mlcmsw_detection->log_file_type);

        printf("Owner Type: %u\n", file_header_mlcmsw_detection->log_file_owner_type);

        printf("Geo Address: %u\n", file_header_mlcmsw_detection->log_file_owner_geographical_address);

        printf("Partition ID: %d\n", file_header_mlcmsw_detection->log_file_owner_partition_id);

        printf("Creation Time: %llu\n", file_header_mlcmsw_detection->log_file_creation_time);

        printf("-------------------------------\n\n");

        if(LOG_MLCMSW_DETECTIONS == file_header_mlcmsw_detection->log_file_type) {
            return handle_detections_log(1,0,0, &buffer, file_size, cursor);
        }

    }
    // END TEMP CODE BLOCK

    // uint64_t log_file_identifier; /* 0x495050504C4F4700 */
    else { 
        log_file_header *file_header = (log_file_header *) &buffer[cursor];
        cursor += sizeof(log_file_header);

        if(0 == memcmp(&file_header->log_file_identifier, logfilecontent, sizeof(file_header->log_file_identifier))) {
            file_header->log_file_type = ntohl(file_header->log_file_type);

            file_header->log_file_owner_geographical_address = ntohs(file_header->log_file_owner_geographical_address);

            file_header->log_file_owner_partition_id = (int32_t)ntohl((uint32_t)file_header->log_file_owner_partition_id);

            file_header->log_file_creation_time = ntohll(file_header->log_file_creation_time);

            printf("\n--- LOG FILE HEADER DETAILS ---\n");

            printf("Version: %u.%u\n", file_header->log_file_version_major, file_header->log_file_version_minor);

            printf("Component Type: %u\n", file_header->log_file_type);

            printf("Owner Type: %u\n", file_header->log_file_owner_type);

            printf("Geo Address: %u\n", file_header->log_file_owner_geographical_address);

            printf("Partition ID: %d\n", file_header->log_file_owner_partition_id);

            printf("Creation Time: %llu\n", file_header->log_file_creation_time);

            printf("-------------------------------\n\n");
        }
        else {
            printf("OLD LOG FILE FORMAT\n");
        }

        if(LOG_CLINTERNAL == file_header->log_file_type) {
            return handle_detections_log(0,1,0, &buffer, file_size, cursor);
        }
        else if(LOG_MMPLINTERNAL == file_header->log_file_type) {
            return handle_detections_log(0,0,1, &buffer, file_size, cursor);
        }
    }

    FILE *outfp = NULL;
    log_data_type *mylog;
    while(cursor < file_size)
    {
        mylog = (log_data_type *) &buffer[cursor];

        // DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP,LOG PAYLOAD LENGTH
        mylog->header.log_id.device_id = ntohs(mylog->header.log_id.device_id);
        mylog->header.log_id.component_type = ntohl(mylog->header.log_id.component_type);
        mylog->header.message_type = ntohl(mylog->header.message_type);
        mylog->header.timestamp = ntohll(mylog->header.timestamp);
        mylog->header.log_payload_len = ntohl(mylog->header.log_payload_len);

        if(get_file_pointer(&outfp, &mylog->header)) {
            printf("%d\n\r",cursor);
            printf("ERROR: File could not be opened!\n\r");
            return 1;
        }

        if (strcmp(outfname, last_written_filename) != 0) {
            
            int chooser = 0;
            if (LOG_ADCU == mylog->header.log_id.component_type) {
                if (ADCU_HM_CBIT_MSG_SIZE == mylog->header.log_payload_len) chooser = 0;
                else if (ADCU_HM_PBIT_MSG_SIZE == mylog->header.log_payload_len) chooser = 1;
                else if (ADCU_HM_DCU_ES_MSG_SIZE == mylog->header.log_payload_len) chooser = 2;
            }

            if(LOG_DTN_IRSW == mylog->header.log_id.component_type) {
                if (DINTER_STATUS_MON_MSG_SIZE == mylog->header.log_payload_len) chooser = 0;
                else if (DINTER_PORT_MON_MSG_SIZE == mylog->header.log_payload_len) chooser = 1;
                else if (DINTER_PARTIAL_PORT_MON_MSG_SIZE == mylog->header.log_payload_len) chooser = 2;
                else if (DINTER_DTN_ES_MON_MSG_SIZE == mylog->header.log_payload_len) chooser = 3;
                else if (DINTER_MCU_MON_MSG_SIZE == mylog->header.log_payload_len) chooser = 4;
            }

            write_file_header(&outfp, &mylog->header, chooser);
            
            strcpy(last_written_filename, outfname);
        }

        fprintf(outfp, "%u,%u,%u,%llu,%u,", mylog->header.log_id.device_id,
                            mylog->header.log_id.component_type,
                            mylog->header.message_type,
                            mylog->header.timestamp,
                            mylog->header.log_payload_len);

        if(LOG_MLCMSW == mylog->header.log_id.component_type)
        {
            if(LOG_STATUS_MSG == mylog->header.message_type)
            {
                log_ml_cmsw_status_data_print(outfp, (Cmsw_ml_status_msg_type *) &buffer[cursor+sizeof(log_header_type)]);
            }
            else if(LOG_DTN_ES_MONITORING == mylog->header.message_type)
            {
                log_ml_cmsw_dtn_es_monitoring_data_print(outfp, (tA664ESMonitoring *) &buffer[cursor+sizeof(log_header_type)]);
            }
            else if(LOG_RTOS_EXCEPTIONS == mylog->header.message_type)
            {
                log_cl_cmsw_rtos_exception_status_data_print(outfp, (Cl_cmsw_exception_data_type *) &buffer[cursor+sizeof(log_header_type)]);
            }
            else if(LOG_MAJOR_SYNC_ERROR == mylog->header.message_type)
            {
                log_mlcmsw_major_sync_error_data_print(outfp, (Pcs_hm_stats_info *) &buffer[cursor+sizeof(log_header_type)]);
            }
        }
        else if(LOG_MMPL_CMSW_MMP_STATUS == mylog->header.log_id.component_type)
        {
            if(LOG_STATUS_MSG == mylog->header.message_type)
            {
                mmpl_cmsw_mmp_status_parser(outfp, (Mmplcmsw_tai_icd_status_t *) &buffer[cursor+sizeof(log_header_type)]);
            }
        }
        else if(LOG_DTN_IRSW == mylog->header.log_id.component_type)
        {
            if(LOG_STATUS_MSG == mylog->header.message_type)
            {
                if (DINTER_STATUS_MON_MSG_SIZE == mylog->header.log_payload_len)
                {
                    dinter_status_mon_log_parser(outfp, (uint8_t *)&buffer[cursor + sizeof(log_header_type)]);
                }
                else if (DINTER_PORT_MON_MSG_SIZE == mylog->header.log_payload_len)
                {
                   dinter_port_mon_log_parser(outfp, (uint8_t *)&buffer[cursor + sizeof(log_header_type)]);
                }
                else if (DINTER_PARTIAL_PORT_MON_MSG_SIZE == mylog->header.log_payload_len)
                {
                   dinter_partial_port_mon_log_parser(outfp, (uint8_t *)&buffer[cursor + sizeof(log_header_type)]);
                }
                else if (DINTER_DTN_ES_MON_MSG_SIZE == mylog->header.log_payload_len)
                {
                    dinter_dtn_es_log_parser(outfp, (dtn_es_mon *) &buffer[cursor+sizeof(log_header_type)]);
                }
                else if (DINTER_MCU_MON_MSG_SIZE == mylog->header.log_payload_len)
                {
                    dinter_cba_hm_log_parser(outfp, (uint8_t *)&buffer[cursor + sizeof(log_header_type)]);
                }
                else
                {
                    printf("Unknown DTN IRSW payload size %u\n", mylog->header.log_payload_len);
                }
            }
        }
        else if(LOG_ADCU == mylog->header.log_id.component_type)
        {
            if(LOG_STATUS_MSG == mylog->header.message_type)
            {
                if(ADCU_HM_CBIT_MSG_SIZE == mylog->header.log_payload_len)
                {
                    adcu_cbit_log_parser(outfp, (Cmsw_pl_adcu_cbit_msg_type *) &buffer[cursor+sizeof(log_header_type)]);
                }
                else if(ADCU_HM_PBIT_MSG_SIZE == mylog->header.log_payload_len)
                {
                    adcu_pbit_log_parser(outfp, (Cmsw_pl_adcu_pbit_msg_type *) &buffer[cursor+sizeof(log_header_type)]);
                }
                else if(ADCU_HM_DCU_ES_MSG_SIZE == mylog->header.log_payload_len)
                {
                    adcu_dtnes_log_parser(outfp, (dtn_es_mon *) &buffer[cursor+sizeof(log_header_type)]);
                }
                else
                {
                    printf("Unknown ADCU payload size %u\n", mylog->header.log_payload_len);
                }
            }
        }
        else if(LOG_SMMM == mylog->header.log_id.component_type)
        {
            if(LOG_STATUS_MSG == mylog->header.message_type)
            {
                smmm_log_parser(outfp, (Mmplcmsw_smmm_monitoring_msg_type *) &buffer[cursor+sizeof(log_header_type)]);
            }
        }
        else if(LOG_MMPL_STATE == mylog->header.log_id.component_type)
        {
            if(LOG_STATUS_MSG == mylog->header.message_type)
            {
                mmpl_state_log_parser(outfp, (Cmsw_pl_program_state_type *) &buffer[cursor+sizeof(log_header_type)]);
            }
        }
        else if(LOG_CL_CMSW == mylog->header.log_id.component_type)
        {
            if(LOG_STATUS_MSG == mylog->header.message_type)
            {
                cl_cmsw_log_parser_data(outfp, (Cl_cmsw_status_report_msg_type *) &buffer[cursor+sizeof(log_header_type)]);
            }
        }
        else if(LOG_RTOS_EXCEPTION == mylog->header.log_id.component_type)
        {
            if(LOG_STATUS_MSG == mylog->header.message_type)
            {
                log_cl_cmsw_rtos_exception_status_data_print(outfp, (Cl_cmsw_exception_data_type *) &buffer[cursor+sizeof(log_header_type)]);
            }
            else if(LOG_MAJOR_SYNC_ERROR == mylog->header.message_type)
            {
                log_cl_cmsw_rtos_exception_major_sync_error_data_print(outfp, (Pcs_hm_stats_info *) &buffer[cursor+sizeof(log_header_type)]);
            }
        }
        else if(LOG_DTNSW == mylog->header.log_id.component_type)
        {
            if(LOG_STATUS_MSG == mylog->header.message_type)
            {
                dsm_dtnsw_log_parser(outfp, (tA664SWMonitoring *) &buffer[cursor+sizeof(log_header_type)]);
            }
        }
        else if(LOG_DTNES == mylog->header.log_id.component_type)
        {
            if(LOG_STATUS_MSG == mylog->header.message_type)
            {
                dsm_dtnes_data_print(outfp, (tA664ESMonitoring *) &buffer[cursor+sizeof(log_header_type)]);
            }
        }
        else if(LOG_CHMC == mylog->header.log_id.component_type)
        {
            if(LOG_STATUS_MSG == mylog->header.message_type)
            {
                chmc_log_parser(outfp, (pcs_chassis_man_record_table_type *) &buffer[cursor+sizeof(log_header_type)]);
            }
        }
        else if(LOG_IPMC_DPM == mylog->header.log_id.component_type)
        {
            if(LOG_STATUS_MSG == mylog->header.message_type)
            {
                ipmc_dpm_log_parser(outfp, (clcmsw_ipmc_dpm_data *) &buffer[cursor+sizeof(log_header_type)]);
            }
        }
        else if(LOG_IPMC_DSM == mylog->header.log_id.component_type)
        {
            if(LOG_STATUS_MSG == mylog->header.message_type)
            {
                ipmc_dsm_log_parser(outfp, (clcmsw_ipmc_dsm_data *) &buffer[cursor+sizeof(log_header_type)]);
            }
        }
        else if(LOG_IPMC_HSM == mylog->header.log_id.component_type)
        {
            if(LOG_STATUS_MSG == mylog->header.message_type)
            {
                ipmc_hsm_log_parser(outfp, (clcmsw_ipmc_hsm_data *) &buffer[cursor+sizeof(log_header_type)]);
            }
        }
        else if(LOG_IPMC_GPM == mylog->header.log_id.component_type)
        {
            if(LOG_STATUS_MSG == mylog->header.message_type)
            {
                ipmc_gpm_log_parser(outfp, (clcmsw_ipmc_gpm_data *) &buffer[cursor+sizeof(log_header_type)]);
            }
        }
        else if(LOG_IPMC_PSM == mylog->header.log_id.component_type)
        {
            if(LOG_STATUS_MSG == mylog->header.message_type)
            {
                log_clcmsw_ipmc_psm_clcmsw_ipmc_psm_data_log_parser(outfp, (clcmsw_ipmc_psm_data *) &buffer[cursor+sizeof(log_header_type)]);
            }
        }
        else if(LOG_IPMC_HUM == mylog->header.log_id.component_type)
        {
            if(LOG_STATUS_MSG == mylog->header.message_type)
            {
                log_clcmsw_ipmc_hum_clcmsw_ipmc_hum_data_log_parser(outfp, (clcmsw_ipmc_hum_data *) &buffer[cursor+sizeof(log_header_type)]);
            }
        }
        else if(LOG_IPMC_SMMM == mylog->header.log_id.component_type)
        {
            if(LOG_STATUS_MSG == mylog->header.message_type)
            {
                ipmc_smmm_log_parser(outfp, (clcmsw_ipmc_smmm_data *) &buffer[cursor+sizeof(log_header_type)]);
            }
        }
        else if(LOG_IPMC_IOCM == mylog->header.log_id.component_type)
        {
            if(LOG_STATUS_MSG == mylog->header.message_type)
            {
                log_clcmsw_ipmc_io_clcmsw_ipmc_io_data_log_parser(outfp, (clcmsw_ipmc_io_data *) &buffer[cursor+sizeof(log_header_type)]);
            }
        }
        
        cursor += mylog->header.log_payload_len + sizeof(log_header_type);
    
    }
    if (outfp != NULL) {
        fclose(outfp);
    }
    free(outfname);
    outfname = NULL;

    free(buffer);
    printf("Conversion Done\n\r");
    return 0;
}
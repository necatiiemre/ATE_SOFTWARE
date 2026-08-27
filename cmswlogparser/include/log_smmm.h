#pragma once
#include <stdint.h>

typedef struct __attribute__((packed)){
	uint16_t device_id;
	uint8_t  msg_type;
	uint16_t msg_len;
	uint16_t hw_unique_id;
	uint16_t fpga_code_ver;
	uint16_t security_controller_fw_ver;
	uint16_t ipmc_fw_ver;
	uint8_t  cfg_id;
	uint8_t  smmm_mode_info;
	uint16_t alarm_status;
	uint16_t  cbit_results;
	uint16_t  pbit_results;
	uint16_t voltage;
	uint16_t current;
	uint16_t temperature;
	uint16_t battery_voltage;
	uint8_t  latest_pilot_auth_status;
	uint64_t pilot_auth_status_occurence_date_time;
	uint8_t  smmm_storage_lock_status;
	uint64_t smmm_rtc_time;
}Mmplcmsw_smmm_monitoring_msg_type;

static int smmm_log_print_header(FILE **fp) {
    fprintf(*fp,  "DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP,LOG PAYLOAD LENGTH,");

    fprintf(*fp, "%s,", "device_id");
    fprintf(*fp, "%s,", "msg_type");
    fprintf(*fp, "%s,", "msg_len");
    fprintf(*fp, "%s,", "hw_unique_id");
    fprintf(*fp, "%s,", "fpga_code_ver");
    fprintf(*fp, "%s,", "security_controller_fw_ver");
    fprintf(*fp, "%s,", "ipmc_fw_ver");
    fprintf(*fp, "%s,", "cfg_id");
    fprintf(*fp, "%s,", "smmm_mode_info");
    fprintf(*fp, "%s,", "alarm_status");
    fprintf(*fp, "%s,", "cbit_results");
    fprintf(*fp, "%s,", "pbit_results");
    fprintf(*fp, "%s,", "voltage");
    fprintf(*fp, "%s,", "current");
    fprintf(*fp, "%s,", "temperature");
    fprintf(*fp, "%s,", "battery_voltage");
    fprintf(*fp, "%s,", "latest_pilot_auth_status");
    fprintf(*fp, "%s,", "pilot_auth_status_occurence_date_time");
    fprintf(*fp, "%s,", "smmm_storage_lock_status");
    fprintf(*fp, "%s\n", "smmm_rtc_time");
}

static int smmm_log_parser(FILE *outfp, Mmplcmsw_smmm_monitoring_msg_type *smmm)
{
    // Convert all network byte order fields to host byte order
    smmm->device_id = ntohs(smmm->device_id);
    smmm->msg_len = ntohs(smmm->msg_len);
    smmm->hw_unique_id = ntohs(smmm->hw_unique_id);
    smmm->fpga_code_ver = ntohs(smmm->fpga_code_ver);
    smmm->security_controller_fw_ver = ntohs(smmm->security_controller_fw_ver);
    smmm->ipmc_fw_ver = ntohs(smmm->ipmc_fw_ver);
    smmm->alarm_status = ntohs(smmm->alarm_status);
    smmm->cbit_results = ntohs(smmm->cbit_results);
    smmm->pbit_results = ntohs(smmm->pbit_results);
    smmm->voltage = ntohs(smmm->voltage);
    smmm->current = ntohs(smmm->current);
    smmm->temperature = ntohs(smmm->temperature);
    smmm->battery_voltage = ntohs(smmm->battery_voltage);
    smmm->pilot_auth_status_occurence_date_time = ntohll(smmm->pilot_auth_status_occurence_date_time);
    smmm->smmm_rtc_time = ntohll(smmm->smmm_rtc_time);
    
    
    // Write to output file
    fprintf(outfp, "%u,", smmm->device_id);
    fprintf(outfp, "%u,", smmm->msg_type);
    fprintf(outfp, "%u,", smmm->msg_len);
    fprintf(outfp, "%u,", smmm->hw_unique_id);
    fprintf(outfp, "%u,", smmm->fpga_code_ver);
    fprintf(outfp, "%u,", smmm->security_controller_fw_ver);
    fprintf(outfp, "%u,", smmm->ipmc_fw_ver);
    fprintf(outfp, "%u,", smmm->cfg_id);
    fprintf(outfp, "%u,", smmm->smmm_mode_info);
    fprintf(outfp, "%u,", smmm->alarm_status);
    fprintf(outfp, "%u,", smmm->cbit_results);
    fprintf(outfp, "%u,", smmm->pbit_results);
    fprintf(outfp, "%u,", smmm->voltage);
    fprintf(outfp, "%u,", smmm->current);
    fprintf(outfp, "%u,", smmm->temperature);
    fprintf(outfp, "%u,", smmm->battery_voltage);
    fprintf(outfp, "%u,", smmm->latest_pilot_auth_status);
    fprintf(outfp, "%llu,", smmm->pilot_auth_status_occurence_date_time);
    fprintf(outfp, "%u,", smmm->smmm_storage_lock_status);
    fprintf(outfp, "%llu\n", smmm->smmm_rtc_time);

    return 0;
}
#pragma once
typedef struct pcs_gpm_board_data
{
    /* pgood */
    pcs_good_type pg_3v3_fo;
    pcs_good_type pg_0v9_gpu_core;
    pcs_good_type pg_0v9_gpu_vddci;
    pcs_good_type pg_1v35_gpu_vmeio;
    pcs_good_type pg_1v8_gpu;
    pcs_good_type pg_3v3_gpu;
    pcs_good_type pg_0v85_fpga_vccint;
    pcs_good_type pg_0v9_fpga_mgtavcc;
    pcs_good_type pg_1v2_fpga_mgtavtt;
    pcs_good_type pg_1v8_fpga_vcco;
    pcs_good_type pg_1v8_fpga_mgtvccaux;
    pcs_good_type pg_1v2_fpga_ddr;
    pcs_good_type pg_3v3_fpga_vcco;
    pcs_good_type pg_3v3_clock;
    pcs_good_type pg_1v8_clock;

    /* valid */
    pcs_valid_type pg_3v3_fo_valid;
    pcs_valid_type pg_0v9_gpu_core_valid;
    pcs_valid_type pg_0v9_gpu_vddci_valid;
    pcs_valid_type pg_1v35_gpu_vmeio_valid;
    pcs_valid_type pg_1v8_gpu_valid;
    pcs_valid_type pg_3v3_gpu_valid;
    pcs_valid_type pg_0v85_fpga_vccint_valid;
    pcs_valid_type pg_0v9_fpga_mgtavcc_valid;
    pcs_valid_type pg_1v2_fpga_mgtavtt_valid;
    pcs_valid_type pg_1v8_fpga_vcco_valid;
    pcs_valid_type pg_1v8_fpga_mgtvccaux_valid;
    pcs_valid_type pg_1v2_fpga_ddr_valid;
    pcs_valid_type pg_3v3_fpga_vcco_valid;
    pcs_valid_type pg_3v3_clock_valid;
    pcs_valid_type pg_1v8_clock_valid;

    /* temperatures */
    Pcs_ipmc_signed_value_type tmp451_sensor_1;
    Pcs_ipmc_signed_value_type tmp451_sensor_2;
    Pcs_ipmc_signed_value_type tmp451_sensor_3;
    Pcs_ipmc_signed_value_type tmp451_sensor_4;

    /* voltage */
    Pcs_ipmc_unsigned_value_type vols_3v3_fo;
    Pcs_ipmc_unsigned_value_type plus_3v3_gpu;
    Pcs_ipmc_unsigned_value_type plus_0v9_gpu_core;
    Pcs_ipmc_unsigned_value_type plus_0v9_gpu_vddci;
    Pcs_ipmc_unsigned_value_type plus_1v8_gpu;
    Pcs_ipmc_unsigned_value_type plus_1v35_gpu_vmeio;
    Pcs_ipmc_unsigned_value_type plus_0v85_fpga_vccint;
    Pcs_ipmc_unsigned_value_type plus_0v9_fpga_mgtavcc;
    Pcs_ipmc_unsigned_value_type plus_1v2_fpga_mgtavtt;
    Pcs_ipmc_unsigned_value_type plus_3v3_fpga;
    Pcs_ipmc_unsigned_value_type plus_1v2_fpga_ddr;
    Pcs_ipmc_unsigned_value_type plus_1v8_fpga_mgtvccaux;
    Pcs_ipmc_unsigned_value_type plus_1v8_fpga_vcco;
    Pcs_ipmc_unsigned_value_type plus_3v3_clock;
    Pcs_ipmc_unsigned_value_type plus_1v8_clock;

    /* current */
    Pcs_ipmc_unsigned_value_type curs_3v3_fo;
    Pcs_ipmc_unsigned_value_type curs_5v0;
    Pcs_ipmc_unsigned_value_type curs_12v0;
    Pcs_ipmc_unsigned_value_type curs_3v3_gpu;
    Pcs_ipmc_unsigned_value_type curs_0v9_gpu_core;
    Pcs_ipmc_unsigned_value_type curs_0v9_gpu_vddci;
    Pcs_ipmc_unsigned_value_type curs_1v8_gpu;
    Pcs_ipmc_unsigned_value_type curs_1v35_gpu_vmeio;
    Pcs_ipmc_unsigned_value_type curs_0v85_fpga_vccint;
    Pcs_ipmc_unsigned_value_type curs_0v9_fpga_mgtavcc;
    Pcs_ipmc_unsigned_value_type curs_1v2_fpga_mgtavtt;
    Pcs_ipmc_unsigned_value_type curs_3v3_fpga_vcco;
    Pcs_ipmc_unsigned_value_type curs_1v2_fpga_ddr;
    Pcs_ipmc_unsigned_value_type curs_1v8_fpga_mgtvccaux;
    Pcs_ipmc_unsigned_value_type curs_1v8_fpga_vcco;
    Pcs_ipmc_unsigned_value_type curs_3v3_clock;
    Pcs_ipmc_unsigned_value_type curs_1v8_clock;

    /* other */
    pcs_good_type lol_clk_gen;
    pcs_good_type fpga_init_3v3;
    pcs_good_type fpga_done_3v3;
    pcs_good_type los_clk_buf;
    pcs_good_type ready_ipmba;
    pcs_good_type ready_ipmbb;

    /* other valid*/
    pcs_valid_type lol_clk_gen_valid;
    pcs_valid_type fpga_init_3v3_valid;
    pcs_valid_type fpga_done_3v3_valid;
    pcs_valid_type los_clk_buf_valid;
    pcs_valid_type ready_ipmba_valid;
    pcs_valid_type ready_ipmbb_valid;
} pcs_gpm_board_data;

typedef struct{
	clcmsw_generic_ipmc_data_type generic_ipmc;
	pcs_gpm_board_data gpm_data;
}clcmsw_ipmc_gpm_data;

static int ipmc_gpm_log_print_header(FILE **fp) {
    fprintf(*fp,  "DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP,LOG PAYLOAD LENGTH,");
    fprintf(*fp, "%s,", "LRM_dev");
    fprintf(*fp, "%s,", "is_auth");
    fprintf(*fp, "%s,", "ipmb_a");
    fprintf(*fp, "%s,", "ipmb_b");
    fprintf(*fp, "%s,", "fw_version");
    fprintf(*fp, "%s,", "operation_mode");
    fprintf(*fp, "%s,", "sequence_number");
    fprintf(*fp, "%s,", "power_status");
    fprintf(*fp, "%s,", "reset_counter");
    fprintf(*fp, "%s,", "health_data_refresh_counter");

    // pgood fields
    fprintf(*fp, "%s,", "pg_3v3_fo");
    fprintf(*fp, "%s,", "pg_0v9_gpu_core");
    fprintf(*fp, "%s,", "pg_0v9_gpu_vddci");
    fprintf(*fp, "%s,", "pg_1v35_gpu_vmeio");
    fprintf(*fp, "%s,", "pg_1v8_gpu");
    fprintf(*fp, "%s,", "pg_3v3_gpu");
    fprintf(*fp, "%s,", "pg_0v85_fpga_vccint");
    fprintf(*fp, "%s,", "pg_0v9_fpga_mgtavcc");
    fprintf(*fp, "%s,", "pg_1v2_fpga_mgtavtt");
    fprintf(*fp, "%s,", "pg_1v8_fpga_vcco");
    fprintf(*fp, "%s,", "pg_1v8_fpga_mgtvccaux");
    fprintf(*fp, "%s,", "pg_1v2_fpga_ddr");
    fprintf(*fp, "%s,", "pg_3v3_fpga_vcco");
    fprintf(*fp, "%s,", "pg_3v3_clock");
    fprintf(*fp, "%s,", "pg_1v8_clock");

    // valid fields
    fprintf(*fp, "%s,", "pg_3v3_fo_valid");
    fprintf(*fp, "%s,", "pg_0v9_gpu_core_valid");
    fprintf(*fp, "%s,", "pg_0v9_gpu_vddci_valid");
    fprintf(*fp, "%s,", "pg_1v35_gpu_vmeio_valid");
    fprintf(*fp, "%s,", "pg_1v8_gpu_valid");
    fprintf(*fp, "%s,", "pg_3v3_gpu_valid");
    fprintf(*fp, "%s,", "pg_0v85_fpga_vccint_valid");
    fprintf(*fp, "%s,", "pg_0v9_fpga_mgtavcc_valid");
    fprintf(*fp, "%s,", "pg_1v2_fpga_mgtavtt_valid");
    fprintf(*fp, "%s,", "pg_1v8_fpga_vcco_valid");
    fprintf(*fp, "%s,", "pg_1v8_fpga_mgtvccaux_valid");
    fprintf(*fp, "%s,", "pg_1v2_fpga_ddr_valid");
    fprintf(*fp, "%s,", "pg_3v3_fpga_vcco_valid");
    fprintf(*fp, "%s,", "pg_3v3_clock_valid");
    fprintf(*fp, "%s,", "pg_1v8_clock_valid");

    fprintf(*fp, "%s\\is_specified\\validity_region,", "tmp451_sensor_1");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "tmp451_sensor_2");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "tmp451_sensor_3");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "tmp451_sensor_4");

    fprintf(*fp, "%s\\is_specified\\validity_region,", "vols_3v3_fo");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_3v3_gpu");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_0v9_gpu_core");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_0v9_gpu_vddci");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_1v8_gpu");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_1v35_gpu_vmeio");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_0v85_fpga_vccint");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_0v9_fpga_mgtavcc");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_1v2_fpga_mgtavtt");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_3v3_fpga");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_1v2_fpga_ddr");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_1v8_fpga_mgtvccaux");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_1v8_fpga_vcco");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_3v3_clock");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_1v8_clock");

    fprintf(*fp, "%s\\is_specified\\validity_region,", "curs_3v3_fo");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "curs_5v0");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "curs_12v0");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "curs_3v3_gpu");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "curs_0v9_gpu_core");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "curs_0v9_gpu_vddci");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "curs_1v8_gpu");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "curs_1v35_gpu_vmeio");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "curs_0v85_fpga_vccint");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "curs_0v9_fpga_mgtavcc");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "curs_1v2_fpga_mgtavtt");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "curs_3v3_fpga_vcco");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "curs_1v2_fpga_ddr");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "curs_1v8_fpga_mgtvccaux");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "curs_1v8_fpga_vcco");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "curs_3v3_clock");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "curs_1v8_clock");

    // other good fields
    fprintf(*fp, "%s,", "lol_clk_gen");
    fprintf(*fp, "%s,", "fpga_init_3v3");
    fprintf(*fp, "%s,", "fpga_done_3v3");
    fprintf(*fp, "%s,", "los_clk_buf");
    fprintf(*fp, "%s,", "ready_ipmba");
    fprintf(*fp, "%s,", "ready_ipmbb");

    // other valid fields
    fprintf(*fp, "%s,", "lol_clk_gen_valid");
    fprintf(*fp, "%s,", "fpga_init_3v3_valid");
    fprintf(*fp, "%s,", "fpga_done_3v3_valid");
    fprintf(*fp, "%s,", "los_clk_buf_valid");
    fprintf(*fp, "%s,", "ready_ipmba_valid");
    fprintf(*fp, "%s\n", "ready_ipmbb_valid");
}

static int ipmc_gpm_log_parser(FILE *outfp, clcmsw_ipmc_gpm_data *ipmc_gpm)
{

    // Convert generic IPMC data fields if needed
    ipmc_gpm->generic_ipmc.LRM_dev = ntohl(ipmc_gpm->generic_ipmc.LRM_dev);
    ipmc_gpm->generic_ipmc.ipmb_a = ntohl(ipmc_gpm->generic_ipmc.ipmb_a);
    ipmc_gpm->generic_ipmc.ipmb_b = ntohl(ipmc_gpm->generic_ipmc.ipmb_b);
    ipmc_gpm->generic_ipmc.operation_mode = ntohl(ipmc_gpm->generic_ipmc.operation_mode);
    ipmc_gpm->generic_ipmc.sequence_number = ntohs(ipmc_gpm->generic_ipmc.sequence_number);
    ipmc_gpm->generic_ipmc.power_status = ntohl(ipmc_gpm->generic_ipmc.power_status);
    ipmc_gpm->generic_ipmc.reset_counter = ntohl(ipmc_gpm->generic_ipmc.reset_counter);
    ipmc_gpm->generic_ipmc.health_data_refresh_counter = ntohl(ipmc_gpm->generic_ipmc.health_data_refresh_counter);


    // Convert pgood fields (assuming they are stored as 32-bit values)
    ipmc_gpm->gpm_data.pg_3v3_fo = ntohl(ipmc_gpm->gpm_data.pg_3v3_fo);
    ipmc_gpm->gpm_data.pg_0v9_gpu_core = ntohl(ipmc_gpm->gpm_data.pg_0v9_gpu_core);
    ipmc_gpm->gpm_data.pg_0v9_gpu_vddci = ntohl(ipmc_gpm->gpm_data.pg_0v9_gpu_vddci);
    ipmc_gpm->gpm_data.pg_1v35_gpu_vmeio = ntohl(ipmc_gpm->gpm_data.pg_1v35_gpu_vmeio);
    ipmc_gpm->gpm_data.pg_1v8_gpu = ntohl(ipmc_gpm->gpm_data.pg_1v8_gpu);
    ipmc_gpm->gpm_data.pg_3v3_gpu = ntohl(ipmc_gpm->gpm_data.pg_3v3_gpu);
    ipmc_gpm->gpm_data.pg_0v85_fpga_vccint = ntohl(ipmc_gpm->gpm_data.pg_0v85_fpga_vccint);
    ipmc_gpm->gpm_data.pg_0v9_fpga_mgtavcc = ntohl(ipmc_gpm->gpm_data.pg_0v9_fpga_mgtavcc);
    ipmc_gpm->gpm_data.pg_1v2_fpga_mgtavtt = ntohl(ipmc_gpm->gpm_data.pg_1v2_fpga_mgtavtt);
    ipmc_gpm->gpm_data.pg_1v8_fpga_vcco = ntohl(ipmc_gpm->gpm_data.pg_1v8_fpga_vcco);
    ipmc_gpm->gpm_data.pg_1v8_fpga_mgtvccaux = ntohl(ipmc_gpm->gpm_data.pg_1v8_fpga_mgtvccaux);
    ipmc_gpm->gpm_data.pg_1v2_fpga_ddr = ntohl(ipmc_gpm->gpm_data.pg_1v2_fpga_ddr);
    ipmc_gpm->gpm_data.pg_3v3_fpga_vcco = ntohl(ipmc_gpm->gpm_data.pg_3v3_fpga_vcco);
    ipmc_gpm->gpm_data.pg_3v3_clock = ntohl(ipmc_gpm->gpm_data.pg_3v3_clock);
    ipmc_gpm->gpm_data.pg_1v8_clock = ntohl(ipmc_gpm->gpm_data.pg_1v8_clock);


    // Valid types
    ipmc_gpm->gpm_data.pg_3v3_fo_valid = ntohs(ipmc_gpm->gpm_data.pg_3v3_fo_valid);
    ipmc_gpm->gpm_data.pg_0v9_gpu_core_valid = ntohs(ipmc_gpm->gpm_data.pg_0v9_gpu_core_valid);
    ipmc_gpm->gpm_data.pg_0v9_gpu_vddci_valid = ntohs(ipmc_gpm->gpm_data.pg_0v9_gpu_vddci_valid);
    ipmc_gpm->gpm_data.pg_1v35_gpu_vmeio_valid = ntohs(ipmc_gpm->gpm_data.pg_1v35_gpu_vmeio_valid);
    ipmc_gpm->gpm_data.pg_1v8_gpu_valid = ntohs(ipmc_gpm->gpm_data.pg_1v8_gpu_valid);
    ipmc_gpm->gpm_data.pg_3v3_gpu_valid = ntohs(ipmc_gpm->gpm_data.pg_3v3_gpu_valid);
    ipmc_gpm->gpm_data.pg_0v85_fpga_vccint_valid = ntohs(ipmc_gpm->gpm_data.pg_0v85_fpga_vccint_valid);
    ipmc_gpm->gpm_data.pg_0v9_fpga_mgtavcc_valid = ntohs(ipmc_gpm->gpm_data.pg_0v9_fpga_mgtavcc_valid);
    ipmc_gpm->gpm_data.pg_1v2_fpga_mgtavtt_valid = ntohs(ipmc_gpm->gpm_data.pg_1v2_fpga_mgtavtt_valid);
    ipmc_gpm->gpm_data.pg_1v8_fpga_vcco_valid = ntohs(ipmc_gpm->gpm_data.pg_1v8_fpga_vcco_valid);
    ipmc_gpm->gpm_data.pg_1v8_fpga_mgtvccaux_valid = ntohs(ipmc_gpm->gpm_data.pg_1v8_fpga_mgtvccaux_valid);
    ipmc_gpm->gpm_data.pg_1v2_fpga_ddr_valid = ntohs(ipmc_gpm->gpm_data.pg_1v2_fpga_ddr_valid);
    ipmc_gpm->gpm_data.pg_3v3_fpga_vcco_valid = ntohs(ipmc_gpm->gpm_data.pg_3v3_fpga_vcco_valid);
    ipmc_gpm->gpm_data.pg_3v3_clock_valid = ntohs(ipmc_gpm->gpm_data.pg_3v3_clock_valid);
    ipmc_gpm->gpm_data.pg_1v8_clock_valid = ntohs(ipmc_gpm->gpm_data.pg_1v8_clock_valid);

    // Process signed voltage values (Pcs_ipmc_signed_value_type)
    ipmc_gpm->gpm_data.tmp451_sensor_1.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.tmp451_sensor_1.limit_status.is_specified);
    ipmc_gpm->gpm_data.tmp451_sensor_1.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.tmp451_sensor_1.limit_status.validity_region);
    ipmc_gpm->gpm_data.tmp451_sensor_1.value = ntohs(ipmc_gpm->gpm_data.tmp451_sensor_1.value);

    ipmc_gpm->gpm_data.tmp451_sensor_2.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.tmp451_sensor_2.limit_status.is_specified);
    ipmc_gpm->gpm_data.tmp451_sensor_2.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.tmp451_sensor_2.limit_status.validity_region);
    ipmc_gpm->gpm_data.tmp451_sensor_2.value = ntohs(ipmc_gpm->gpm_data.tmp451_sensor_2.value);

    ipmc_gpm->gpm_data.tmp451_sensor_3.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.tmp451_sensor_3.limit_status.is_specified);
    ipmc_gpm->gpm_data.tmp451_sensor_3.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.tmp451_sensor_3.limit_status.validity_region);
    ipmc_gpm->gpm_data.tmp451_sensor_3.value = ntohs(ipmc_gpm->gpm_data.tmp451_sensor_3.value);

    ipmc_gpm->gpm_data.tmp451_sensor_4.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.tmp451_sensor_4.limit_status.is_specified);
    ipmc_gpm->gpm_data.tmp451_sensor_4.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.tmp451_sensor_4.limit_status.validity_region);
    ipmc_gpm->gpm_data.tmp451_sensor_4.value = ntohs(ipmc_gpm->gpm_data.tmp451_sensor_4.value);

    // Voltage values (Pcs_ipmc_unsigned_value_type)
    ipmc_gpm->gpm_data.vols_3v3_fo.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.vols_3v3_fo.limit_status.is_specified);
    ipmc_gpm->gpm_data.vols_3v3_fo.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.vols_3v3_fo.limit_status.validity_region);
    ipmc_gpm->gpm_data.vols_3v3_fo.value = ntohs(ipmc_gpm->gpm_data.vols_3v3_fo.value);

    ipmc_gpm->gpm_data.plus_3v3_gpu.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.plus_3v3_gpu.limit_status.is_specified);
    ipmc_gpm->gpm_data.plus_3v3_gpu.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.plus_3v3_gpu.limit_status.validity_region);
    ipmc_gpm->gpm_data.plus_3v3_gpu.value = ntohs(ipmc_gpm->gpm_data.plus_3v3_gpu.value);

    ipmc_gpm->gpm_data.plus_0v9_gpu_core.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.plus_0v9_gpu_core.limit_status.is_specified);
    ipmc_gpm->gpm_data.plus_0v9_gpu_core.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.plus_0v9_gpu_core.limit_status.validity_region);
    ipmc_gpm->gpm_data.plus_0v9_gpu_core.value = ntohs(ipmc_gpm->gpm_data.plus_0v9_gpu_core.value);

    ipmc_gpm->gpm_data.plus_0v9_gpu_vddci.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.plus_0v9_gpu_vddci.limit_status.is_specified);
    ipmc_gpm->gpm_data.plus_0v9_gpu_vddci.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.plus_0v9_gpu_vddci.limit_status.validity_region);
    ipmc_gpm->gpm_data.plus_0v9_gpu_vddci.value = ntohs(ipmc_gpm->gpm_data.plus_0v9_gpu_vddci.value);

    ipmc_gpm->gpm_data.plus_1v8_gpu.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.plus_1v8_gpu.limit_status.is_specified);
    ipmc_gpm->gpm_data.plus_1v8_gpu.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.plus_1v8_gpu.limit_status.validity_region);
    ipmc_gpm->gpm_data.plus_1v8_gpu.value = ntohs(ipmc_gpm->gpm_data.plus_1v8_gpu.value);

    ipmc_gpm->gpm_data.plus_1v35_gpu_vmeio.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.plus_1v35_gpu_vmeio.limit_status.is_specified);
    ipmc_gpm->gpm_data.plus_1v35_gpu_vmeio.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.plus_1v35_gpu_vmeio.limit_status.validity_region);
    ipmc_gpm->gpm_data.plus_1v35_gpu_vmeio.value = ntohs(ipmc_gpm->gpm_data.plus_1v35_gpu_vmeio.value);

    ipmc_gpm->gpm_data.plus_0v85_fpga_vccint.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.plus_0v85_fpga_vccint.limit_status.is_specified);
    ipmc_gpm->gpm_data.plus_0v85_fpga_vccint.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.plus_0v85_fpga_vccint.limit_status.validity_region);
    ipmc_gpm->gpm_data.plus_0v85_fpga_vccint.value = ntohs(ipmc_gpm->gpm_data.plus_0v85_fpga_vccint.value);

    ipmc_gpm->gpm_data.plus_0v9_fpga_mgtavcc.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.plus_0v9_fpga_mgtavcc.limit_status.is_specified);
    ipmc_gpm->gpm_data.plus_0v9_fpga_mgtavcc.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.plus_0v9_fpga_mgtavcc.limit_status.validity_region);
    ipmc_gpm->gpm_data.plus_0v9_fpga_mgtavcc.value = ntohs(ipmc_gpm->gpm_data.plus_0v9_fpga_mgtavcc.value);

    ipmc_gpm->gpm_data.plus_1v2_fpga_mgtavtt.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.plus_1v2_fpga_mgtavtt.limit_status.is_specified);
    ipmc_gpm->gpm_data.plus_1v2_fpga_mgtavtt.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.plus_1v2_fpga_mgtavtt.limit_status.validity_region);
    ipmc_gpm->gpm_data.plus_1v2_fpga_mgtavtt.value = ntohs(ipmc_gpm->gpm_data.plus_1v2_fpga_mgtavtt.value);

    ipmc_gpm->gpm_data.plus_3v3_fpga.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.plus_3v3_fpga.limit_status.is_specified);
    ipmc_gpm->gpm_data.plus_3v3_fpga.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.plus_3v3_fpga.limit_status.validity_region);
    ipmc_gpm->gpm_data.plus_3v3_fpga.value = ntohs(ipmc_gpm->gpm_data.plus_3v3_fpga.value);

    ipmc_gpm->gpm_data.plus_1v2_fpga_ddr.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.plus_1v2_fpga_ddr.limit_status.is_specified);
    ipmc_gpm->gpm_data.plus_1v2_fpga_ddr.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.plus_1v2_fpga_ddr.limit_status.validity_region);
    ipmc_gpm->gpm_data.plus_1v2_fpga_ddr.value = ntohs(ipmc_gpm->gpm_data.plus_1v2_fpga_ddr.value);

    ipmc_gpm->gpm_data.plus_1v8_fpga_mgtvccaux.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.plus_1v8_fpga_mgtvccaux.limit_status.is_specified);
    ipmc_gpm->gpm_data.plus_1v8_fpga_mgtvccaux.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.plus_1v8_fpga_mgtvccaux.limit_status.validity_region);
    ipmc_gpm->gpm_data.plus_1v8_fpga_mgtvccaux.value = ntohs(ipmc_gpm->gpm_data.plus_1v8_fpga_mgtvccaux.value);

    ipmc_gpm->gpm_data.plus_1v8_fpga_vcco.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.plus_1v8_fpga_vcco.limit_status.is_specified);
    ipmc_gpm->gpm_data.plus_1v8_fpga_vcco.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.plus_1v8_fpga_vcco.limit_status.validity_region);
    ipmc_gpm->gpm_data.plus_1v8_fpga_vcco.value = ntohs(ipmc_gpm->gpm_data.plus_1v8_fpga_vcco.value);

    ipmc_gpm->gpm_data.plus_3v3_clock.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.plus_3v3_clock.limit_status.is_specified);
    ipmc_gpm->gpm_data.plus_3v3_clock.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.plus_3v3_clock.limit_status.validity_region);
    ipmc_gpm->gpm_data.plus_3v3_clock.value = ntohs(ipmc_gpm->gpm_data.plus_3v3_clock.value);

    ipmc_gpm->gpm_data.plus_1v8_clock.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.plus_1v8_clock.limit_status.is_specified);
    ipmc_gpm->gpm_data.plus_1v8_clock.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.plus_1v8_clock.limit_status.validity_region);
    ipmc_gpm->gpm_data.plus_1v8_clock.value = ntohs(ipmc_gpm->gpm_data.plus_1v8_clock.value);

    // Current values (Pcs_ipmc_unsigned_value_type)
    ipmc_gpm->gpm_data.curs_3v3_fo.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.curs_3v3_fo.limit_status.is_specified);
    ipmc_gpm->gpm_data.curs_3v3_fo.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.curs_3v3_fo.limit_status.validity_region);
    ipmc_gpm->gpm_data.curs_3v3_fo.value = ntohs(ipmc_gpm->gpm_data.curs_3v3_fo.value);

    ipmc_gpm->gpm_data.curs_5v0.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.curs_5v0.limit_status.is_specified);
    ipmc_gpm->gpm_data.curs_5v0.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.curs_5v0.limit_status.validity_region);
    ipmc_gpm->gpm_data.curs_5v0.value = ntohs(ipmc_gpm->gpm_data.curs_5v0.value);

    ipmc_gpm->gpm_data.curs_12v0.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.curs_12v0.limit_status.is_specified);
    ipmc_gpm->gpm_data.curs_12v0.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.curs_12v0.limit_status.validity_region);
    ipmc_gpm->gpm_data.curs_12v0.value = ntohs(ipmc_gpm->gpm_data.curs_12v0.value);

    ipmc_gpm->gpm_data.curs_3v3_gpu.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.curs_3v3_gpu.limit_status.is_specified);
    ipmc_gpm->gpm_data.curs_3v3_gpu.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.curs_3v3_gpu.limit_status.validity_region);
    ipmc_gpm->gpm_data.curs_3v3_gpu.value = ntohs(ipmc_gpm->gpm_data.curs_3v3_gpu.value);

    ipmc_gpm->gpm_data.curs_0v9_gpu_core.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.curs_0v9_gpu_core.limit_status.is_specified);
    ipmc_gpm->gpm_data.curs_0v9_gpu_core.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.curs_0v9_gpu_core.limit_status.validity_region);
    ipmc_gpm->gpm_data.curs_0v9_gpu_core.value = ntohs(ipmc_gpm->gpm_data.curs_0v9_gpu_core.value);

    ipmc_gpm->gpm_data.curs_0v9_gpu_vddci.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.curs_0v9_gpu_vddci.limit_status.is_specified);
    ipmc_gpm->gpm_data.curs_0v9_gpu_vddci.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.curs_0v9_gpu_vddci.limit_status.validity_region);
    ipmc_gpm->gpm_data.curs_0v9_gpu_vddci.value = ntohs(ipmc_gpm->gpm_data.curs_0v9_gpu_vddci.value);

    ipmc_gpm->gpm_data.curs_1v8_gpu.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.curs_1v8_gpu.limit_status.is_specified);
    ipmc_gpm->gpm_data.curs_1v8_gpu.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.curs_1v8_gpu.limit_status.validity_region);
    ipmc_gpm->gpm_data.curs_1v8_gpu.value = ntohs(ipmc_gpm->gpm_data.curs_1v8_gpu.value);

    ipmc_gpm->gpm_data.curs_1v35_gpu_vmeio.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.curs_1v35_gpu_vmeio.limit_status.is_specified);
    ipmc_gpm->gpm_data.curs_1v35_gpu_vmeio.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.curs_1v35_gpu_vmeio.limit_status.validity_region);
    ipmc_gpm->gpm_data.curs_1v35_gpu_vmeio.value = ntohs(ipmc_gpm->gpm_data.curs_1v35_gpu_vmeio.value);

    ipmc_gpm->gpm_data.curs_0v85_fpga_vccint.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.curs_0v85_fpga_vccint.limit_status.is_specified);
    ipmc_gpm->gpm_data.curs_0v85_fpga_vccint.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.curs_0v85_fpga_vccint.limit_status.validity_region);
    ipmc_gpm->gpm_data.curs_0v85_fpga_vccint.value = ntohs(ipmc_gpm->gpm_data.curs_0v85_fpga_vccint.value);

    ipmc_gpm->gpm_data.curs_0v9_fpga_mgtavcc.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.curs_0v9_fpga_mgtavcc.limit_status.is_specified);
    ipmc_gpm->gpm_data.curs_0v9_fpga_mgtavcc.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.curs_0v9_fpga_mgtavcc.limit_status.validity_region);
    ipmc_gpm->gpm_data.curs_0v9_fpga_mgtavcc.value = ntohs(ipmc_gpm->gpm_data.curs_0v9_fpga_mgtavcc.value);

    ipmc_gpm->gpm_data.curs_1v2_fpga_mgtavtt.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.curs_1v2_fpga_mgtavtt.limit_status.is_specified);
    ipmc_gpm->gpm_data.curs_1v2_fpga_mgtavtt.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.curs_1v2_fpga_mgtavtt.limit_status.validity_region);
    ipmc_gpm->gpm_data.curs_1v2_fpga_mgtavtt.value = ntohs(ipmc_gpm->gpm_data.curs_1v2_fpga_mgtavtt.value);

    ipmc_gpm->gpm_data.curs_3v3_fpga_vcco.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.curs_3v3_fpga_vcco.limit_status.is_specified);
    ipmc_gpm->gpm_data.curs_3v3_fpga_vcco.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.curs_3v3_fpga_vcco.limit_status.validity_region);
    ipmc_gpm->gpm_data.curs_3v3_fpga_vcco.value = ntohs(ipmc_gpm->gpm_data.curs_3v3_fpga_vcco.value);

    ipmc_gpm->gpm_data.curs_1v2_fpga_ddr.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.curs_1v2_fpga_ddr.limit_status.is_specified);
    ipmc_gpm->gpm_data.curs_1v2_fpga_ddr.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.curs_1v2_fpga_ddr.limit_status.validity_region);
    ipmc_gpm->gpm_data.curs_1v2_fpga_ddr.value = ntohs(ipmc_gpm->gpm_data.curs_1v2_fpga_ddr.value);

    ipmc_gpm->gpm_data.curs_1v8_fpga_mgtvccaux.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.curs_1v8_fpga_mgtvccaux.limit_status.is_specified);
    ipmc_gpm->gpm_data.curs_1v8_fpga_mgtvccaux.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.curs_1v8_fpga_mgtvccaux.limit_status.validity_region);
    ipmc_gpm->gpm_data.curs_1v8_fpga_mgtvccaux.value = ntohs(ipmc_gpm->gpm_data.curs_1v8_fpga_mgtvccaux.value);

    ipmc_gpm->gpm_data.curs_1v8_fpga_vcco.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.curs_1v8_fpga_vcco.limit_status.is_specified);
    ipmc_gpm->gpm_data.curs_1v8_fpga_vcco.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.curs_1v8_fpga_vcco.limit_status.validity_region);
    ipmc_gpm->gpm_data.curs_1v8_fpga_vcco.value = ntohs(ipmc_gpm->gpm_data.curs_1v8_fpga_vcco.value);

    ipmc_gpm->gpm_data.curs_3v3_clock.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.curs_3v3_clock.limit_status.is_specified);
    ipmc_gpm->gpm_data.curs_3v3_clock.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.curs_3v3_clock.limit_status.validity_region);
    ipmc_gpm->gpm_data.curs_3v3_clock.value = ntohs(ipmc_gpm->gpm_data.curs_3v3_clock.value);

    ipmc_gpm->gpm_data.curs_1v8_clock.limit_status.is_specified = ntohl(ipmc_gpm->gpm_data.curs_1v8_clock.limit_status.is_specified);
    ipmc_gpm->gpm_data.curs_1v8_clock.limit_status.validity_region = ntohl(ipmc_gpm->gpm_data.curs_1v8_clock.limit_status.validity_region);
    ipmc_gpm->gpm_data.curs_1v8_clock.value = ntohs(ipmc_gpm->gpm_data.curs_1v8_clock.value);

        // Other good types
    ipmc_gpm->gpm_data.lol_clk_gen = ntohs(ipmc_gpm->gpm_data.lol_clk_gen);
    ipmc_gpm->gpm_data.fpga_init_3v3 = ntohs(ipmc_gpm->gpm_data.fpga_init_3v3);
    ipmc_gpm->gpm_data.fpga_done_3v3 = ntohs(ipmc_gpm->gpm_data.fpga_done_3v3);
    ipmc_gpm->gpm_data.los_clk_buf = ntohs(ipmc_gpm->gpm_data.los_clk_buf);
    ipmc_gpm->gpm_data.ready_ipmba = ntohs(ipmc_gpm->gpm_data.ready_ipmba);
    ipmc_gpm->gpm_data.ready_ipmbb = ntohs(ipmc_gpm->gpm_data.ready_ipmbb);

    // Other valid types
    ipmc_gpm->gpm_data.lol_clk_gen_valid = ntohs(ipmc_gpm->gpm_data.lol_clk_gen_valid);
    ipmc_gpm->gpm_data.fpga_init_3v3_valid = ntohs(ipmc_gpm->gpm_data.fpga_init_3v3_valid);
    ipmc_gpm->gpm_data.fpga_done_3v3_valid = ntohs(ipmc_gpm->gpm_data.fpga_done_3v3_valid);
    ipmc_gpm->gpm_data.los_clk_buf_valid = ntohs(ipmc_gpm->gpm_data.los_clk_buf_valid);
    ipmc_gpm->gpm_data.ready_ipmba_valid = ntohs(ipmc_gpm->gpm_data.ready_ipmba_valid);
    ipmc_gpm->gpm_data.ready_ipmbb_valid = ntohs(ipmc_gpm->gpm_data.ready_ipmbb_valid);

    fprintf(outfp, "%d,", ipmc_gpm->generic_ipmc.LRM_dev);
    fprintf(outfp, "%u,", ipmc_gpm->generic_ipmc.is_auth);
    fprintf(outfp, "%u,", ipmc_gpm->generic_ipmc.ipmb_a);
    fprintf(outfp, "%u,", ipmc_gpm->generic_ipmc.ipmb_b);
    fprintf(outfp, "%u.%u.%u,", ipmc_gpm->generic_ipmc.fw_version_major,
            ipmc_gpm->generic_ipmc.fw_version_minor,
            ipmc_gpm->generic_ipmc.fw_version_patch);
    fprintf(outfp, "%u,", ipmc_gpm->generic_ipmc.operation_mode);
    fprintf(outfp, "%u,", ipmc_gpm->generic_ipmc.sequence_number);
    fprintf(outfp, "%u,", ipmc_gpm->generic_ipmc.power_status);
    fprintf(outfp, "%u,", ipmc_gpm->generic_ipmc.reset_counter);
    fprintf(outfp, "%u,", ipmc_gpm->generic_ipmc.health_data_refresh_counter);

    /* pgood */
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_3v3_fo);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_0v9_gpu_core);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_0v9_gpu_vddci);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_1v35_gpu_vmeio);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_1v8_gpu);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_3v3_gpu);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_0v85_fpga_vccint);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_0v9_fpga_mgtavcc);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_1v2_fpga_mgtavtt);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_1v8_fpga_vcco);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_1v8_fpga_mgtvccaux);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_1v2_fpga_ddr);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_3v3_fpga_vcco);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_3v3_clock);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_1v8_clock);

    /* valid */
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_3v3_fo_valid);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_0v9_gpu_core_valid);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_0v9_gpu_vddci_valid);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_1v35_gpu_vmeio_valid);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_1v8_gpu_valid);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_3v3_gpu_valid);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_0v85_fpga_vccint_valid);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_0v9_fpga_mgtavcc_valid);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_1v2_fpga_mgtavtt_valid);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_1v8_fpga_vcco_valid);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_1v8_fpga_mgtvccaux_valid);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_1v2_fpga_ddr_valid);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_3v3_fpga_vcco_valid);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_3v3_clock_valid);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.pg_1v8_clock_valid);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.tmp451_sensor_1.value,
        ipmc_gpm->gpm_data.tmp451_sensor_1.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.tmp451_sensor_1.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.tmp451_sensor_1.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.tmp451_sensor_1.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.tmp451_sensor_2.value,
        ipmc_gpm->gpm_data.tmp451_sensor_2.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.tmp451_sensor_2.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.tmp451_sensor_2.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.tmp451_sensor_2.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.tmp451_sensor_3.value,
        ipmc_gpm->gpm_data.tmp451_sensor_3.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.tmp451_sensor_3.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.tmp451_sensor_3.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.tmp451_sensor_3.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.tmp451_sensor_4.value,
        ipmc_gpm->gpm_data.tmp451_sensor_4.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.tmp451_sensor_4.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.tmp451_sensor_4.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.tmp451_sensor_4.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.vols_3v3_fo.value,
        ipmc_gpm->gpm_data.vols_3v3_fo.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.vols_3v3_fo.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.vols_3v3_fo.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.vols_3v3_fo.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.plus_3v3_gpu.value,
        ipmc_gpm->gpm_data.plus_3v3_gpu.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.plus_3v3_gpu.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.plus_3v3_gpu.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.plus_3v3_gpu.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.plus_0v9_gpu_core.value,
        ipmc_gpm->gpm_data.plus_0v9_gpu_core.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.plus_0v9_gpu_core.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.plus_0v9_gpu_core.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.plus_0v9_gpu_core.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.plus_0v9_gpu_vddci.value,
        ipmc_gpm->gpm_data.plus_0v9_gpu_vddci.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.plus_0v9_gpu_vddci.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.plus_0v9_gpu_vddci.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.plus_0v9_gpu_vddci.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.plus_1v8_gpu.value,
        ipmc_gpm->gpm_data.plus_1v8_gpu.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.plus_1v8_gpu.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.plus_1v8_gpu.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.plus_1v8_gpu.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.plus_1v35_gpu_vmeio.value,
        ipmc_gpm->gpm_data.plus_1v35_gpu_vmeio.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.plus_1v35_gpu_vmeio.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.plus_1v35_gpu_vmeio.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.plus_1v35_gpu_vmeio.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.plus_0v85_fpga_vccint.value,
        ipmc_gpm->gpm_data.plus_0v85_fpga_vccint.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.plus_0v85_fpga_vccint.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.plus_0v85_fpga_vccint.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.plus_0v85_fpga_vccint.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.plus_0v9_fpga_mgtavcc.value,
        ipmc_gpm->gpm_data.plus_0v9_fpga_mgtavcc.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.plus_0v9_fpga_mgtavcc.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.plus_0v9_fpga_mgtavcc.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.plus_0v9_fpga_mgtavcc.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.plus_1v2_fpga_mgtavtt.value,
        ipmc_gpm->gpm_data.plus_1v2_fpga_mgtavtt.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.plus_1v2_fpga_mgtavtt.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.plus_1v2_fpga_mgtavtt.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.plus_1v2_fpga_mgtavtt.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.plus_3v3_fpga.value,
        ipmc_gpm->gpm_data.plus_3v3_fpga.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.plus_3v3_fpga.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.plus_3v3_fpga.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.plus_3v3_fpga.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.plus_1v2_fpga_ddr.value,
        ipmc_gpm->gpm_data.plus_1v2_fpga_ddr.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.plus_1v2_fpga_ddr.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.plus_1v2_fpga_ddr.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.plus_1v2_fpga_ddr.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.plus_1v8_fpga_mgtvccaux.value,
        ipmc_gpm->gpm_data.plus_1v8_fpga_mgtvccaux.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.plus_1v8_fpga_mgtvccaux.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.plus_1v8_fpga_mgtvccaux.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.plus_1v8_fpga_mgtvccaux.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.plus_1v8_fpga_vcco.value,
        ipmc_gpm->gpm_data.plus_1v8_fpga_vcco.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.plus_1v8_fpga_vcco.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.plus_1v8_fpga_vcco.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.plus_1v8_fpga_vcco.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.plus_3v3_clock.value,
        ipmc_gpm->gpm_data.plus_3v3_clock.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.plus_3v3_clock.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.plus_3v3_clock.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.plus_3v3_clock.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.plus_1v8_clock.value,
        ipmc_gpm->gpm_data.plus_1v8_clock.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.plus_1v8_clock.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.plus_1v8_clock.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.plus_1v8_clock.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.curs_3v3_fo.value,
        ipmc_gpm->gpm_data.curs_3v3_fo.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.curs_3v3_fo.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.curs_3v3_fo.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.curs_3v3_fo.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.curs_5v0.value,
        ipmc_gpm->gpm_data.curs_5v0.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.curs_5v0.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.curs_5v0.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.curs_5v0.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.curs_12v0.value,
        ipmc_gpm->gpm_data.curs_12v0.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.curs_12v0.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.curs_12v0.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.curs_12v0.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.curs_3v3_gpu.value,
        ipmc_gpm->gpm_data.curs_3v3_gpu.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.curs_3v3_gpu.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.curs_3v3_gpu.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.curs_3v3_gpu.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.curs_0v9_gpu_core.value,
        ipmc_gpm->gpm_data.curs_0v9_gpu_core.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.curs_0v9_gpu_core.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.curs_0v9_gpu_core.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.curs_0v9_gpu_core.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.curs_0v9_gpu_vddci.value,
        ipmc_gpm->gpm_data.curs_0v9_gpu_vddci.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.curs_0v9_gpu_vddci.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.curs_0v9_gpu_vddci.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.curs_0v9_gpu_vddci.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.curs_1v8_gpu.value,
        ipmc_gpm->gpm_data.curs_1v8_gpu.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.curs_1v8_gpu.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.curs_1v8_gpu.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.curs_1v8_gpu.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.curs_1v35_gpu_vmeio.value,
        ipmc_gpm->gpm_data.curs_1v35_gpu_vmeio.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.curs_1v35_gpu_vmeio.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.curs_1v35_gpu_vmeio.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.curs_1v35_gpu_vmeio.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.curs_0v85_fpga_vccint.value,
        ipmc_gpm->gpm_data.curs_0v85_fpga_vccint.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.curs_0v85_fpga_vccint.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.curs_0v85_fpga_vccint.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.curs_0v85_fpga_vccint.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.curs_0v9_fpga_mgtavcc.value,
        ipmc_gpm->gpm_data.curs_0v9_fpga_mgtavcc.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.curs_0v9_fpga_mgtavcc.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.curs_0v9_fpga_mgtavcc.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.curs_0v9_fpga_mgtavcc.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.curs_1v2_fpga_mgtavtt.value,
        ipmc_gpm->gpm_data.curs_1v2_fpga_mgtavtt.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.curs_1v2_fpga_mgtavtt.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.curs_1v2_fpga_mgtavtt.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.curs_1v2_fpga_mgtavtt.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.curs_3v3_fpga_vcco.value,
        ipmc_gpm->gpm_data.curs_3v3_fpga_vcco.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.curs_3v3_fpga_vcco.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.curs_3v3_fpga_vcco.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.curs_3v3_fpga_vcco.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.curs_1v2_fpga_ddr.value,
        ipmc_gpm->gpm_data.curs_1v2_fpga_ddr.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.curs_1v2_fpga_ddr.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.curs_1v2_fpga_ddr.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.curs_1v2_fpga_ddr.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.curs_1v8_fpga_mgtvccaux.value,
        ipmc_gpm->gpm_data.curs_1v8_fpga_mgtvccaux.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.curs_1v8_fpga_mgtvccaux.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.curs_1v8_fpga_mgtvccaux.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.curs_1v8_fpga_mgtvccaux.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.curs_1v8_fpga_vcco.value,
        ipmc_gpm->gpm_data.curs_1v8_fpga_vcco.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.curs_1v8_fpga_vcco.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.curs_1v8_fpga_vcco.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.curs_1v8_fpga_vcco.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.curs_3v3_clock.value,
        ipmc_gpm->gpm_data.curs_3v3_clock.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.curs_3v3_clock.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.curs_3v3_clock.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.curs_3v3_clock.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_gpm->gpm_data.curs_1v8_clock.value,
        ipmc_gpm->gpm_data.curs_1v8_clock.limit_status.is_specified < 2 ? 
            is_specified[ipmc_gpm->gpm_data.curs_1v8_clock.limit_status.is_specified] : 
            is_specified[2],
        ipmc_gpm->gpm_data.curs_1v8_clock.limit_status.validity_region < 4 ? 
            validity_region[ipmc_gpm->gpm_data.curs_1v8_clock.limit_status.validity_region] : 
            validity_region[4]);

    /* other */
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.lol_clk_gen);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.fpga_init_3v3);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.fpga_done_3v3);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.los_clk_buf);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.ready_ipmba);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.ready_ipmbb);

    /* other valid*/
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.lol_clk_gen_valid);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.fpga_init_3v3_valid);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.fpga_done_3v3_valid);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.los_clk_buf_valid);
    fprintf(outfp, "%u,", ipmc_gpm->gpm_data.ready_ipmba_valid);
    fprintf(outfp, "%u\n", ipmc_gpm->gpm_data.ready_ipmbb_valid);


    return 0;
}
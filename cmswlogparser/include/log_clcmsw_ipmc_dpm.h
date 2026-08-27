#pragma once
typedef struct __attribute__((packed)) pcs_dpm_board_data
{
    /* pgood */
    pcs_good_type dtn_es_1v0_pg;
    pcs_good_type dtn_es_1v2_pg;
    pcs_good_type dtn_es_vdda_1v_pg;
    pcs_good_type tps_pgood_dtn_es_vcc3v3;
    pcs_good_type dtn_es_vddix_3v3_pgood;
    pcs_good_type dtn_es_2v5_pg;
    pcs_good_type dtn_es_1v8_pg;
    pcs_good_type dtn_es_vddi_1v8_pg;
    pcs_good_type phy_pgood_dvdd_1v;
    pcs_good_type vtt_vref_pg;
    pcs_good_type tps_pgood_cpu_s1vdd;
    pcs_good_type tps_pgood_cpu_s2vdd;
    pcs_good_type dtn_es_vddi_2v5_pg;
    pcs_good_type bias_pgood_5v;
    pcs_good_type tpsm_pgood_cpu_vcc1v35;
    pcs_good_type tpsm_pgood_cpu_vcc1v8;
    pcs_good_type prog_sfp_pgood;
    pcs_good_type tpsm_pgood_cpu_vcc1v2;

    /* valid */
    pcs_valid_type dtn_es_1v0_pg_valid;
    pcs_valid_type dtn_es_1v2_pg_valid;
    pcs_valid_type dtn_es_vdda_1v_pg_valid;
    pcs_valid_type tps_pgood_dtn_es_vcc3v3_valid;
    pcs_valid_type dtn_es_vddix_3v3_pgood_valid;
    pcs_valid_type dtn_es_2v5_pg_valid;
    pcs_valid_type dtn_es_1v8_pg_valid;
    pcs_valid_type dtn_es_vddi_1v8_pg_valid;
    pcs_valid_type phy_pgood_dvdd_1v_valid;
    pcs_valid_type vtt_vref_pg_valid;
    pcs_valid_type tps_pgood_cpu_s1vdd_valid;
    pcs_valid_type tps_pgood_cpu_s2vdd_valid;
    pcs_valid_type dtn_es_vddi_2v5_pg_valid;
    pcs_valid_type bias_pgood_5v_valid;
    pcs_valid_type tpsm_pgood_cpu_vcc1v35_valid;
    pcs_valid_type tpsm_pgood_cpu_vcc1v8_valid;
    pcs_valid_type prog_sfp_pgood_valid;
    pcs_valid_type tpsm_pgood_cpu_vcc1v2_valid;

    /* temperatures */
    Pcs_ipmc_signed_value_type tmp451_sensor_for_cpu;
    Pcs_ipmc_signed_value_type tmp451_sensor_for_fpga;

    /* current */
    Pcs_ipmc_unsigned_value_type dtn_es_1v0_cs;
    Pcs_ipmc_unsigned_value_type dtn_es_vddi_2v5_cs;
    Pcs_ipmc_unsigned_value_type dtn_es_vdda_1v_cs;
    Pcs_ipmc_unsigned_value_type dtn_es_vcc3v3_cs;
    Pcs_ipmc_unsigned_value_type dtn_es_2v5_cs;
    Pcs_ipmc_unsigned_value_type dtn_es_vdd_1v8_cs;
    Pcs_ipmc_unsigned_value_type dtn_es_vddi_1v8_cs;
    Pcs_ipmc_unsigned_value_type cpu_1v2_cs;
    Pcs_ipmc_unsigned_value_type cpu_1v8_cs;
    Pcs_ipmc_unsigned_value_type cpu_1v35_cs;
    Pcs_ipmc_unsigned_value_type cpu_s1vdd_cs;
    Pcs_ipmc_unsigned_value_type cpu_s2vdd_cs;
    Pcs_ipmc_unsigned_value_type vcore_imon;

    /* voltage */
    Pcs_ipmc_unsigned_value_type cpu_1v8;
    Pcs_ipmc_unsigned_value_type cpu_1v35;
    Pcs_ipmc_unsigned_value_type cpu_s1vdd;
    Pcs_ipmc_unsigned_value_type cpu_s2vdd;
    Pcs_ipmc_unsigned_value_type dtn_es_1v0;
    Pcs_ipmc_unsigned_value_type dtn_es_vddi_2v5;
    Pcs_ipmc_unsigned_value_type dtn_es_vdda_1v;
    Pcs_ipmc_unsigned_value_type dtn_es_vcc3v3;
    Pcs_ipmc_unsigned_value_type dtn_es_2v5;
    Pcs_ipmc_unsigned_value_type dtn_es_vdd_1v8;
    Pcs_ipmc_unsigned_value_type dtn_es_vddi_1v8;
    Pcs_ipmc_unsigned_value_type cpu_1v2;


    /* other */
    pcs_good_type ipmbb_ready;
    pcs_good_type ipmba_ready;
    pcs_good_type clk_los;
    pcs_good_type clk_gen_lol;
    pcs_good_type mcu_asleep;
    pcs_good_type vcore_rdy;
    pcs_good_type vcore_fault;
    pcs_good_type vcore_temp;

    pcs_good_type ddr_test;
    pcs_good_type serdes1_pll1_test;
    pcs_good_type serdes2_pll1_test;
    pcs_good_type serdes2_pll2_test;
    pcs_good_type ifc_nand_test;

    /* other valid */
    pcs_valid_type ipmbb_ready_valid;
    pcs_valid_type ipmba_ready_valid;
    pcs_valid_type clk_los_valid;
    pcs_valid_type clk_gen_lol_valid;
    pcs_valid_type mcu_asleep_valid;
    pcs_valid_type vcore_rdy_valid;
    pcs_valid_type vcore_fault_valid;
    pcs_valid_type vcore_temp_valid;

} pcs_dpm_board_data;

typedef struct{
	clcmsw_generic_ipmc_data_type generic_ipmc;
	pcs_dpm_board_data dpm_data;
}clcmsw_ipmc_dpm_data;

static int ipmc_dpm_log_print_header(FILE **fp) {
    fprintf(*fp,  "DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP,LOG PAYLOAD LENGTH,");

    fprintf(*fp, "%s,", "LRM_dev");
    fprintf(*fp, "%s,", "is_auth");
    fprintf(*fp, "%s,", "ipmb_a");
    fprintf(*fp, "%s,", "ipmb_b");
    fprintf(*fp, "%s,", "fw_version_major");
    fprintf(*fp, "%s,", "fw_version_minor");
    fprintf(*fp, "%s,", "fw_version_patch");
    fprintf(*fp, "%s,", "operation_mode");
    fprintf(*fp, "%s,", "sequence_number");
    fprintf(*fp, "%s,", "power_status");
    fprintf(*fp, "%s,", "reset_counter");
    fprintf(*fp, "%s,", "health_data_refresh_counter");

    // pgood fields
    fprintf(*fp, "%s,", "dtn_es_1v0_pg");
    fprintf(*fp, "%s,", "dtn_es_1v2_pg");
    fprintf(*fp, "%s,", "dtn_es_vdda_1v_pg");
    fprintf(*fp, "%s,", "tps_pgood_dtn_es_vcc3v3");
    fprintf(*fp, "%s,", "dtn_es_vddix_3v3_pgood");
    fprintf(*fp, "%s,", "dtn_es_2v5_pg");
    fprintf(*fp, "%s,", "dtn_es_1v8_pg");
    fprintf(*fp, "%s,", "dtn_es_vddi_1v8_pg");
    fprintf(*fp, "%s,", "phy_pgood_dvdd_1v");
    fprintf(*fp, "%s,", "vtt_vref_pg");
    fprintf(*fp, "%s,", "tps_pgood_cpu_s1vdd");
    fprintf(*fp, "%s,", "tps_pgood_cpu_s2vdd");
    fprintf(*fp, "%s,", "dtn_es_vddi_2v5_pg");
    fprintf(*fp, "%s,", "bias_pgood_5v");
    fprintf(*fp, "%s,", "tpsm_pgood_cpu_vcc1v35");
    fprintf(*fp, "%s,", "tpsm_pgood_cpu_vcc1v8");
    fprintf(*fp, "%s,", "prog_sfp_pgood");
    fprintf(*fp, "%s,", "tpsm_pgood_cpu_vcc1v2");

    // valid fields
    fprintf(*fp, "%s,", "dtn_es_1v0_pg_valid");
    fprintf(*fp, "%s,", "dtn_es_1v2_pg_valid");
    fprintf(*fp, "%s,", "dtn_es_vdda_1v_pg_valid");
    fprintf(*fp, "%s,", "tps_pgood_dtn_es_vcc3v3_valid");
    fprintf(*fp, "%s,", "dtn_es_vddix_3v3_pgood_valid");
    fprintf(*fp, "%s,", "dtn_es_2v5_pg_valid");
    fprintf(*fp, "%s,", "dtn_es_1v8_pg_valid");
    fprintf(*fp, "%s,", "dtn_es_vddi_1v8_pg_valid");
    fprintf(*fp, "%s,", "phy_pgood_dvdd_1v_valid");
    fprintf(*fp, "%s,", "vtt_vref_pg_valid");
    fprintf(*fp, "%s,", "tps_pgood_cpu_s1vdd_valid");
    fprintf(*fp, "%s,", "tps_pgood_cpu_s2vdd_valid");
    fprintf(*fp, "%s,", "dtn_es_vddi_2v5_pg_valid");
    fprintf(*fp, "%s,", "bias_pgood_5v_valid");
    fprintf(*fp, "%s,", "tpsm_pgood_cpu_vcc1v35_valid");
    fprintf(*fp, "%s,", "tpsm_pgood_cpu_vcc1v8_valid");
    fprintf(*fp, "%s,", "prog_sfp_pgood_valid");
    fprintf(*fp, "%s,", "tpsm_pgood_cpu_vcc1v2_valid");

    // temperature fields
    fprintf(*fp, "%s,", "tmp451_sensor_for_cpu");
    fprintf(*fp, "%s,", "tmp451_sensor_for_fpga");

    // current fields
    fprintf(*fp, "%s,", "dtn_es_1v0_cs");
    fprintf(*fp, "%s,", "dtn_es_vddi_2v5_cs");
    fprintf(*fp, "%s,", "dtn_es_vdda_1v_cs");
    fprintf(*fp, "%s,", "dtn_es_vcc3v3_cs");
    fprintf(*fp, "%s,", "dtn_es_2v5_cs");
    fprintf(*fp, "%s,", "dtn_es_vdd_1v8_cs");
    fprintf(*fp, "%s,", "dtn_es_vddi_1v8_cs");
    fprintf(*fp, "%s,", "cpu_1v2_cs");
    fprintf(*fp, "%s,", "cpu_1v8_cs");
    fprintf(*fp, "%s,", "cpu_1v35_cs");
    fprintf(*fp, "%s,", "cpu_s1vdd_cs");
    fprintf(*fp, "%s,", "cpu_s2vdd_cs");
    fprintf(*fp, "%s,", "vcore_imon");

    // voltage fields
    fprintf(*fp, "%s,", "cpu_1v8");
    fprintf(*fp, "%s,", "cpu_1v35");
    fprintf(*fp, "%s,", "cpu_s1vdd");
    fprintf(*fp, "%s,", "cpu_s2vdd");
    fprintf(*fp, "%s,", "dtn_es_1v0");
    fprintf(*fp, "%s,", "dtn_es_vddi_2v5");
    fprintf(*fp, "%s,", "dtn_es_vdda_1v");
    fprintf(*fp, "%s,", "dtn_es_vcc3v3");
    fprintf(*fp, "%s,", "dtn_es_2v5");
    fprintf(*fp, "%s,", "dtn_es_vdd_1v8");
    fprintf(*fp, "%s,", "dtn_es_vddi_1v8");
    fprintf(*fp, "%s,", "cpu_1v2");

    // other good fields
    fprintf(*fp, "%s,", "ipmbb_ready");
    fprintf(*fp, "%s,", "ipmba_ready");
    fprintf(*fp, "%s,", "clk_los");
    fprintf(*fp, "%s,", "clk_gen_lol");
    fprintf(*fp, "%s,", "mcu_asleep");
    fprintf(*fp, "%s,", "vcore_rdy");
    fprintf(*fp, "%s,", "vcore_fault");
    fprintf(*fp, "%s,", "vcore_temp");
    fprintf(*fp, "%s,", "ddr_test");
    fprintf(*fp, "%s,", "serdes1_pll1_test");
    fprintf(*fp, "%s,", "serdes2_pll1_test");
    fprintf(*fp, "%s,", "serdes2_pll2_test");
    fprintf(*fp, "%s,", "ifc_nand_test");

    // other valid fields
    fprintf(*fp, "%s,", "ipmbb_ready_valid");
    fprintf(*fp, "%s,", "ipmba_ready_valid");
    fprintf(*fp, "%s,", "clk_los_valid");
    fprintf(*fp, "%s,", "clk_gen_lol_valid");
    fprintf(*fp, "%s,", "mcu_asleep_valid");
    fprintf(*fp, "%s,", "vcore_rdy_valid");
    fprintf(*fp, "%s,", "vcore_fault_valid");
    fprintf(*fp, "%s\n", "vcore_temp_valid");
}

static int ipmc_dpm_log_parser(FILE *outfp, clcmsw_ipmc_dpm_data *ipmc_dpm) {

    ipmc_dpm->generic_ipmc.LRM_dev = ntohl(ipmc_dpm->generic_ipmc.LRM_dev);
    ipmc_dpm->generic_ipmc.is_auth = ipmc_dpm->generic_ipmc.is_auth;
    
    // Convert enums using ntohl()
    ipmc_dpm->generic_ipmc.ipmb_a = ntohl(ipmc_dpm->generic_ipmc.ipmb_a);
    ipmc_dpm->generic_ipmc.ipmb_b = ntohl(ipmc_dpm->generic_ipmc.ipmb_b);
    ipmc_dpm->generic_ipmc.operation_mode = ntohl(ipmc_dpm->generic_ipmc.operation_mode);
    ipmc_dpm->generic_ipmc.power_status = ntohl(ipmc_dpm->generic_ipmc.power_status);
    
    // Convert uint16_t values using ntohs()
    ipmc_dpm->generic_ipmc.sequence_number = ntohs(ipmc_dpm->generic_ipmc.sequence_number);
    
    // Convert pgood fields (enums)
    ipmc_dpm->dpm_data.dtn_es_1v0_pg = ntohl(ipmc_dpm->dpm_data.dtn_es_1v0_pg);
    ipmc_dpm->dpm_data.dtn_es_1v2_pg = ntohl(ipmc_dpm->dpm_data.dtn_es_1v2_pg);
    ipmc_dpm->dpm_data.dtn_es_vdda_1v_pg = ntohl(ipmc_dpm->dpm_data.dtn_es_vdda_1v_pg);
    ipmc_dpm->dpm_data.tps_pgood_dtn_es_vcc3v3 = ntohl(ipmc_dpm->dpm_data.tps_pgood_dtn_es_vcc3v3);
    ipmc_dpm->dpm_data.dtn_es_vddix_3v3_pgood = ntohl(ipmc_dpm->dpm_data.dtn_es_vddix_3v3_pgood);
    ipmc_dpm->dpm_data.dtn_es_2v5_pg = ntohl(ipmc_dpm->dpm_data.dtn_es_2v5_pg);
    ipmc_dpm->dpm_data.dtn_es_1v8_pg = ntohl(ipmc_dpm->dpm_data.dtn_es_1v8_pg);
    ipmc_dpm->dpm_data.dtn_es_vddi_1v8_pg = ntohl(ipmc_dpm->dpm_data.dtn_es_vddi_1v8_pg);
    ipmc_dpm->dpm_data.phy_pgood_dvdd_1v = ntohl(ipmc_dpm->dpm_data.phy_pgood_dvdd_1v);
    ipmc_dpm->dpm_data.vtt_vref_pg = ntohl(ipmc_dpm->dpm_data.vtt_vref_pg);
    ipmc_dpm->dpm_data.tps_pgood_cpu_s1vdd = ntohl(ipmc_dpm->dpm_data.tps_pgood_cpu_s1vdd);
    ipmc_dpm->dpm_data.tps_pgood_cpu_s2vdd = ntohl(ipmc_dpm->dpm_data.tps_pgood_cpu_s2vdd);
    ipmc_dpm->dpm_data.dtn_es_vddi_2v5_pg = ntohl(ipmc_dpm->dpm_data.dtn_es_vddi_2v5_pg);
    ipmc_dpm->dpm_data.bias_pgood_5v = ntohl(ipmc_dpm->dpm_data.bias_pgood_5v);
    ipmc_dpm->dpm_data.tpsm_pgood_cpu_vcc1v35 = ntohl(ipmc_dpm->dpm_data.tpsm_pgood_cpu_vcc1v35);
    ipmc_dpm->dpm_data.tpsm_pgood_cpu_vcc1v8 = ntohl(ipmc_dpm->dpm_data.tpsm_pgood_cpu_vcc1v8);
    ipmc_dpm->dpm_data.prog_sfp_pgood = ntohl(ipmc_dpm->dpm_data.prog_sfp_pgood);
    ipmc_dpm->dpm_data.tpsm_pgood_cpu_vcc1v2 = ntohl(ipmc_dpm->dpm_data.tpsm_pgood_cpu_vcc1v2);
    
    // Convert valid fields (enums)
    ipmc_dpm->dpm_data.dtn_es_1v0_pg_valid = ntohl(ipmc_dpm->dpm_data.dtn_es_1v0_pg_valid);
    ipmc_dpm->dpm_data.dtn_es_1v2_pg_valid = ntohl(ipmc_dpm->dpm_data.dtn_es_1v2_pg_valid);
    ipmc_dpm->dpm_data.dtn_es_vdda_1v_pg_valid = ntohl(ipmc_dpm->dpm_data.dtn_es_vdda_1v_pg_valid);
    ipmc_dpm->dpm_data.tps_pgood_dtn_es_vcc3v3_valid = ntohl(ipmc_dpm->dpm_data.tps_pgood_dtn_es_vcc3v3_valid);
    ipmc_dpm->dpm_data.dtn_es_vddix_3v3_pgood_valid = ntohl(ipmc_dpm->dpm_data.dtn_es_vddix_3v3_pgood_valid);
    ipmc_dpm->dpm_data.dtn_es_2v5_pg_valid = ntohl(ipmc_dpm->dpm_data.dtn_es_2v5_pg_valid);
    ipmc_dpm->dpm_data.dtn_es_1v8_pg_valid = ntohl(ipmc_dpm->dpm_data.dtn_es_1v8_pg_valid);
    ipmc_dpm->dpm_data.dtn_es_vddi_1v8_pg_valid = ntohl(ipmc_dpm->dpm_data.dtn_es_vddi_1v8_pg_valid);
    ipmc_dpm->dpm_data.phy_pgood_dvdd_1v_valid = ntohl(ipmc_dpm->dpm_data.phy_pgood_dvdd_1v_valid);
    ipmc_dpm->dpm_data.vtt_vref_pg_valid = ntohl(ipmc_dpm->dpm_data.vtt_vref_pg_valid);
    ipmc_dpm->dpm_data.tps_pgood_cpu_s1vdd_valid = ntohl(ipmc_dpm->dpm_data.tps_pgood_cpu_s1vdd_valid);
    ipmc_dpm->dpm_data.tps_pgood_cpu_s2vdd_valid = ntohl(ipmc_dpm->dpm_data.tps_pgood_cpu_s2vdd_valid);
    ipmc_dpm->dpm_data.dtn_es_vddi_2v5_pg_valid = ntohl(ipmc_dpm->dpm_data.dtn_es_vddi_2v5_pg_valid);
    ipmc_dpm->dpm_data.bias_pgood_5v_valid = ntohl(ipmc_dpm->dpm_data.bias_pgood_5v_valid);
    ipmc_dpm->dpm_data.tpsm_pgood_cpu_vcc1v35_valid = ntohl(ipmc_dpm->dpm_data.tpsm_pgood_cpu_vcc1v35_valid);
    ipmc_dpm->dpm_data.tpsm_pgood_cpu_vcc1v8_valid = ntohl(ipmc_dpm->dpm_data.tpsm_pgood_cpu_vcc1v8_valid);
    ipmc_dpm->dpm_data.prog_sfp_pgood_valid = ntohl(ipmc_dpm->dpm_data.prog_sfp_pgood_valid);
    ipmc_dpm->dpm_data.tpsm_pgood_cpu_vcc1v2_valid = ntohl(ipmc_dpm->dpm_data.tpsm_pgood_cpu_vcc1v2_valid);
    
    // Convert signed value fields
    ipmc_dpm->dpm_data.tmp451_sensor_for_cpu.value = ntohs(ipmc_dpm->dpm_data.tmp451_sensor_for_cpu.value);
    ipmc_dpm->dpm_data.tmp451_sensor_for_cpu.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.tmp451_sensor_for_cpu.limit_status.is_specified);
    ipmc_dpm->dpm_data.tmp451_sensor_for_fpga.value = ntohs(ipmc_dpm->dpm_data.tmp451_sensor_for_fpga.value);
    ipmc_dpm->dpm_data.tmp451_sensor_for_fpga.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.tmp451_sensor_for_fpga.limit_status.is_specified);
    
    // Convert unsigned value fields
    ipmc_dpm->dpm_data.dtn_es_1v0_cs.value = ntohs(ipmc_dpm->dpm_data.dtn_es_1v0_cs.value);
    ipmc_dpm->dpm_data.dtn_es_vddi_2v5_cs.value = ntohs(ipmc_dpm->dpm_data.dtn_es_vddi_2v5_cs.value);
    ipmc_dpm->dpm_data.dtn_es_vdda_1v_cs.value = ntohs(ipmc_dpm->dpm_data.dtn_es_vdda_1v_cs.value);
    ipmc_dpm->dpm_data.dtn_es_vcc3v3_cs.value = ntohs(ipmc_dpm->dpm_data.dtn_es_vcc3v3_cs.value);
    ipmc_dpm->dpm_data.dtn_es_2v5_cs.value = ntohs(ipmc_dpm->dpm_data.dtn_es_2v5_cs.value);
    ipmc_dpm->dpm_data.dtn_es_vdd_1v8_cs.value = ntohs(ipmc_dpm->dpm_data.dtn_es_vdd_1v8_cs.value);
    ipmc_dpm->dpm_data.dtn_es_vddi_1v8_cs.value = ntohs(ipmc_dpm->dpm_data.dtn_es_vddi_1v8_cs.value);
    ipmc_dpm->dpm_data.cpu_1v2_cs.value = ntohs(ipmc_dpm->dpm_data.cpu_1v2_cs.value);
    ipmc_dpm->dpm_data.cpu_1v8_cs.value = ntohs(ipmc_dpm->dpm_data.cpu_1v8_cs.value);
    ipmc_dpm->dpm_data.cpu_1v35_cs.value = ntohs(ipmc_dpm->dpm_data.cpu_1v35_cs.value);
    ipmc_dpm->dpm_data.cpu_s1vdd_cs.value = ntohs(ipmc_dpm->dpm_data.cpu_s1vdd_cs.value);
    ipmc_dpm->dpm_data.cpu_s2vdd_cs.value = ntohs(ipmc_dpm->dpm_data.cpu_s2vdd_cs.value);
    ipmc_dpm->dpm_data.vcore_imon.value = ntohs(ipmc_dpm->dpm_data.vcore_imon.value);

    ipmc_dpm->dpm_data.dtn_es_1v0_cs.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.dtn_es_1v0_cs.limit_status.is_specified);
    ipmc_dpm->dpm_data.dtn_es_vddi_2v5_cs.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.dtn_es_vddi_2v5_cs.limit_status.is_specified);
    ipmc_dpm->dpm_data.dtn_es_vdda_1v_cs.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.dtn_es_vdda_1v_cs.limit_status.is_specified);
    ipmc_dpm->dpm_data.dtn_es_vcc3v3_cs.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.dtn_es_vcc3v3_cs.limit_status.is_specified);
    ipmc_dpm->dpm_data.dtn_es_2v5_cs.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.dtn_es_2v5_cs.limit_status.is_specified);
    ipmc_dpm->dpm_data.dtn_es_vdd_1v8_cs.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.dtn_es_vdd_1v8_cs.limit_status.is_specified);
    ipmc_dpm->dpm_data.dtn_es_vddi_1v8_cs.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.dtn_es_vddi_1v8_cs.limit_status.is_specified);
    ipmc_dpm->dpm_data.cpu_1v2_cs.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.cpu_1v2_cs.limit_status.is_specified);
    ipmc_dpm->dpm_data.cpu_1v8_cs.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.cpu_1v8_cs.limit_status.is_specified);
    ipmc_dpm->dpm_data.cpu_1v35_cs.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.cpu_1v35_cs.limit_status.is_specified);
    ipmc_dpm->dpm_data.cpu_s1vdd_cs.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.cpu_s1vdd_cs.limit_status.is_specified);
    ipmc_dpm->dpm_data.cpu_s2vdd_cs.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.cpu_s2vdd_cs.limit_status.is_specified);
    ipmc_dpm->dpm_data.vcore_imon.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.vcore_imon.limit_status.is_specified);
    
    // Convert voltage fields
    ipmc_dpm->dpm_data.cpu_1v8.value = ntohs(ipmc_dpm->dpm_data.cpu_1v8.value);
    ipmc_dpm->dpm_data.cpu_1v35.value = ntohs(ipmc_dpm->dpm_data.cpu_1v35.value);
    ipmc_dpm->dpm_data.cpu_s1vdd.value = ntohs(ipmc_dpm->dpm_data.cpu_s1vdd.value);
    ipmc_dpm->dpm_data.cpu_s2vdd.value = ntohs(ipmc_dpm->dpm_data.cpu_s2vdd.value);
    ipmc_dpm->dpm_data.dtn_es_1v0.value = ntohs(ipmc_dpm->dpm_data.dtn_es_1v0.value);
    ipmc_dpm->dpm_data.dtn_es_vddi_2v5.value = ntohs(ipmc_dpm->dpm_data.dtn_es_vddi_2v5.value);
    ipmc_dpm->dpm_data.dtn_es_vdda_1v.value = ntohs(ipmc_dpm->dpm_data.dtn_es_vdda_1v.value);
    ipmc_dpm->dpm_data.dtn_es_vcc3v3.value = ntohs(ipmc_dpm->dpm_data.dtn_es_vcc3v3.value);
    ipmc_dpm->dpm_data.dtn_es_2v5.value = ntohs(ipmc_dpm->dpm_data.dtn_es_2v5.value);
    ipmc_dpm->dpm_data.dtn_es_vdd_1v8.value = ntohs(ipmc_dpm->dpm_data.dtn_es_vdd_1v8.value);
    ipmc_dpm->dpm_data.dtn_es_vddi_1v8.value = ntohs(ipmc_dpm->dpm_data.dtn_es_vddi_1v8.value);
    ipmc_dpm->dpm_data.cpu_1v2.value = ntohs(ipmc_dpm->dpm_data.cpu_1v2.value);

    ipmc_dpm->dpm_data.cpu_1v8.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.cpu_1v8.limit_status.is_specified);
    ipmc_dpm->dpm_data.cpu_1v35.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.cpu_1v35.limit_status.is_specified);
    ipmc_dpm->dpm_data.cpu_s1vdd.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.cpu_s1vdd.limit_status.is_specified);
    ipmc_dpm->dpm_data.cpu_s2vdd.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.cpu_s2vdd.limit_status.is_specified);
    ipmc_dpm->dpm_data.dtn_es_1v0.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.dtn_es_1v0.limit_status.is_specified);
    ipmc_dpm->dpm_data.dtn_es_vddi_2v5.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.dtn_es_vddi_2v5.limit_status.is_specified);
    ipmc_dpm->dpm_data.dtn_es_vdda_1v.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.dtn_es_vdda_1v.limit_status.is_specified);
    ipmc_dpm->dpm_data.dtn_es_vcc3v3.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.dtn_es_vcc3v3.limit_status.is_specified);
    ipmc_dpm->dpm_data.dtn_es_2v5.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.dtn_es_2v5.limit_status.is_specified);
    ipmc_dpm->dpm_data.dtn_es_vdd_1v8.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.dtn_es_vdd_1v8.limit_status.is_specified);
    ipmc_dpm->dpm_data.dtn_es_vddi_1v8.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.dtn_es_vddi_1v8.limit_status.is_specified);
    ipmc_dpm->dpm_data.cpu_1v2.limit_status.is_specified = ntohl(ipmc_dpm->dpm_data.cpu_1v2.limit_status.is_specified);

    
    // Convert other good fields (enums)
    ipmc_dpm->dpm_data.ipmbb_ready = ntohl(ipmc_dpm->dpm_data.ipmbb_ready);
    ipmc_dpm->dpm_data.ipmba_ready = ntohl(ipmc_dpm->dpm_data.ipmba_ready);
    ipmc_dpm->dpm_data.clk_los = ntohl(ipmc_dpm->dpm_data.clk_los);
    ipmc_dpm->dpm_data.clk_gen_lol = ntohl(ipmc_dpm->dpm_data.clk_gen_lol);
    ipmc_dpm->dpm_data.mcu_asleep = ntohl(ipmc_dpm->dpm_data.mcu_asleep);
    ipmc_dpm->dpm_data.vcore_rdy = ntohl(ipmc_dpm->dpm_data.vcore_rdy);
    ipmc_dpm->dpm_data.vcore_fault = ntohl(ipmc_dpm->dpm_data.vcore_fault);
    ipmc_dpm->dpm_data.vcore_temp = ntohl(ipmc_dpm->dpm_data.vcore_temp);
    ipmc_dpm->dpm_data.ddr_test = ntohl(ipmc_dpm->dpm_data.ddr_test);
    ipmc_dpm->dpm_data.serdes1_pll1_test = ntohl(ipmc_dpm->dpm_data.serdes1_pll1_test);
    ipmc_dpm->dpm_data.serdes2_pll1_test = ntohl(ipmc_dpm->dpm_data.serdes2_pll1_test);
    ipmc_dpm->dpm_data.serdes2_pll2_test = ntohl(ipmc_dpm->dpm_data.serdes2_pll2_test);
    ipmc_dpm->dpm_data.ifc_nand_test = ntohl(ipmc_dpm->dpm_data.ifc_nand_test);
    
    // Convert other valid fields (enums)
    ipmc_dpm->dpm_data.ipmbb_ready_valid = ntohl(ipmc_dpm->dpm_data.ipmbb_ready_valid);
    ipmc_dpm->dpm_data.ipmba_ready_valid = ntohl(ipmc_dpm->dpm_data.ipmba_ready_valid);
    ipmc_dpm->dpm_data.clk_los_valid = ntohl(ipmc_dpm->dpm_data.clk_los_valid);
    ipmc_dpm->dpm_data.clk_gen_lol_valid = ntohl(ipmc_dpm->dpm_data.clk_gen_lol_valid);
    ipmc_dpm->dpm_data.mcu_asleep_valid = ntohl(ipmc_dpm->dpm_data.mcu_asleep_valid);
    ipmc_dpm->dpm_data.vcore_rdy_valid = ntohl(ipmc_dpm->dpm_data.vcore_rdy_valid);
    ipmc_dpm->dpm_data.vcore_fault_valid = ntohl(ipmc_dpm->dpm_data.vcore_fault_valid);
    ipmc_dpm->dpm_data.vcore_temp_valid = ntohl(ipmc_dpm->dpm_data.vcore_temp_valid);

    fprintf(outfp, "%d,", ipmc_dpm->generic_ipmc.LRM_dev);
    fprintf(outfp, "%u,", ipmc_dpm->generic_ipmc.is_auth);
    fprintf(outfp, "%d,", ipmc_dpm->generic_ipmc.ipmb_a);
    fprintf(outfp, "%d,", ipmc_dpm->generic_ipmc.ipmb_b);
    fprintf(outfp, "%u,", ipmc_dpm->generic_ipmc.fw_version_major);
    fprintf(outfp, "%u,", ipmc_dpm->generic_ipmc.fw_version_minor);
    fprintf(outfp, "%u,", ipmc_dpm->generic_ipmc.fw_version_patch);
    fprintf(outfp, "%d,", ipmc_dpm->generic_ipmc.operation_mode);
    fprintf(outfp, "%u,", ipmc_dpm->generic_ipmc.sequence_number);
    fprintf(outfp, "%d,", ipmc_dpm->generic_ipmc.power_status);
    fprintf(outfp, "%u,", ipmc_dpm->generic_ipmc.reset_counter);
    fprintf(outfp, "%u,", ipmc_dpm->generic_ipmc.health_data_refresh_counter);

    // pgood fields
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.dtn_es_1v0_pg);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.dtn_es_1v2_pg);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.dtn_es_vdda_1v_pg);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.tps_pgood_dtn_es_vcc3v3);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.dtn_es_vddix_3v3_pgood);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.dtn_es_2v5_pg);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.dtn_es_1v8_pg);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.dtn_es_vddi_1v8_pg);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.phy_pgood_dvdd_1v);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.vtt_vref_pg);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.tps_pgood_cpu_s1vdd);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.tps_pgood_cpu_s2vdd);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.dtn_es_vddi_2v5_pg);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.bias_pgood_5v);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.tpsm_pgood_cpu_vcc1v35);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.tpsm_pgood_cpu_vcc1v8);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.prog_sfp_pgood);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.tpsm_pgood_cpu_vcc1v2);

    // valid fields
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.dtn_es_1v0_pg_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.dtn_es_1v2_pg_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.dtn_es_vdda_1v_pg_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.tps_pgood_dtn_es_vcc3v3_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.dtn_es_vddix_3v3_pgood_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.dtn_es_2v5_pg_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.dtn_es_1v8_pg_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.dtn_es_vddi_1v8_pg_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.phy_pgood_dvdd_1v_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.vtt_vref_pg_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.tps_pgood_cpu_s1vdd_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.tps_pgood_cpu_s2vdd_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.dtn_es_vddi_2v5_pg_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.bias_pgood_5v_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.tpsm_pgood_cpu_vcc1v35_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.tpsm_pgood_cpu_vcc1v8_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.prog_sfp_pgood_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.tpsm_pgood_cpu_vcc1v2_valid);

    // temperature sensors
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.tmp451_sensor_for_cpu.value, ipmc_dpm->dpm_data.tmp451_sensor_for_cpu.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.tmp451_sensor_for_fpga.value, ipmc_dpm->dpm_data.tmp451_sensor_for_fpga.limit_status.is_specified);

    // current sensors
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.dtn_es_1v0_cs.value, ipmc_dpm->dpm_data.dtn_es_1v0_cs.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.dtn_es_vddi_2v5_cs.value, ipmc_dpm->dpm_data.dtn_es_vddi_2v5_cs.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.dtn_es_vdda_1v_cs.value, ipmc_dpm->dpm_data.dtn_es_vdda_1v_cs.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.dtn_es_vcc3v3_cs.value, ipmc_dpm->dpm_data.dtn_es_vcc3v3_cs.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.dtn_es_2v5_cs.value, ipmc_dpm->dpm_data.dtn_es_2v5_cs.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.dtn_es_vdd_1v8_cs.value, ipmc_dpm->dpm_data.dtn_es_vdd_1v8_cs.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.dtn_es_vddi_1v8_cs.value, ipmc_dpm->dpm_data.dtn_es_vddi_1v8_cs.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.cpu_1v2_cs.value, ipmc_dpm->dpm_data.cpu_1v2_cs.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.cpu_1v8_cs.value, ipmc_dpm->dpm_data.cpu_1v8_cs.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.cpu_1v35_cs.value, ipmc_dpm->dpm_data.cpu_1v35_cs.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.cpu_s1vdd_cs.value, ipmc_dpm->dpm_data.cpu_s1vdd_cs.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.cpu_s2vdd_cs.value, ipmc_dpm->dpm_data.cpu_s2vdd_cs.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.vcore_imon.value, ipmc_dpm->dpm_data.vcore_imon.limit_status.is_specified);

    // voltage sensors
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.cpu_1v8.value, ipmc_dpm->dpm_data.cpu_1v8.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.cpu_1v35.value, ipmc_dpm->dpm_data.cpu_1v35.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.cpu_s1vdd.value, ipmc_dpm->dpm_data.cpu_s1vdd.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.cpu_s2vdd.value, ipmc_dpm->dpm_data.cpu_s2vdd.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.dtn_es_1v0.value, ipmc_dpm->dpm_data.dtn_es_1v0.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.dtn_es_vddi_2v5.value, ipmc_dpm->dpm_data.dtn_es_vddi_2v5.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.dtn_es_vdda_1v.value, ipmc_dpm->dpm_data.dtn_es_vdda_1v.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.dtn_es_vcc3v3.value, ipmc_dpm->dpm_data.dtn_es_vcc3v3.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.dtn_es_2v5.value, ipmc_dpm->dpm_data.dtn_es_2v5.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.dtn_es_vdd_1v8.value, ipmc_dpm->dpm_data.dtn_es_vdd_1v8.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.dtn_es_vddi_1v8.value, ipmc_dpm->dpm_data.dtn_es_vddi_1v8.limit_status.is_specified);
    fprintf(outfp, "%d\\%d,", ipmc_dpm->dpm_data.cpu_1v2.value, ipmc_dpm->dpm_data.cpu_1v2.limit_status.is_specified);

    // other fields
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.ipmbb_ready);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.ipmba_ready);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.clk_los);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.clk_gen_lol);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.mcu_asleep);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.vcore_rdy);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.vcore_fault);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.vcore_temp);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.ddr_test);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.serdes1_pll1_test);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.serdes2_pll1_test);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.serdes2_pll2_test);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.ifc_nand_test);

    // other valid fields
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.ipmbb_ready_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.ipmba_ready_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.clk_los_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.clk_gen_lol_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.mcu_asleep_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.vcore_rdy_valid);
    fprintf(outfp, "%d,", ipmc_dpm->dpm_data.vcore_fault_valid);
    fprintf(outfp, "%d\n", ipmc_dpm->dpm_data.vcore_temp_valid);


    return 0;
}
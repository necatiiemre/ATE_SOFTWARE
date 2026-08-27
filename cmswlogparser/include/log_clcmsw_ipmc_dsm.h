#pragma once
typedef struct pcs_dsm_board_data
{
    /* pgood */
    pcs_good_type phy_pgood_dvdd_1v;
    pcs_good_type bias_pgood_5v;
    pcs_good_type refclk_vdd1v8_pg;
    pcs_good_type refclk_vddo_pg;
    pcs_good_type refclk_fod_pg;
    pcs_good_type refclk_vdd1_3v3_pg;
    pcs_good_type refclk_vdd2_3v3_pg;
    pcs_good_type onepps_vdd1v8_pg;
    pcs_good_type onepps_vddo_pg;
    pcs_good_type onepps_fod_pg;
    pcs_good_type onepps_vdd1_3v3_pg;
    pcs_good_type onepps_vdd2_3v3_pg;
    pcs_good_type dtn_es_1v2_pg;
    pcs_good_type dtn_es_vdda_1v_pg;
    pcs_good_type tps_pgood_dtn_es_vcc3v3;
    pcs_good_type dtn_es_vddix_3v3_pgood;
    pcs_good_type dtn_es_2v5_pg;
    pcs_good_type dtn_es_1v8_pg;
    pcs_good_type dtn_es_vddi_1v8_pg;
    pcs_good_type dtn_es_1v0_pg;
    pcs_good_type vtt_vref_pg;
    pcs_good_type tps_pgood_cpu_s1vdd;
    pcs_good_type tps_pgood_cpu_s2vdd;
    pcs_good_type tpsm_pgood_cpu_vcc1v35;
    pcs_good_type tpsm_pgood_cpu_vcc1v8;
    pcs_good_type prog_sfp_pgood;
    pcs_good_type tpsm_pgood_cpu_vcc1v2;
    pcs_good_type dtn_es_vddi_2v5_pg;

    /* valid */
    pcs_valid_type phy_pgood_dvdd_1v_valid;
    pcs_valid_type bias_pgood_5v_valid;
    pcs_valid_type refclk_vdd1v8_pg_valid;
    pcs_valid_type refclk_vddo_pg_valid;
    pcs_valid_type refclk_fod_pg_valid;
    pcs_valid_type refclk_vdd1_3v3_pg_valid;
    pcs_valid_type refclk_vdd2_3v3_pg_valid;
    pcs_valid_type onepps_vdd1v8_pg_valid;
    pcs_valid_type onepps_vddo_pg_valid;
    pcs_valid_type onepps_fod_pg_valid;
    pcs_valid_type onepps_vdd1_3v3_pg_valid;
    pcs_valid_type onepps_vdd2_3v3_pg_valid;
    pcs_valid_type dtn_es_1v2_pg_valid;
    pcs_valid_type dtn_es_vdda_1v_pg_valid;
    pcs_valid_type tps_pgood_dtn_es_vcc3v3_valid;
    pcs_valid_type dtn_es_vddix_3v3_pgood_valid;
    pcs_valid_type dtn_es_2v5_pg_valid;
    pcs_valid_type dtn_es_1v8_pg_valid;
    pcs_valid_type dtn_es_vddi_1v8_pg_valid;
    pcs_valid_type dtn_es_1v0_pg_valid;
    pcs_valid_type vtt_vref_pg_valid;
    pcs_valid_type tps_pgood_cpu_s1vdd_valid;
    pcs_valid_type tps_pgood_cpu_s2vdd_valid;
    pcs_valid_type tpsm_pgood_cpu_vcc1v35_valid;
    pcs_valid_type tpsm_pgood_cpu_vcc1v8_valid;
    pcs_valid_type prog_sfp_pgood_valid;
    pcs_valid_type tpsm_pgood_cpu_vcc1v2_valid;
    pcs_valid_type dtn_es_vddi_2v5_pg_valid;

    /* temperatures */
    Pcs_ipmc_signed_value_type tmp451_sensor_for_cpu;
    Pcs_ipmc_signed_value_type tmp451_sensor_for_fpga;

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
    Pcs_ipmc_unsigned_value_type plus_1v8_1pps_vdd;
    Pcs_ipmc_unsigned_value_type plus_1v8_1pps_vddo;
    Pcs_ipmc_unsigned_value_type plus_1v8_1pps_fod;
    Pcs_ipmc_unsigned_value_type plus_3v3_1pps_vdd1;
    Pcs_ipmc_unsigned_value_type plus_3v3_1pps_vdd2;
    Pcs_ipmc_unsigned_value_type plus_1v8_refclk_vdd;
    Pcs_ipmc_unsigned_value_type plus_1v8_refclk_vddo;
    Pcs_ipmc_unsigned_value_type plus_1v8_refclk_fod;
    Pcs_ipmc_unsigned_value_type plus_3v3_refclk_vdd1;
    Pcs_ipmc_unsigned_value_type plus_3v3_refclk_vdd2;

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
    Pcs_ipmc_unsigned_value_type plus_1v8_1pps_vdd_cs;
    Pcs_ipmc_unsigned_value_type plus_1v8_1pps_vddo_cs;
    Pcs_ipmc_unsigned_value_type plus_1v8_1pps_fod_cs;
    Pcs_ipmc_unsigned_value_type plus_3v3_1pps_vdd1_cs_cs;
    Pcs_ipmc_unsigned_value_type plus_3v3_1pps_vdd2_cs;
    Pcs_ipmc_unsigned_value_type plus_1v8_refclk_vdd_cs;
    Pcs_ipmc_unsigned_value_type plus_1v8_refclk_vddo_cs;
    Pcs_ipmc_unsigned_value_type plus_1v8_refclk_fod_cs;
    Pcs_ipmc_unsigned_value_type plus_3v3_refclk_vdd1_cs;
    Pcs_ipmc_unsigned_value_type plus_3v3_refclk_vdd2_cs;

    /* other */
    pcs_good_type cl_ref_lock_ind;
    pcs_good_type cl_ref_hldovr_stat;
    pcs_good_type cl_ref_los_ind;
    pcs_good_type cl_1pps_lock_ind;
    pcs_good_type cl_1pps_hldovr_stat;
    pcs_good_type cl_1pps_los_ind;
    pcs_good_type clk_los;
    pcs_good_type clk_gen_lol;
    pcs_good_type ipmbb_ready;
    pcs_good_type ipmba_ready;
    pcs_good_type mcu_asleep;

    pcs_good_type ddr_test;
    pcs_good_type serdes1_pll1_test;
    pcs_good_type serdes1_pll2_test;
    pcs_good_type ifc_nand_test;

    /* other valid */
    pcs_valid_type cl_ref_lock_ind_valid;
    pcs_valid_type cl_ref_hldovr_stat_valid;
    pcs_valid_type cl_ref_los_ind_valid;
    pcs_valid_type cl_1pps_lock_ind_valid;
    pcs_valid_type cl_1pps_hldovr_stat_valid;
    pcs_valid_type cl_1pps_los_ind_valid;
    pcs_valid_type clk_los_valid;
    pcs_valid_type clk_gen_lol_valid;
    pcs_valid_type ipmbb_ready_valid;
    pcs_valid_type ipmba_ready_valid;
    pcs_valid_type mcu_asleep_valid;
} pcs_dsm_board_data;

typedef struct{
	clcmsw_generic_ipmc_data_type generic_ipmc;
	pcs_dsm_board_data dsm_data;
}clcmsw_ipmc_dsm_data;

static void ipmc_dsm_log_header_print(FILE **fp)
{
                fprintf(*fp,  "DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP, LOG PAYLOAD LENGTH,");

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
                fprintf(*fp, "%s,", "phy_pgood_dvdd_1v");
                fprintf(*fp, "%s,", "bias_pgood_5v");
                fprintf(*fp, "%s,", "refclk_vdd1v8_pg");
                fprintf(*fp, "%s,", "refclk_vddo_pg");
                fprintf(*fp, "%s,", "refclk_fod_pg");
                fprintf(*fp, "%s,", "refclk_vdd1_3v3_pg");
                fprintf(*fp, "%s,", "refclk_vdd2_3v3_pg");
                fprintf(*fp, "%s,", "onepps_vdd1v8_pg");
                fprintf(*fp, "%s,", "onepps_vddo_pg");
                fprintf(*fp, "%s,", "onepps_fod_pg");
                fprintf(*fp, "%s,", "onepps_vdd1_3v3_pg");
                fprintf(*fp, "%s,", "onepps_vdd2_3v3_pg");
                fprintf(*fp, "%s,", "dtn_es_1v2_pg");
                fprintf(*fp, "%s,", "dtn_es_vdda_1v_pg");
                fprintf(*fp, "%s,", "tps_pgood_dtn_es_vcc3v3");
                fprintf(*fp, "%s,", "dtn_es_vddix_3v3_pgood");
                fprintf(*fp, "%s,", "dtn_es_2v5_pg");
                fprintf(*fp, "%s,", "dtn_es_1v8_pg");
                fprintf(*fp, "%s,", "dtn_es_vddi_1v8_pg");
                fprintf(*fp, "%s,", "dtn_es_1v0_pg");
                fprintf(*fp, "%s,", "vtt_vref_pg");
                fprintf(*fp, "%s,", "tps_pgood_cpu_s1vdd");
                fprintf(*fp, "%s,", "tps_pgood_cpu_s2vdd");
                fprintf(*fp, "%s,", "tpsm_pgood_cpu_vcc1v35");
                fprintf(*fp, "%s,", "tpsm_pgood_cpu_vcc1v8");
                fprintf(*fp, "%s,", "prog_sfp_pgood");
                fprintf(*fp, "%s,", "tpsm_pgood_cpu_vcc1v2");
                fprintf(*fp, "%s,", "dtn_es_vddi_2v5_pg");

                // valid fields
                fprintf(*fp, "%s,", "phy_pgood_dvdd_1v_valid");
                fprintf(*fp, "%s,", "bias_pgood_5v_valid");
                fprintf(*fp, "%s,", "refclk_vdd1v8_pg_valid");
                fprintf(*fp, "%s,", "refclk_vddo_pg_valid");
                fprintf(*fp, "%s,", "refclk_fod_pg_valid");
                fprintf(*fp, "%s,", "refclk_vdd1_3v3_pg_valid");
                fprintf(*fp, "%s,", "refclk_vdd2_3v3_pg_valid");
                fprintf(*fp, "%s,", "onepps_vdd1v8_pg_valid");
                fprintf(*fp, "%s,", "onepps_vddo_pg_valid");
                fprintf(*fp, "%s,", "onepps_fod_pg_valid");
                fprintf(*fp, "%s,", "onepps_vdd1_3v3_pg_valid");
                fprintf(*fp, "%s,", "onepps_vdd2_3v3_pg_valid");
                fprintf(*fp, "%s,", "dtn_es_1v2_pg_valid");
                fprintf(*fp, "%s,", "dtn_es_vdda_1v_pg_valid");
                fprintf(*fp, "%s,", "tps_pgood_dtn_es_vcc3v3_valid");
                fprintf(*fp, "%s,", "dtn_es_vddix_3v3_pgood_valid");
                fprintf(*fp, "%s,", "dtn_es_2v5_pg_valid");
                fprintf(*fp, "%s,", "dtn_es_1v8_pg_valid");
                fprintf(*fp, "%s,", "dtn_es_vddi_1v8_pg_valid");
                fprintf(*fp, "%s,", "dtn_es_1v0_pg_valid");
                fprintf(*fp, "%s,", "vtt_vref_pg_valid");
                fprintf(*fp, "%s,", "tps_pgood_cpu_s1vdd_valid");
                fprintf(*fp, "%s,", "tps_pgood_cpu_s2vdd_valid");
                fprintf(*fp, "%s,", "tpsm_pgood_cpu_vcc1v35_valid");
                fprintf(*fp, "%s,", "tpsm_pgood_cpu_vcc1v8_valid");
                fprintf(*fp, "%s,", "prog_sfp_pgood_valid");
                fprintf(*fp, "%s,", "tpsm_pgood_cpu_vcc1v2_valid");
                fprintf(*fp, "%s,", "dtn_es_vddi_2v5_pg_valid");

                // temperature sensors
                fprintf(*fp, "%s\\is_specified\\validity_region,", "tmp451_sensor_for_cpu");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "tmp451_sensor_for_fpga");

                // voltage values
                fprintf(*fp, "%s\\is_specified\\validity_region,", "cpu_1v8");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "cpu_1v35");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "cpu_s1vdd");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "cpu_s2vdd");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "dtn_es_1v0");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "dtn_es_vddi_2v5");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "dtn_es_vdda_1v");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "dtn_es_vcc3v3");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "dtn_es_2v5");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "dtn_es_vdd_1v8");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "dtn_es_vddi_1v8");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "cpu_1v2");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_1v8_1pps_vdd");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_1v8_1pps_vddo");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_1v8_1pps_fod");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_3v3_1pps_vdd1");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_3v3_1pps_vdd2");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_1v8_refclk_vdd");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_1v8_refclk_vddo");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_1v8_refclk_fod");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_3v3_refclk_vdd1");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_3v3_refclk_vdd2");

                // current values
                fprintf(*fp, "%s\\is_specified\\validity_region,", "dtn_es_1v0_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "dtn_es_vddi_2v5_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "dtn_es_vdda_1v_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "dtn_es_vcc3v3_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "dtn_es_2v5_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "dtn_es_vdd_1v8_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "dtn_es_vddi_1v8_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "cpu_1v2_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "cpu_1v8_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "cpu_1v35_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "cpu_s1vdd_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "cpu_s2vdd_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_1v8_1pps_vdd_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_1v8_1pps_vddo_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_1v8_1pps_fod_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_3v3_1pps_vdd1_cs_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_3v3_1pps_vdd2_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_1v8_refclk_vdd_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_1v8_refclk_vddo_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_1v8_refclk_fod_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_3v3_refclk_vdd1_cs");
                fprintf(*fp, "%s\\is_specified\\validity_region,", "plus_3v3_refclk_vdd2_cs");

                // other fields
                fprintf(*fp, "%s,", "cl_ref_lock_ind");
                fprintf(*fp, "%s,", "cl_ref_hldovr_stat");
                fprintf(*fp, "%s,", "cl_ref_los_ind");
                fprintf(*fp, "%s,", "cl_1pps_lock_ind");
                fprintf(*fp, "%s,", "cl_1pps_hldovr_stat");
                fprintf(*fp, "%s,", "cl_1pps_los_ind");
                fprintf(*fp, "%s,", "clk_los");
                fprintf(*fp, "%s,", "clk_gen_lol");
                fprintf(*fp, "%s,", "ipmbb_ready");
                fprintf(*fp, "%s,", "ipmba_ready");
                fprintf(*fp, "%s,", "mcu_asleep");
                fprintf(*fp, "%s,", "ddr_test");
                fprintf(*fp, "%s,", "serdes1_pll1_test");
                fprintf(*fp, "%s,", "serdes1_pll2_test");
                fprintf(*fp, "%s,", "ifc_nand_test");

                // other valid fields
                fprintf(*fp, "%s,", "cl_ref_lock_ind_valid");
                fprintf(*fp, "%s,", "cl_ref_hldovr_stat_valid");
                fprintf(*fp, "%s,", "cl_ref_los_ind_valid");
                fprintf(*fp, "%s,", "cl_1pps_lock_ind_valid");
                fprintf(*fp, "%s,", "cl_1pps_hldovr_stat_valid");
                fprintf(*fp, "%s,", "cl_1pps_los_ind_valid");
                fprintf(*fp, "%s,", "clk_los_valid");
                fprintf(*fp, "%s,", "clk_gen_lol_valid");
                fprintf(*fp, "%s,", "ipmbb_ready_valid");
                fprintf(*fp, "%s,", "ipmba_ready_valid");
                fprintf(*fp, "%s\n", "mcu_asleep_valid");
}

static int ipmc_dsm_log_parser(FILE *outfp, clcmsw_ipmc_dsm_data *ipmc_dsm) {

    ipmc_dsm->generic_ipmc.LRM_dev = ntohl(ipmc_dsm->generic_ipmc.LRM_dev);
    ipmc_dsm->generic_ipmc.is_auth = ipmc_dsm->generic_ipmc.is_auth;
    ipmc_dsm->generic_ipmc.ipmb_a = ntohl(ipmc_dsm->generic_ipmc.ipmb_a);
    ipmc_dsm->generic_ipmc.ipmb_b = ntohl(ipmc_dsm->generic_ipmc.ipmb_b);
    ipmc_dsm->generic_ipmc.operation_mode = ntohl(ipmc_dsm->generic_ipmc.operation_mode);
    ipmc_dsm->generic_ipmc.sequence_number = ntohs(ipmc_dsm->generic_ipmc.sequence_number);
    ipmc_dsm->generic_ipmc.power_status = ntohl(ipmc_dsm->generic_ipmc.power_status);
    ipmc_dsm->generic_ipmc.reset_counter = ipmc_dsm->generic_ipmc.reset_counter;
    ipmc_dsm->generic_ipmc.health_data_refresh_counter = ipmc_dsm->generic_ipmc.health_data_refresh_counter;

    // Convert all relevant fields from network byte order to host byte order
    // PGOOD Fields
    ipmc_dsm->dsm_data.phy_pgood_dvdd_1v = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.phy_pgood_dvdd_1v);
    ipmc_dsm->dsm_data.bias_pgood_5v = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.bias_pgood_5v);
    ipmc_dsm->dsm_data.refclk_vdd1v8_pg = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.refclk_vdd1v8_pg);
    ipmc_dsm->dsm_data.refclk_vddo_pg = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.refclk_vddo_pg);
    ipmc_dsm->dsm_data.refclk_fod_pg = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.refclk_fod_pg);
    ipmc_dsm->dsm_data.refclk_vdd1_3v3_pg = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.refclk_vdd1_3v3_pg);
    ipmc_dsm->dsm_data.refclk_vdd2_3v3_pg = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.refclk_vdd2_3v3_pg);
    ipmc_dsm->dsm_data.onepps_vdd1v8_pg = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.onepps_vdd1v8_pg);
    ipmc_dsm->dsm_data.onepps_vddo_pg = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.onepps_vddo_pg);
    ipmc_dsm->dsm_data.onepps_fod_pg = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.onepps_fod_pg);
    ipmc_dsm->dsm_data.onepps_vdd1_3v3_pg = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.onepps_vdd1_3v3_pg);
    ipmc_dsm->dsm_data.onepps_vdd2_3v3_pg = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.onepps_vdd2_3v3_pg);
    ipmc_dsm->dsm_data.dtn_es_1v2_pg = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.dtn_es_1v2_pg);
    ipmc_dsm->dsm_data.dtn_es_vdda_1v_pg = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.dtn_es_vdda_1v_pg);
    ipmc_dsm->dsm_data.tps_pgood_dtn_es_vcc3v3 = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.tps_pgood_dtn_es_vcc3v3);
    ipmc_dsm->dsm_data.dtn_es_vddix_3v3_pgood = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.dtn_es_vddix_3v3_pgood);
    ipmc_dsm->dsm_data.dtn_es_2v5_pg = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.dtn_es_2v5_pg);
    ipmc_dsm->dsm_data.dtn_es_1v8_pg = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.dtn_es_1v8_pg);
    ipmc_dsm->dsm_data.dtn_es_vddi_1v8_pg = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.dtn_es_vddi_1v8_pg);
    ipmc_dsm->dsm_data.dtn_es_1v0_pg = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.dtn_es_1v0_pg);
    ipmc_dsm->dsm_data.vtt_vref_pg = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.vtt_vref_pg);
    ipmc_dsm->dsm_data.tps_pgood_cpu_s1vdd = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.tps_pgood_cpu_s1vdd);
    ipmc_dsm->dsm_data.tps_pgood_cpu_s2vdd = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.tps_pgood_cpu_s2vdd);
    ipmc_dsm->dsm_data.tpsm_pgood_cpu_vcc1v35 = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.tpsm_pgood_cpu_vcc1v35);
    ipmc_dsm->dsm_data.tpsm_pgood_cpu_vcc1v8 = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.tpsm_pgood_cpu_vcc1v8);
    ipmc_dsm->dsm_data.prog_sfp_pgood = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.prog_sfp_pgood);
    ipmc_dsm->dsm_data.tpsm_pgood_cpu_vcc1v2 = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.tpsm_pgood_cpu_vcc1v2);
    ipmc_dsm->dsm_data.dtn_es_vddi_2v5_pg = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.dtn_es_vddi_2v5_pg);

    // VALID Fields
    ipmc_dsm->dsm_data.phy_pgood_dvdd_1v_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.phy_pgood_dvdd_1v_valid);
    ipmc_dsm->dsm_data.bias_pgood_5v_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.bias_pgood_5v_valid);
    ipmc_dsm->dsm_data.refclk_vdd1v8_pg_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.refclk_vdd1v8_pg_valid);
    ipmc_dsm->dsm_data.refclk_vddo_pg_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.refclk_vddo_pg_valid);
    ipmc_dsm->dsm_data.refclk_fod_pg_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.refclk_fod_pg_valid);
    ipmc_dsm->dsm_data.refclk_vdd1_3v3_pg_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.refclk_vdd1_3v3_pg_valid);
    ipmc_dsm->dsm_data.refclk_vdd2_3v3_pg_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.refclk_vdd2_3v3_pg_valid);
    ipmc_dsm->dsm_data.onepps_vdd1v8_pg_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.onepps_vdd1v8_pg_valid);
    ipmc_dsm->dsm_data.onepps_vddo_pg_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.onepps_vddo_pg_valid);
    ipmc_dsm->dsm_data.onepps_fod_pg_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.onepps_fod_pg_valid);
    ipmc_dsm->dsm_data.onepps_vdd1_3v3_pg_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.onepps_vdd1_3v3_pg_valid);
    ipmc_dsm->dsm_data.onepps_vdd2_3v3_pg_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.onepps_vdd2_3v3_pg_valid);
    ipmc_dsm->dsm_data.dtn_es_1v2_pg_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.dtn_es_1v2_pg_valid);
    ipmc_dsm->dsm_data.dtn_es_vdda_1v_pg_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.dtn_es_vdda_1v_pg_valid);
    ipmc_dsm->dsm_data.tps_pgood_dtn_es_vcc3v3_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.tps_pgood_dtn_es_vcc3v3_valid);
    ipmc_dsm->dsm_data.dtn_es_vddix_3v3_pgood_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.dtn_es_vddix_3v3_pgood_valid);
    ipmc_dsm->dsm_data.dtn_es_2v5_pg_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.dtn_es_2v5_pg_valid);
    ipmc_dsm->dsm_data.dtn_es_1v8_pg_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.dtn_es_1v8_pg_valid);
    ipmc_dsm->dsm_data.dtn_es_vddi_1v8_pg_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.dtn_es_vddi_1v8_pg_valid);
    ipmc_dsm->dsm_data.dtn_es_1v0_pg_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.dtn_es_1v0_pg_valid);
    ipmc_dsm->dsm_data.vtt_vref_pg_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.vtt_vref_pg_valid);
    ipmc_dsm->dsm_data.tps_pgood_cpu_s1vdd_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.tps_pgood_cpu_s1vdd_valid);
    ipmc_dsm->dsm_data.tps_pgood_cpu_s2vdd_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.tps_pgood_cpu_s2vdd_valid);
    ipmc_dsm->dsm_data.tpsm_pgood_cpu_vcc1v35_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.tpsm_pgood_cpu_vcc1v35_valid);
    ipmc_dsm->dsm_data.tpsm_pgood_cpu_vcc1v8_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.tpsm_pgood_cpu_vcc1v8_valid);
    ipmc_dsm->dsm_data.prog_sfp_pgood_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.prog_sfp_pgood_valid);
    ipmc_dsm->dsm_data.tpsm_pgood_cpu_vcc1v2_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.tpsm_pgood_cpu_vcc1v2_valid);
    ipmc_dsm->dsm_data.dtn_es_vddi_2v5_pg_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.dtn_es_vddi_2v5_pg_valid);

    // Temperatures
    ipmc_dsm->dsm_data.tmp451_sensor_for_cpu.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.tmp451_sensor_for_cpu.limit_status.is_specified);
    ipmc_dsm->dsm_data.tmp451_sensor_for_cpu.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.tmp451_sensor_for_cpu.limit_status.validity_region);
    ipmc_dsm->dsm_data.tmp451_sensor_for_cpu.value = ntohs(ipmc_dsm->dsm_data.tmp451_sensor_for_cpu.value);

    ipmc_dsm->dsm_data.tmp451_sensor_for_fpga.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.tmp451_sensor_for_fpga.limit_status.is_specified);
    ipmc_dsm->dsm_data.tmp451_sensor_for_fpga.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.tmp451_sensor_for_fpga.limit_status.validity_region);
    ipmc_dsm->dsm_data.tmp451_sensor_for_fpga.value = ntohs(ipmc_dsm->dsm_data.tmp451_sensor_for_fpga.value);

    // Voltage Values
    ipmc_dsm->dsm_data.cpu_1v8.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.cpu_1v8.limit_status.is_specified);
    ipmc_dsm->dsm_data.cpu_1v8.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.cpu_1v8.limit_status.validity_region);
    ipmc_dsm->dsm_data.cpu_1v8.value = ntohs(ipmc_dsm->dsm_data.cpu_1v8.value);

    ipmc_dsm->dsm_data.cpu_1v35.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.cpu_1v35.limit_status.is_specified);
    ipmc_dsm->dsm_data.cpu_1v35.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.cpu_1v35.limit_status.validity_region);
    ipmc_dsm->dsm_data.cpu_1v35.value = ntohs(ipmc_dsm->dsm_data.cpu_1v35.value);

    ipmc_dsm->dsm_data.cpu_s1vdd.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.cpu_s1vdd.limit_status.is_specified);
    ipmc_dsm->dsm_data.cpu_s1vdd.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.cpu_s1vdd.limit_status.validity_region);
    ipmc_dsm->dsm_data.cpu_s1vdd.value = ntohs(ipmc_dsm->dsm_data.cpu_s1vdd.value);

    ipmc_dsm->dsm_data.cpu_s2vdd.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.cpu_s2vdd.limit_status.is_specified);
    ipmc_dsm->dsm_data.cpu_s2vdd.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.cpu_s2vdd.limit_status.validity_region);
    ipmc_dsm->dsm_data.cpu_s2vdd.value = ntohs(ipmc_dsm->dsm_data.cpu_s2vdd.value);

    ipmc_dsm->dsm_data.dtn_es_1v0.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.dtn_es_1v0.limit_status.is_specified);
    ipmc_dsm->dsm_data.dtn_es_1v0.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.dtn_es_1v0.limit_status.validity_region);
    ipmc_dsm->dsm_data.dtn_es_1v0.value = ntohs(ipmc_dsm->dsm_data.dtn_es_1v0.value);

    ipmc_dsm->dsm_data.dtn_es_vddi_2v5.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.dtn_es_vddi_2v5.limit_status.is_specified);
    ipmc_dsm->dsm_data.dtn_es_vddi_2v5.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.dtn_es_vddi_2v5.limit_status.validity_region);
    ipmc_dsm->dsm_data.dtn_es_vddi_2v5.value = ntohs(ipmc_dsm->dsm_data.dtn_es_vddi_2v5.value);

    ipmc_dsm->dsm_data.dtn_es_vdda_1v.value = ntohs(ipmc_dsm->dsm_data.dtn_es_vdda_1v.value);
    ipmc_dsm->dsm_data.dtn_es_vdda_1v.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.dtn_es_vdda_1v.limit_status.is_specified);
    ipmc_dsm->dsm_data.dtn_es_vdda_1v.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.dtn_es_vdda_1v.limit_status.validity_region);

    ipmc_dsm->dsm_data.dtn_es_vcc3v3.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.dtn_es_vcc3v3.limit_status.is_specified);
    ipmc_dsm->dsm_data.dtn_es_vcc3v3.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.dtn_es_vcc3v3.limit_status.validity_region);
    ipmc_dsm->dsm_data.dtn_es_vcc3v3.value = ntohs(ipmc_dsm->dsm_data.dtn_es_vcc3v3.value);

    ipmc_dsm->dsm_data.dtn_es_2v5.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.dtn_es_2v5.limit_status.is_specified);
    ipmc_dsm->dsm_data.dtn_es_2v5.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.dtn_es_2v5.limit_status.validity_region);
    ipmc_dsm->dsm_data.dtn_es_2v5.value = ntohs(ipmc_dsm->dsm_data.dtn_es_2v5.value);

    ipmc_dsm->dsm_data.dtn_es_vdd_1v8.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.dtn_es_vdd_1v8.limit_status.is_specified);
    ipmc_dsm->dsm_data.dtn_es_vdd_1v8.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.dtn_es_vdd_1v8.limit_status.validity_region);
    ipmc_dsm->dsm_data.dtn_es_vdd_1v8.value = ntohs(ipmc_dsm->dsm_data.dtn_es_vdd_1v8.value);

    ipmc_dsm->dsm_data.dtn_es_vddi_1v8.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.dtn_es_vddi_1v8.limit_status.is_specified);
    ipmc_dsm->dsm_data.dtn_es_vddi_1v8.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.dtn_es_vddi_1v8.limit_status.validity_region);
    ipmc_dsm->dsm_data.dtn_es_vddi_1v8.value = ntohs(ipmc_dsm->dsm_data.dtn_es_vddi_1v8.value);

    ipmc_dsm->dsm_data.cpu_1v2.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.cpu_1v2.limit_status.is_specified);
    ipmc_dsm->dsm_data.cpu_1v2.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.cpu_1v2.limit_status.validity_region);
    ipmc_dsm->dsm_data.cpu_1v2.value = ntohs(ipmc_dsm->dsm_data.cpu_1v2.value);

    ipmc_dsm->dsm_data.plus_1v8_1pps_vdd.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_1v8_1pps_vdd.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_1v8_1pps_vdd.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_1v8_1pps_vdd.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_1v8_1pps_vdd.value = ntohs(ipmc_dsm->dsm_data.plus_1v8_1pps_vdd.value);

    ipmc_dsm->dsm_data.plus_1v8_1pps_vddo.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_1v8_1pps_vddo.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_1v8_1pps_vddo.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_1v8_1pps_vddo.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_1v8_1pps_vddo.value = ntohs(ipmc_dsm->dsm_data.plus_1v8_1pps_vddo.value);

    ipmc_dsm->dsm_data.plus_1v8_1pps_fod.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_1v8_1pps_fod.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_1v8_1pps_fod.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_1v8_1pps_fod.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_1v8_1pps_fod.value = ntohs(ipmc_dsm->dsm_data.plus_1v8_1pps_fod.value);

    ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1.value = ntohs(ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1.value);

    ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2.value = ntohs(ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2.value);

    ipmc_dsm->dsm_data.plus_1v8_refclk_vdd.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_1v8_refclk_vdd.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_1v8_refclk_vdd.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_1v8_refclk_vdd.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_1v8_refclk_vdd.value = ntohs(ipmc_dsm->dsm_data.plus_1v8_refclk_vdd.value);

    ipmc_dsm->dsm_data.plus_1v8_refclk_vddo.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_1v8_refclk_vddo.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_1v8_refclk_vddo.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_1v8_refclk_vddo.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_1v8_refclk_vddo.value = ntohs(ipmc_dsm->dsm_data.plus_1v8_refclk_vddo.value);

    ipmc_dsm->dsm_data.plus_1v8_refclk_fod.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_1v8_refclk_fod.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_1v8_refclk_fod.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_1v8_refclk_fod.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_1v8_refclk_fod.value = ntohs(ipmc_dsm->dsm_data.plus_1v8_refclk_fod.value);

    ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1.value = ntohs(ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1.value);

    ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2.value = ntohs(ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2.value);

    // Current Values
    ipmc_dsm->dsm_data.dtn_es_1v0_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.dtn_es_1v0_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.dtn_es_1v0_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.dtn_es_1v0_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.dtn_es_1v0_cs.value = ntohs(ipmc_dsm->dsm_data.dtn_es_1v0_cs.value);

    ipmc_dsm->dsm_data.dtn_es_vddi_2v5_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.dtn_es_vddi_2v5_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.dtn_es_vddi_2v5_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.dtn_es_vddi_2v5_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.dtn_es_vddi_2v5_cs.value = ntohs(ipmc_dsm->dsm_data.dtn_es_vddi_2v5_cs.value);

    ipmc_dsm->dsm_data.dtn_es_vdda_1v_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.dtn_es_vdda_1v_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.dtn_es_vdda_1v_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.dtn_es_vdda_1v_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.dtn_es_vdda_1v_cs.value = ntohs(ipmc_dsm->dsm_data.dtn_es_vdda_1v_cs.value);

    ipmc_dsm->dsm_data.dtn_es_vcc3v3_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.dtn_es_vcc3v3_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.dtn_es_vcc3v3_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.dtn_es_vcc3v3_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.dtn_es_vcc3v3_cs.value = ntohs(ipmc_dsm->dsm_data.dtn_es_vcc3v3_cs.value);

    ipmc_dsm->dsm_data.dtn_es_2v5_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.dtn_es_2v5_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.dtn_es_2v5_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.dtn_es_2v5_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.dtn_es_2v5_cs.value = ntohs(ipmc_dsm->dsm_data.dtn_es_2v5_cs.value);

    ipmc_dsm->dsm_data.dtn_es_vdd_1v8_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.dtn_es_vdd_1v8_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.dtn_es_vdd_1v8_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.dtn_es_vdd_1v8_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.dtn_es_vdd_1v8_cs.value = ntohs(ipmc_dsm->dsm_data.dtn_es_vdd_1v8_cs.value);

    ipmc_dsm->dsm_data.dtn_es_vddi_1v8_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.dtn_es_vddi_1v8_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.dtn_es_vddi_1v8_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.dtn_es_vddi_1v8_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.dtn_es_vddi_1v8_cs.value = ntohs(ipmc_dsm->dsm_data.dtn_es_vddi_1v8_cs.value);

    ipmc_dsm->dsm_data.cpu_1v2_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.cpu_1v2_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.cpu_1v2_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.cpu_1v2_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.cpu_1v2_cs.value = ntohs(ipmc_dsm->dsm_data.cpu_1v2_cs.value);

    ipmc_dsm->dsm_data.cpu_1v8_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.cpu_1v8_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.cpu_1v8_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.cpu_1v8_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.cpu_1v8_cs.value = ntohs(ipmc_dsm->dsm_data.cpu_1v8_cs.value);

    ipmc_dsm->dsm_data.cpu_1v35_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.cpu_1v35_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.cpu_1v35_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.cpu_1v35_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.cpu_1v35_cs.value = ntohs(ipmc_dsm->dsm_data.cpu_1v35_cs.value);

    ipmc_dsm->dsm_data.cpu_s1vdd_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.cpu_s1vdd_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.cpu_s1vdd_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.cpu_s1vdd_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.cpu_s1vdd_cs.value = ntohs(ipmc_dsm->dsm_data.cpu_s1vdd_cs.value);

    ipmc_dsm->dsm_data.cpu_s2vdd_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.cpu_s2vdd_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.cpu_s2vdd_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.cpu_s2vdd_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.cpu_s2vdd_cs.value = ntohs(ipmc_dsm->dsm_data.cpu_s2vdd_cs.value);

    ipmc_dsm->dsm_data.plus_1v8_1pps_vdd_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_1v8_1pps_vdd_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_1v8_1pps_vdd_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_1v8_1pps_vdd_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_1v8_1pps_vdd_cs.value = ntohs(ipmc_dsm->dsm_data.plus_1v8_1pps_vdd_cs.value);

    ipmc_dsm->dsm_data.plus_1v8_1pps_vddo_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_1v8_1pps_vddo_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_1v8_1pps_vddo_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_1v8_1pps_vddo_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_1v8_1pps_vddo_cs.value = ntohs(ipmc_dsm->dsm_data.plus_1v8_1pps_vddo_cs.value);

    ipmc_dsm->dsm_data.plus_1v8_1pps_fod_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_1v8_1pps_fod_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_1v8_1pps_fod_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_1v8_1pps_fod_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_1v8_1pps_fod_cs.value = ntohs(ipmc_dsm->dsm_data.plus_1v8_1pps_fod_cs.value);

    ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1_cs_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1_cs_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1_cs_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1_cs_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1_cs_cs.value = ntohs(ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1_cs_cs.value);

    ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2_cs.value = ntohs(ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2_cs.value);

    ipmc_dsm->dsm_data.plus_1v8_refclk_vdd_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_1v8_refclk_vdd_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_1v8_refclk_vdd_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_1v8_refclk_vdd_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_1v8_refclk_vdd_cs.value = ntohs(ipmc_dsm->dsm_data.plus_1v8_refclk_vdd_cs.value);

    ipmc_dsm->dsm_data.plus_1v8_refclk_vddo_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_1v8_refclk_vddo_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_1v8_refclk_vddo_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_1v8_refclk_vddo_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_1v8_refclk_vddo_cs.value = ntohs(ipmc_dsm->dsm_data.plus_1v8_refclk_vddo_cs.value);

    ipmc_dsm->dsm_data.plus_1v8_refclk_fod_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_1v8_refclk_fod_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_1v8_refclk_fod_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_1v8_refclk_fod_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_1v8_refclk_fod_cs.value = ntohs(ipmc_dsm->dsm_data.plus_1v8_refclk_fod_cs.value);

    ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1_cs.value = ntohs(ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1_cs.value);

    ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2_cs.limit_status.is_specified = ntohl(ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2_cs.limit_status.is_specified);
    ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2_cs.limit_status.validity_region = ntohl(ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2_cs.limit_status.validity_region);
    ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2_cs.value = ntohs(ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2_cs.value);

    // Other Good Fields
    ipmc_dsm->dsm_data.cl_ref_lock_ind = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.cl_ref_lock_ind);
    ipmc_dsm->dsm_data.cl_ref_hldovr_stat = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.cl_ref_hldovr_stat);
    ipmc_dsm->dsm_data.cl_ref_los_ind = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.cl_ref_los_ind);
    ipmc_dsm->dsm_data.cl_1pps_lock_ind = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.cl_1pps_lock_ind);
    ipmc_dsm->dsm_data.cl_1pps_hldovr_stat = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.cl_1pps_hldovr_stat);
    ipmc_dsm->dsm_data.cl_1pps_los_ind = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.cl_1pps_los_ind);
    ipmc_dsm->dsm_data.clk_los = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.clk_los);
    ipmc_dsm->dsm_data.clk_gen_lol = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.clk_gen_lol);
    ipmc_dsm->dsm_data.ipmbb_ready = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.ipmbb_ready);
    ipmc_dsm->dsm_data.ipmba_ready = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.ipmba_ready);
    ipmc_dsm->dsm_data.mcu_asleep = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.mcu_asleep);
    ipmc_dsm->dsm_data.ddr_test = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.ddr_test);
    ipmc_dsm->dsm_data.serdes1_pll1_test = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.serdes1_pll1_test);
    ipmc_dsm->dsm_data.serdes1_pll2_test = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.serdes1_pll2_test);
    ipmc_dsm->dsm_data.ifc_nand_test = (pcs_good_type)ntohl(ipmc_dsm->dsm_data.ifc_nand_test);

    // Valid Other Fields
    ipmc_dsm->dsm_data.cl_ref_lock_ind_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.cl_ref_lock_ind_valid);
    ipmc_dsm->dsm_data.cl_ref_hldovr_stat_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.cl_ref_hldovr_stat_valid);
    ipmc_dsm->dsm_data.cl_ref_los_ind_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.cl_ref_los_ind_valid);
    ipmc_dsm->dsm_data.cl_1pps_lock_ind_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.cl_1pps_lock_ind_valid);
    ipmc_dsm->dsm_data.cl_1pps_hldovr_stat_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.cl_1pps_hldovr_stat_valid);
    ipmc_dsm->dsm_data.cl_1pps_los_ind_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.cl_1pps_los_ind_valid);
    ipmc_dsm->dsm_data.clk_los_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.clk_los_valid);
    ipmc_dsm->dsm_data.clk_gen_lol_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.clk_gen_lol_valid);
    ipmc_dsm->dsm_data.ipmbb_ready_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.ipmbb_ready_valid);
    ipmc_dsm->dsm_data.ipmba_ready_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.ipmba_ready_valid);
    ipmc_dsm->dsm_data.mcu_asleep_valid = (pcs_valid_type)ntohl(ipmc_dsm->dsm_data.mcu_asleep_valid);

    fprintf(outfp, "%d,", ipmc_dsm->generic_ipmc.LRM_dev);
    fprintf(outfp, "%u,", ipmc_dsm->generic_ipmc.is_auth);
    fprintf(outfp, "%u,", ipmc_dsm->generic_ipmc.ipmb_a);
    fprintf(outfp, "%u,", ipmc_dsm->generic_ipmc.ipmb_b);
    fprintf(outfp, "%u.%u.%u,", ipmc_dsm->generic_ipmc.fw_version_major,
            ipmc_dsm->generic_ipmc.fw_version_minor,
            ipmc_dsm->generic_ipmc.fw_version_patch);
    fprintf(outfp, "%u,", ipmc_dsm->generic_ipmc.operation_mode);
    fprintf(outfp, "%u,", ipmc_dsm->generic_ipmc.sequence_number);
    fprintf(outfp, "%u,", ipmc_dsm->generic_ipmc.power_status);
    fprintf(outfp, "%u,", ipmc_dsm->generic_ipmc.reset_counter);
    fprintf(outfp, "%u,", ipmc_dsm->generic_ipmc.health_data_refresh_counter);

    // pgood values
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.phy_pgood_dvdd_1v);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.bias_pgood_5v);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.refclk_vdd1v8_pg);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.refclk_vddo_pg);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.refclk_fod_pg);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.refclk_vdd1_3v3_pg);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.refclk_vdd2_3v3_pg);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.onepps_vdd1v8_pg);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.onepps_vddo_pg);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.onepps_fod_pg);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.onepps_vdd1_3v3_pg);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.onepps_vdd2_3v3_pg);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.dtn_es_1v2_pg);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.dtn_es_vdda_1v_pg);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.tps_pgood_dtn_es_vcc3v3);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.dtn_es_vddix_3v3_pgood);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.dtn_es_2v5_pg);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.dtn_es_1v8_pg);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.dtn_es_vddi_1v8_pg);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.dtn_es_1v0_pg);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.vtt_vref_pg);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.tps_pgood_cpu_s1vdd);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.tps_pgood_cpu_s2vdd);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.tpsm_pgood_cpu_vcc1v35);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.tpsm_pgood_cpu_vcc1v8);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.prog_sfp_pgood);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.tpsm_pgood_cpu_vcc1v2);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.dtn_es_vddi_2v5_pg);

    // valid values
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.phy_pgood_dvdd_1v_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.bias_pgood_5v_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.refclk_vdd1v8_pg_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.refclk_vddo_pg_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.refclk_fod_pg_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.refclk_vdd1_3v3_pg_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.refclk_vdd2_3v3_pg_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.onepps_vdd1v8_pg_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.onepps_vddo_pg_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.onepps_fod_pg_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.onepps_vdd1_3v3_pg_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.onepps_vdd2_3v3_pg_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.dtn_es_1v2_pg_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.dtn_es_vdda_1v_pg_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.tps_pgood_dtn_es_vcc3v3_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.dtn_es_vddix_3v3_pgood_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.dtn_es_2v5_pg_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.dtn_es_1v8_pg_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.dtn_es_vddi_1v8_pg_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.dtn_es_1v0_pg_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.vtt_vref_pg_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.tps_pgood_cpu_s1vdd_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.tps_pgood_cpu_s2vdd_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.tpsm_pgood_cpu_vcc1v35_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.tpsm_pgood_cpu_vcc1v8_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.prog_sfp_pgood_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.tpsm_pgood_cpu_vcc1v2_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.dtn_es_vddi_2v5_pg_valid);

    // signed temperature values with limit status (using string representation)
    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_dsm->dsm_data.tmp451_sensor_for_cpu.value,
        ipmc_dsm->dsm_data.tmp451_sensor_for_cpu.limit_status.is_specified < 2 ? 
            is_specified[ipmc_dsm->dsm_data.tmp451_sensor_for_cpu.limit_status.is_specified] : 
            is_specified[2],
        ipmc_dsm->dsm_data.tmp451_sensor_for_cpu.limit_status.validity_region < 4 ? 
            validity_region[ipmc_dsm->dsm_data.tmp451_sensor_for_cpu.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(outfp, "%d\\%s\\%s,", 
        ipmc_dsm->dsm_data.tmp451_sensor_for_fpga.value,
        ipmc_dsm->dsm_data.tmp451_sensor_for_fpga.limit_status.is_specified < 2 ? 
            is_specified[ipmc_dsm->dsm_data.tmp451_sensor_for_fpga.limit_status.is_specified] : 
            is_specified[2],
        ipmc_dsm->dsm_data.tmp451_sensor_for_fpga.limit_status.validity_region < 4 ? 
            validity_region[ipmc_dsm->dsm_data.tmp451_sensor_for_fpga.limit_status.validity_region] : 
            validity_region[4]);

    // unsigned voltage values with limit status
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.cpu_1v8.value, 
            ipmc_dsm->dsm_data.cpu_1v8.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.cpu_1v8.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.cpu_1v8.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.cpu_1v8.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.cpu_1v35.value, 
            ipmc_dsm->dsm_data.cpu_1v35.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.cpu_1v35.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.cpu_1v35.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.cpu_1v35.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.cpu_s1vdd.value, 
            ipmc_dsm->dsm_data.cpu_s1vdd.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.cpu_s1vdd.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.cpu_s1vdd.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.cpu_s1vdd.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.cpu_s2vdd.value, 
            ipmc_dsm->dsm_data.cpu_s2vdd.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.cpu_s2vdd.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.cpu_s2vdd.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.cpu_s2vdd.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.dtn_es_1v0.value, 
            ipmc_dsm->dsm_data.dtn_es_1v0.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.dtn_es_1v0.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.dtn_es_1v0.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.dtn_es_1v0.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.dtn_es_vddi_2v5.value, 
            ipmc_dsm->dsm_data.dtn_es_vddi_2v5.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.dtn_es_vddi_2v5.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.dtn_es_vddi_2v5.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.dtn_es_vddi_2v5.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.dtn_es_vdda_1v.value, 
            ipmc_dsm->dsm_data.dtn_es_vdda_1v.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.dtn_es_vdda_1v.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.dtn_es_vdda_1v.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.dtn_es_vdda_1v.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.dtn_es_vcc3v3.value, 
            ipmc_dsm->dsm_data.dtn_es_vcc3v3.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.dtn_es_vcc3v3.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.dtn_es_vcc3v3.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.dtn_es_vcc3v3.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.dtn_es_2v5.value, 
            ipmc_dsm->dsm_data.dtn_es_2v5.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.dtn_es_2v5.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.dtn_es_2v5.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.dtn_es_2v5.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.dtn_es_vdd_1v8.value, 
            ipmc_dsm->dsm_data.dtn_es_vdd_1v8.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.dtn_es_vdd_1v8.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.dtn_es_vdd_1v8.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.dtn_es_vdd_1v8.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.dtn_es_vddi_1v8.value, 
            ipmc_dsm->dsm_data.dtn_es_vddi_1v8.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.dtn_es_vddi_1v8.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.dtn_es_vddi_1v8.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.dtn_es_vddi_1v8.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.cpu_1v2.value, 
            ipmc_dsm->dsm_data.cpu_1v2.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.cpu_1v2.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.cpu_1v2.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.cpu_1v2.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_1v8_1pps_vdd.value, 
            ipmc_dsm->dsm_data.plus_1v8_1pps_vdd.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_1v8_1pps_vdd.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_1v8_1pps_vdd.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_1v8_1pps_vdd.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_1v8_1pps_vddo.value, 
            ipmc_dsm->dsm_data.plus_1v8_1pps_vddo.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_1v8_1pps_vddo.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_1v8_1pps_vddo.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_1v8_1pps_vddo.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_1v8_1pps_fod.value, 
            ipmc_dsm->dsm_data.plus_1v8_1pps_fod.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_1v8_1pps_fod.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_1v8_1pps_fod.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_1v8_1pps_fod.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1.value, 
            ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2.value, 
            ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_1v8_refclk_vdd.value, 
            ipmc_dsm->dsm_data.plus_1v8_refclk_vdd.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_1v8_refclk_vdd.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_1v8_refclk_vdd.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_1v8_refclk_vdd.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_1v8_refclk_vddo.value, 
            ipmc_dsm->dsm_data.plus_1v8_refclk_vddo.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_1v8_refclk_vddo.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_1v8_refclk_vddo.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_1v8_refclk_vddo.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_1v8_refclk_fod.value, 
            ipmc_dsm->dsm_data.plus_1v8_refclk_fod.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_1v8_refclk_fod.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_1v8_refclk_fod.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_1v8_refclk_fod.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1.value, 
            ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2.value, 
            ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2.limit_status.validity_region] : 
                validity_region[4]);

    // unsigned current values with limit status
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.dtn_es_1v0_cs.value, 
            ipmc_dsm->dsm_data.dtn_es_1v0_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.dtn_es_1v0_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.dtn_es_1v0_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.dtn_es_1v0_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.dtn_es_vddi_2v5_cs.value, 
            ipmc_dsm->dsm_data.dtn_es_vddi_2v5_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.dtn_es_vddi_2v5_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.dtn_es_vddi_2v5_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.dtn_es_vddi_2v5_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.dtn_es_vdda_1v_cs.value, 
            ipmc_dsm->dsm_data.dtn_es_vdda_1v_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.dtn_es_vdda_1v_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.dtn_es_vdda_1v_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.dtn_es_vdda_1v_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.dtn_es_vcc3v3_cs.value, 
            ipmc_dsm->dsm_data.dtn_es_vcc3v3_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.dtn_es_vcc3v3_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.dtn_es_vcc3v3_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.dtn_es_vcc3v3_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.dtn_es_2v5_cs.value, 
            ipmc_dsm->dsm_data.dtn_es_2v5_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.dtn_es_2v5_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.dtn_es_2v5_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.dtn_es_2v5_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.dtn_es_vdd_1v8_cs.value, 
            ipmc_dsm->dsm_data.dtn_es_vdd_1v8_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.dtn_es_vdd_1v8_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.dtn_es_vdd_1v8_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.dtn_es_vdd_1v8_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.dtn_es_vddi_1v8_cs.value, 
            ipmc_dsm->dsm_data.dtn_es_vddi_1v8_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.dtn_es_vddi_1v8_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.dtn_es_vddi_1v8_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.dtn_es_vddi_1v8_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.cpu_1v2_cs.value, 
            ipmc_dsm->dsm_data.cpu_1v2_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.cpu_1v2_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.cpu_1v2_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.cpu_1v2_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.cpu_1v8_cs.value, 
            ipmc_dsm->dsm_data.cpu_1v8_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.cpu_1v8_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.cpu_1v8_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.cpu_1v8_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.cpu_1v35_cs.value, 
            ipmc_dsm->dsm_data.cpu_1v35_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.cpu_1v35_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.cpu_1v35_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.cpu_1v35_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.cpu_s1vdd_cs.value, 
            ipmc_dsm->dsm_data.cpu_s1vdd_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.cpu_s1vdd_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.cpu_s1vdd_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.cpu_s1vdd_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.cpu_s2vdd_cs.value, 
            ipmc_dsm->dsm_data.cpu_s2vdd_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.cpu_s2vdd_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.cpu_s2vdd_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.cpu_s2vdd_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_1v8_1pps_vdd_cs.value, 
            ipmc_dsm->dsm_data.plus_1v8_1pps_vdd_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_1v8_1pps_vdd_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_1v8_1pps_vdd_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_1v8_1pps_vdd_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_1v8_1pps_vddo_cs.value, 
            ipmc_dsm->dsm_data.plus_1v8_1pps_vddo_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_1v8_1pps_vddo_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_1v8_1pps_vddo_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_1v8_1pps_vddo_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_1v8_1pps_fod_cs.value, 
            ipmc_dsm->dsm_data.plus_1v8_1pps_fod_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_1v8_1pps_fod_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_1v8_1pps_fod_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_1v8_1pps_fod_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1_cs_cs.value, 
            ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1_cs_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1_cs_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1_cs_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_3v3_1pps_vdd1_cs_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2_cs.value, 
            ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_3v3_1pps_vdd2_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_1v8_refclk_vdd_cs.value, 
            ipmc_dsm->dsm_data.plus_1v8_refclk_vdd_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_1v8_refclk_vdd_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_1v8_refclk_vdd_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_1v8_refclk_vdd_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_1v8_refclk_vddo_cs.value, 
            ipmc_dsm->dsm_data.plus_1v8_refclk_vddo_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_1v8_refclk_vddo_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_1v8_refclk_vddo_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_1v8_refclk_vddo_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_1v8_refclk_fod_cs.value, 
            ipmc_dsm->dsm_data.plus_1v8_refclk_fod_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_1v8_refclk_fod_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_1v8_refclk_fod_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_1v8_refclk_fod_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1_cs.value, 
            ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_3v3_refclk_vdd1_cs.limit_status.validity_region] : 
                validity_region[4]);
    fprintf(outfp, "%u\\%s\\%s,", ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2_cs.value, 
            ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2_cs.limit_status.is_specified < 2 ? 
                is_specified[ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2_cs.limit_status.is_specified] : 
                is_specified[2],
            ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2_cs.limit_status.validity_region < 4 ? 
                validity_region[ipmc_dsm->dsm_data.plus_3v3_refclk_vdd2_cs.limit_status.validity_region] : 
                validity_region[4]);

    // other good values
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.cl_ref_lock_ind);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.cl_ref_hldovr_stat);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.cl_ref_los_ind);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.cl_1pps_lock_ind);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.cl_1pps_hldovr_stat);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.cl_1pps_los_ind);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.clk_los);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.clk_gen_lol);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.ipmbb_ready);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.ipmba_ready);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.mcu_asleep);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.ddr_test);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.serdes1_pll1_test);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.serdes1_pll2_test);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.ifc_nand_test);

    // other valid values
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.cl_ref_lock_ind_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.cl_ref_hldovr_stat_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.cl_ref_los_ind_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.cl_1pps_lock_ind_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.cl_1pps_hldovr_stat_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.cl_1pps_los_ind_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.clk_los_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.clk_gen_lol_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.ipmbb_ready_valid);
    fprintf(outfp, "%u,", ipmc_dsm->dsm_data.ipmba_ready_valid);
    fprintf(outfp, "%u\n", ipmc_dsm->dsm_data.mcu_asleep_valid); // Final field with newline

    return 0;
}
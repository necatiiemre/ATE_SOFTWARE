#pragma once
#if 0
typedef struct pcs_hsm_board_data {
    /* pgood */
    pcs_good_type pg_3v3_vcc;
    pcs_good_type pg_hsn_3v3_sw;
    pcs_good_type pg_1v8_pex_vdda;
    pcs_good_type pg_0v9_pex_vdd;
    pcs_good_type pg_1v8_hsn;
    pcs_good_type pg_0v9_hsn_mgtavcc;
    pcs_good_type pg_1v8_vcc;
    pcs_good_type pg_0v9_pex_vdda;
    pcs_good_type pg_1v2_hsn;

    /* valid */
    pcs_valid_type pg_3v3_vcc_valid;
    pcs_valid_type pg_hsn_3v3_sw_valid;
    pcs_valid_type pg_1v8_pex_vdda_valid;
    pcs_valid_type pg_0v9_pex_vdd_valid;
    pcs_valid_type pg_1v8_hsn_valid;
    pcs_valid_type pg_0v9_hsn_mgtavcc_valid;
    pcs_valid_type pg_1v8_vcc_valid;
    pcs_valid_type pg_0v9_pex_vdda_valid;
    pcs_valid_type pg_1v2_hsn_valid;

    /* temperatures */
    Pcs_ipmc_signed_value_type tmp451_sensor_1;

    /* voltage */
    Pcs_ipmc_unsigned_value_type sns_1v8_hsn_vcc;

    /* other */
    pcs_good_type hsn_done_3v3;
    pcs_good_type pcie_refclk_los;
    pcs_good_type pcie_reflck_sel;
    pcs_good_type ipmba_buff_rdy;
    pcs_good_type ipmbb_buff_rdy;

    /* other valid */
    pcs_valid_type hsn_done_3v3_valid;
    pcs_valid_type pcie_refclk_los_valid;
    pcs_valid_type pcie_reflck_sel_valid;
    pcs_valid_type ipmba_buff_rdy_valid;
    pcs_valid_type ipmbb_buff_rdy_valid;

} pcs_hsm_board_data;
#else
typedef struct __attribute__((packed)) pcs_hsm_board_data
{
    /* pgood */
    pcs_good_type pg_0v85_hsn_vccint;
    pcs_good_type pg_3v3_vcc;
    pcs_good_type pg_hsn_3v3_sw;
    pcs_good_type pg_1v8_pex_vdda;
    pcs_good_type pg_0v9_pex_vdd;
    pcs_good_type pg_1v8_hsn;
    pcs_good_type pg_0v9_hsn_mgtavcc;
    pcs_good_type pg_1v8_vcc;
    pcs_good_type pg_0v9_pex_vdda;
    pcs_good_type pg_1v2_hsn;

    /* valid */
    pcs_valid_type pg_0v85_hsn_vccint_valid;
    pcs_valid_type pg_3v3_vcc_valid;
    pcs_valid_type pg_hsn_3v3_sw_valid;
    pcs_valid_type pg_1v8_pex_vdda_valid;
    pcs_valid_type pg_0v9_pex_vdd_valid;
    pcs_valid_type pg_1v8_hsn_valid;
    pcs_valid_type pg_0v9_hsn_mgtavcc_valid;
    pcs_valid_type pg_1v8_vcc_valid;
    pcs_valid_type pg_0v9_pex_vdda_valid;
    pcs_valid_type pg_1v2_hsn_valid;

    /* temperatures */
    Pcs_ipmc_signed_value_type tmp451_sensor;
    Pcs_ipmc_signed_value_type tmp451_sensor_1;
    Pcs_ipmc_signed_value_type tmp451_sensor_2;
    Pcs_ipmc_signed_value_type tmp451_sensor_3;
    Pcs_ipmc_signed_value_type tmp451_sensor_4;

    /* voltage */
    Pcs_ipmc_unsigned_value_type sns_0v9_hsn_mgtavcc;
    Pcs_ipmc_unsigned_value_type sns_1v8_hsn_vcc;
    Pcs_ipmc_unsigned_value_type sns_3v3_vcc;
    Pcs_ipmc_unsigned_value_type sns_1v2_hsn_vcc;
    Pcs_ipmc_unsigned_value_type sns_0v9_pex_vdd;
    Pcs_ipmc_unsigned_value_type sns_0v85_hsn_vccint;
    Pcs_ipmc_unsigned_value_type sns_1v8_pex_vdda;
    Pcs_ipmc_unsigned_value_type sns_0v9_pex_vdda;

    /* current */
    Pcs_ipmc_unsigned_value_type cs_0v85_hsn_vccint;
    Pcs_ipmc_unsigned_value_type cs_1v8_pex_vdda;
    Pcs_ipmc_unsigned_value_type cs_0v9_pex_vdda;
    Pcs_ipmc_unsigned_value_type cs_0v9_pex_vdd;
    Pcs_ipmc_unsigned_value_type cs_12v_vpx_vss;
    Pcs_ipmc_unsigned_value_type cs_5v_vpx_vss3;
    Pcs_ipmc_unsigned_value_type cs_3v3_vcc;
    Pcs_ipmc_unsigned_value_type cs_1v2_hsn_vcc;
    Pcs_ipmc_unsigned_value_type cs_1v8_hsn_vcc;
    Pcs_ipmc_unsigned_value_type cs_0v9_hsn_mgtavcc;

    /* other */
    pcs_good_type hsn_init_3v3;
    pcs_good_type hsn_done_3v3;
    pcs_good_type pcie_refclk_los;
    pcs_good_type pcie_reflck_sel;
    pcs_good_type ipmba_buff_rdy;
    pcs_good_type ipmbb_buff_rdy;

    /* other valid */
    pcs_valid_type hsn_init_3v3_valid;
    pcs_valid_type hsn_done_3v3_valid;
    pcs_valid_type pcie_refclk_los_valid;
    pcs_valid_type pcie_reflck_sel_valid;
    pcs_valid_type ipmba_buff_rdy_valid;
    pcs_valid_type ipmbb_buff_rdy_valid;
} pcs_hsm_board_data;

#endif

typedef struct {
    clcmsw_generic_ipmc_data_type generic_ipmc;
    pcs_hsm_board_data hsm_data;
} clcmsw_ipmc_hsm_data;

static void ipmc_hsm_log_header_print(FILE **fp) {
    fprintf(*fp,  "DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP, LOG PAYLOAD LENGTH,");

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

    fprintf(*fp, "%s,", "pg_0v85_hsn_vccint");
    fprintf(*fp, "%s,", "pg_3v3_vcc");
    fprintf(*fp, "%s,", "pg_hsn_3v3_sw");
    fprintf(*fp, "%s,", "pg_1v8_pex_vdda");
    fprintf(*fp, "%s,", "pg_0v9_pex_vdd");
    fprintf(*fp, "%s,", "pg_1v8_hsn");
    fprintf(*fp, "%s,", "pg_0v9_hsn_mgtavcc");
    fprintf(*fp, "%s,", "pg_1v8_vcc");
    fprintf(*fp, "%s,", "pg_0v9_pex_vdda");
    fprintf(*fp, "%s,", "pg_1v2_hsn");

    fprintf(*fp, "%s,", "pg_0v85_hsn_vccint_valid");
    fprintf(*fp, "%s,", "pg_3v3_vcc_valid");
    fprintf(*fp, "%s,", "pg_hsn_3v3_sw_valid");
    fprintf(*fp, "%s,", "pg_1v8_pex_vdda_valid");
    fprintf(*fp, "%s,", "pg_0v9_pex_vdd_valid");
    fprintf(*fp, "%s,", "pg_1v8_hsn_valid");
    fprintf(*fp, "%s,", "pg_0v9_hsn_mgtavcc_valid");
    fprintf(*fp, "%s,", "pg_1v8_vcc_valid");
    fprintf(*fp, "%s,", "pg_0v9_pex_vdda_valid");
    fprintf(*fp, "%s,", "pg_1v2_hsn_valid");

    fprintf(*fp, "%s\\is_specified\\validity_region,", "tmp451_sensor");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "tmp451_sensor_1");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "tmp451_sensor_2");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "tmp451_sensor_3");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "tmp451_sensor_4");

    fprintf(*fp, "%s\\is_specified\\validity_region,", "sns_0v9_hsn_mgtavcc");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "sns_1v8_hsn_vcc");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "sns_3v3_vcc");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "sns_1v2_hsn_vcc");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "sns_0v9_pex_vdd");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "sns_0v85_hsn_vccint");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "sns_1v8_pex_vdda");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "sns_0v9_pex_vdda");

    fprintf(*fp, "%s\\is_specified\\validity_region,", "cs_0v85_hsn_vccint");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "cs_1v8_pex_vdda");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "cs_0v9_pex_vdda");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "cs_0v9_pex_vdd");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "cs_12v_vpx_vss");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "cs_5v_vpx_vss3");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "cs_3v3_vcc");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "cs_1v2_hsn_vcc");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "cs_1v8_hsn_vcc");
    fprintf(*fp, "%s\\is_specified\\validity_region,", "cs_0v9_hsn_mgtavcc");

    fprintf(*fp, "%s,", "hsn_init_3v3");
    fprintf(*fp, "%s,", "hsn_done_3v3");
    fprintf(*fp, "%s,", "pcie_refclk_los");
    fprintf(*fp, "%s,", "pcie_reflck_sel");
    fprintf(*fp, "%s,", "ipmba_buff_rdy");
    fprintf(*fp, "%s,", "ipmbb_buff_rdy");

    fprintf(*fp, "%s,", "hsn_init_3v3_valid");
    fprintf(*fp, "%s,", "hsn_done_3v3_valid");
    fprintf(*fp, "%s,", "pcie_refclk_los_valid");
    fprintf(*fp, "%s,", "pcie_reflck_sel_valid");
    fprintf(*fp, "%s,", "ipmba_buff_rdy_valid");
    fprintf(*fp, "%s\n", "ipmbb_buff_rdy_valid");
}

static void ipmc_hsm_ntoh(clcmsw_ipmc_hsm_data *obj) {
    obj->generic_ipmc.LRM_dev = ntohl(obj->generic_ipmc.LRM_dev);
    obj->generic_ipmc.is_auth = obj->generic_ipmc.is_auth;
    obj->generic_ipmc.ipmb_a = ntohl(obj->generic_ipmc.ipmb_a);
    obj->generic_ipmc.ipmb_b = ntohl(obj->generic_ipmc.ipmb_b);
    obj->generic_ipmc.operation_mode = ntohl(obj->generic_ipmc.operation_mode);
    obj->generic_ipmc.sequence_number = ntohs(obj->generic_ipmc.sequence_number);
    obj->generic_ipmc.power_status = ntohl(obj->generic_ipmc.power_status);
    obj->generic_ipmc.reset_counter = obj->generic_ipmc.reset_counter;
    obj->generic_ipmc.health_data_refresh_counter = obj->generic_ipmc.health_data_refresh_counter;

    obj->hsm_data.pg_0v85_hsn_vccint = (pcs_good_type)ntohl(obj->hsm_data.pg_0v85_hsn_vccint);
    obj->hsm_data.pg_3v3_vcc = (pcs_good_type)ntohl(obj->hsm_data.pg_3v3_vcc);
    obj->hsm_data.pg_hsn_3v3_sw = (pcs_good_type)ntohl(obj->hsm_data.pg_hsn_3v3_sw);
    obj->hsm_data.pg_1v8_pex_vdda = (pcs_good_type)ntohl(obj->hsm_data.pg_1v8_pex_vdda);
    obj->hsm_data.pg_0v9_pex_vdd = (pcs_good_type)ntohl(obj->hsm_data.pg_0v9_pex_vdd);
    obj->hsm_data.pg_1v8_hsn = (pcs_good_type)ntohl(obj->hsm_data.pg_1v8_hsn);
    obj->hsm_data.pg_0v9_hsn_mgtavcc = (pcs_good_type)ntohl(obj->hsm_data.pg_0v9_hsn_mgtavcc);
    obj->hsm_data.pg_1v8_vcc = (pcs_good_type)ntohl(obj->hsm_data.pg_1v8_vcc);
    obj->hsm_data.pg_0v9_pex_vdda = (pcs_good_type)ntohl(obj->hsm_data.pg_0v9_pex_vdda);
    obj->hsm_data.pg_1v2_hsn = (pcs_good_type)ntohl(obj->hsm_data.pg_1v2_hsn);

    obj->hsm_data.pg_0v85_hsn_vccint_valid = (pcs_valid_type)ntohl(obj->hsm_data.pg_0v85_hsn_vccint_valid);
    obj->hsm_data.pg_3v3_vcc_valid =         (pcs_valid_type)ntohl(obj->hsm_data.pg_3v3_vcc_valid);
    obj->hsm_data.pg_hsn_3v3_sw_valid =      (pcs_valid_type)ntohl(obj->hsm_data.pg_hsn_3v3_sw_valid);
    obj->hsm_data.pg_1v8_pex_vdda_valid =    (pcs_valid_type)ntohl(obj->hsm_data.pg_1v8_pex_vdda_valid);
    obj->hsm_data.pg_0v9_pex_vdd_valid =     (pcs_valid_type)ntohl(obj->hsm_data.pg_0v9_pex_vdd_valid);
    obj->hsm_data.pg_1v8_hsn_valid =         (pcs_valid_type)ntohl(obj->hsm_data.pg_1v8_hsn_valid);
    obj->hsm_data.pg_0v9_hsn_mgtavcc_valid = (pcs_valid_type)ntohl(obj->hsm_data.pg_0v9_hsn_mgtavcc_valid);
    obj->hsm_data.pg_1v8_vcc_valid =         (pcs_valid_type)ntohl(obj->hsm_data.pg_1v8_vcc_valid);
    obj->hsm_data.pg_0v9_pex_vdda_valid =    (pcs_valid_type)ntohl(obj->hsm_data.pg_0v9_pex_vdda_valid);
    obj->hsm_data.pg_1v2_hsn_valid =         (pcs_valid_type)ntohl(obj->hsm_data.pg_1v2_hsn_valid);

    obj->hsm_data.tmp451_sensor.limit_status.is_specified = ntohl(obj->hsm_data.tmp451_sensor.limit_status.is_specified);
    obj->hsm_data.tmp451_sensor.limit_status.validity_region = ntohl(obj->hsm_data.tmp451_sensor.limit_status.validity_region);
    obj->hsm_data.tmp451_sensor.value = ntohs(obj->hsm_data.tmp451_sensor.value);
    obj->hsm_data.tmp451_sensor_1.limit_status.is_specified = ntohl(obj->hsm_data.tmp451_sensor_1.limit_status.is_specified);
    obj->hsm_data.tmp451_sensor_1.limit_status.validity_region = ntohl(obj->hsm_data.tmp451_sensor_1.limit_status.validity_region);
    obj->hsm_data.tmp451_sensor_1.value = ntohs(obj->hsm_data.tmp451_sensor_1.value);
    obj->hsm_data.tmp451_sensor_2.limit_status.is_specified = ntohl(obj->hsm_data.tmp451_sensor_2.limit_status.is_specified);
    obj->hsm_data.tmp451_sensor_2.limit_status.validity_region = ntohl(obj->hsm_data.tmp451_sensor_2.limit_status.validity_region);
    obj->hsm_data.tmp451_sensor_2.value = ntohs(obj->hsm_data.tmp451_sensor_2.value);
    obj->hsm_data.tmp451_sensor_3.limit_status.is_specified = ntohl(obj->hsm_data.tmp451_sensor_3.limit_status.is_specified);
    obj->hsm_data.tmp451_sensor_3.limit_status.validity_region = ntohl(obj->hsm_data.tmp451_sensor_3.limit_status.validity_region);
    obj->hsm_data.tmp451_sensor_3.value = ntohs(obj->hsm_data.tmp451_sensor_3.value);
    obj->hsm_data.tmp451_sensor_4.limit_status.is_specified = ntohl(obj->hsm_data.tmp451_sensor_4.limit_status.is_specified);
    obj->hsm_data.tmp451_sensor_4.limit_status.validity_region = ntohl(obj->hsm_data.tmp451_sensor_4.limit_status.validity_region);
    obj->hsm_data.tmp451_sensor_4.value = ntohs(obj->hsm_data.tmp451_sensor_4.value);

    /* voltage */

    obj->hsm_data.sns_0v9_hsn_mgtavcc.limit_status.is_specified = ntohl(obj->hsm_data.sns_0v9_hsn_mgtavcc.limit_status.is_specified);
    obj->hsm_data.sns_0v9_hsn_mgtavcc.limit_status.validity_region = ntohl(obj->hsm_data.sns_0v9_hsn_mgtavcc.limit_status.validity_region);
    obj->hsm_data.sns_0v9_hsn_mgtavcc.value = ntohs(obj->hsm_data.sns_0v9_hsn_mgtavcc.value);
    
    obj->hsm_data.sns_1v8_hsn_vcc.limit_status.is_specified = ntohl(obj->hsm_data.sns_1v8_hsn_vcc.limit_status.is_specified);
    obj->hsm_data.sns_1v8_hsn_vcc.limit_status.validity_region = ntohl(obj->hsm_data.sns_1v8_hsn_vcc.limit_status.validity_region);
    obj->hsm_data.sns_1v8_hsn_vcc.value = ntohs(obj->hsm_data.sns_1v8_hsn_vcc.value);
    
    obj->hsm_data.sns_3v3_vcc.limit_status.is_specified = ntohl(obj->hsm_data.sns_3v3_vcc.limit_status.is_specified);
    obj->hsm_data.sns_3v3_vcc.limit_status.validity_region = ntohl(obj->hsm_data.sns_3v3_vcc.limit_status.validity_region);
    obj->hsm_data.sns_3v3_vcc.value = ntohs(obj->hsm_data.sns_3v3_vcc.value);
    
    obj->hsm_data.sns_1v2_hsn_vcc.limit_status.is_specified = ntohl(obj->hsm_data.sns_1v2_hsn_vcc.limit_status.is_specified);
    obj->hsm_data.sns_1v2_hsn_vcc.limit_status.validity_region = ntohl(obj->hsm_data.sns_1v2_hsn_vcc.limit_status.validity_region);
    obj->hsm_data.sns_1v2_hsn_vcc.value = ntohs(obj->hsm_data.sns_1v2_hsn_vcc.value);
    
    obj->hsm_data.sns_0v9_pex_vdd.limit_status.is_specified = ntohl(obj->hsm_data.sns_0v9_pex_vdd.limit_status.is_specified);
    obj->hsm_data.sns_0v9_pex_vdd.limit_status.validity_region = ntohl(obj->hsm_data.sns_0v9_pex_vdd.limit_status.validity_region);
    obj->hsm_data.sns_0v9_pex_vdd.value = ntohs(obj->hsm_data.sns_0v9_pex_vdd.value);
    
    obj->hsm_data.sns_0v85_hsn_vccint.limit_status.is_specified = ntohl(obj->hsm_data.sns_0v85_hsn_vccint.limit_status.is_specified);
    obj->hsm_data.sns_0v85_hsn_vccint.limit_status.validity_region = ntohl(obj->hsm_data.sns_0v85_hsn_vccint.limit_status.validity_region);
    obj->hsm_data.sns_0v85_hsn_vccint.value = ntohs(obj->hsm_data.sns_0v85_hsn_vccint.value);
    
    obj->hsm_data.sns_1v8_pex_vdda.limit_status.is_specified = ntohl(obj->hsm_data.sns_1v8_pex_vdda.limit_status.is_specified);
    obj->hsm_data.sns_1v8_pex_vdda.limit_status.validity_region = ntohl(obj->hsm_data.sns_1v8_pex_vdda.limit_status.validity_region);
    obj->hsm_data.sns_1v8_pex_vdda.value = ntohs(obj->hsm_data.sns_1v8_pex_vdda.value);
    
    obj->hsm_data.sns_0v9_pex_vdda.limit_status.is_specified = ntohl(obj->hsm_data.sns_0v9_pex_vdda.limit_status.is_specified);
    obj->hsm_data.sns_0v9_pex_vdda.limit_status.validity_region = ntohl(obj->hsm_data.sns_0v9_pex_vdda.limit_status.validity_region);
    obj->hsm_data.sns_0v9_pex_vdda.value = ntohs(obj->hsm_data.sns_0v9_pex_vdda.value);

    /* current */
    obj->hsm_data.cs_0v85_hsn_vccint.limit_status.is_specified = ntohl(obj->hsm_data.cs_0v85_hsn_vccint.limit_status.is_specified);
    obj->hsm_data.cs_0v85_hsn_vccint.limit_status.validity_region = ntohl(obj->hsm_data.cs_0v85_hsn_vccint.limit_status.validity_region);
    obj->hsm_data.cs_0v85_hsn_vccint.value = ntohs(obj->hsm_data.cs_0v85_hsn_vccint.value);

    obj->hsm_data.cs_1v8_pex_vdda.limit_status.is_specified = ntohl(obj->hsm_data.cs_1v8_pex_vdda.limit_status.is_specified);
    obj->hsm_data.cs_1v8_pex_vdda.limit_status.validity_region = ntohl(obj->hsm_data.cs_1v8_pex_vdda.limit_status.validity_region);
    obj->hsm_data.cs_1v8_pex_vdda.value = ntohs(obj->hsm_data.cs_1v8_pex_vdda.value);

    obj->hsm_data.cs_0v9_pex_vdda.limit_status.is_specified = ntohl(obj->hsm_data.cs_0v9_pex_vdda.limit_status.is_specified);
    obj->hsm_data.cs_0v9_pex_vdda.limit_status.validity_region = ntohl(obj->hsm_data.cs_0v9_pex_vdda.limit_status.validity_region);
    obj->hsm_data.cs_0v9_pex_vdda.value = ntohs(obj->hsm_data.cs_0v9_pex_vdda.value);

    obj->hsm_data.cs_0v9_pex_vdd.limit_status.is_specified = ntohl(obj->hsm_data.cs_0v9_pex_vdd.limit_status.is_specified);
    obj->hsm_data.cs_0v9_pex_vdd.limit_status.validity_region = ntohl(obj->hsm_data.cs_0v9_pex_vdd.limit_status.validity_region);
    obj->hsm_data.cs_0v9_pex_vdd.value = ntohs(obj->hsm_data.cs_0v9_pex_vdd.value);

    obj->hsm_data.cs_12v_vpx_vss.limit_status.is_specified = ntohl(obj->hsm_data.cs_12v_vpx_vss.limit_status.is_specified);
    obj->hsm_data.cs_12v_vpx_vss.limit_status.validity_region = ntohl(obj->hsm_data.cs_12v_vpx_vss.limit_status.validity_region);
    obj->hsm_data.cs_12v_vpx_vss.value = ntohs(obj->hsm_data.cs_12v_vpx_vss.value);

    obj->hsm_data.cs_5v_vpx_vss3.limit_status.is_specified = ntohl(obj->hsm_data.cs_5v_vpx_vss3.limit_status.is_specified);
    obj->hsm_data.cs_5v_vpx_vss3.limit_status.validity_region = ntohl(obj->hsm_data.cs_5v_vpx_vss3.limit_status.validity_region);
    obj->hsm_data.cs_5v_vpx_vss3.value = ntohs(obj->hsm_data.cs_5v_vpx_vss3.value);

    obj->hsm_data.cs_3v3_vcc.limit_status.is_specified = ntohl(obj->hsm_data.cs_3v3_vcc.limit_status.is_specified);
    obj->hsm_data.cs_3v3_vcc.limit_status.validity_region = ntohl(obj->hsm_data.cs_3v3_vcc.limit_status.validity_region);
    obj->hsm_data.cs_3v3_vcc.value = ntohs(obj->hsm_data.cs_3v3_vcc.value);

    obj->hsm_data.cs_1v2_hsn_vcc.limit_status.is_specified = ntohl(obj->hsm_data.cs_1v2_hsn_vcc.limit_status.is_specified);
    obj->hsm_data.cs_1v2_hsn_vcc.limit_status.validity_region = ntohl(obj->hsm_data.cs_1v2_hsn_vcc.limit_status.validity_region);
    obj->hsm_data.cs_1v2_hsn_vcc.value = ntohs(obj->hsm_data.cs_1v2_hsn_vcc.value);

    obj->hsm_data.cs_1v8_hsn_vcc.limit_status.is_specified = ntohl(obj->hsm_data.cs_1v8_hsn_vcc.limit_status.is_specified);
    obj->hsm_data.cs_1v8_hsn_vcc.limit_status.validity_region = ntohl(obj->hsm_data.cs_1v8_hsn_vcc.limit_status.validity_region);
    obj->hsm_data.cs_1v8_hsn_vcc.value = ntohs(obj->hsm_data.cs_1v8_hsn_vcc.value);

    obj->hsm_data.cs_0v9_hsn_mgtavcc.limit_status.is_specified = ntohl(obj->hsm_data.cs_0v9_hsn_mgtavcc.limit_status.is_specified);
    obj->hsm_data.cs_0v9_hsn_mgtavcc.limit_status.validity_region = ntohl(obj->hsm_data.cs_0v9_hsn_mgtavcc.limit_status.validity_region);
    obj->hsm_data.cs_0v9_hsn_mgtavcc.value = ntohs(obj->hsm_data.cs_0v9_hsn_mgtavcc.value);
    
    /* other */
    obj->hsm_data.hsn_init_3v3 =             (pcs_good_type)ntohl(obj->hsm_data.hsn_init_3v3);
    obj->hsm_data.hsn_done_3v3 =             (pcs_good_type)ntohl(obj->hsm_data.hsn_done_3v3);
    obj->hsm_data.pcie_refclk_los =          (pcs_good_type)ntohl(obj->hsm_data.pcie_refclk_los);
    obj->hsm_data.pcie_reflck_sel =          (pcs_good_type)ntohl(obj->hsm_data.pcie_reflck_sel);
    obj->hsm_data.ipmba_buff_rdy =           (pcs_good_type)ntohl(obj->hsm_data.ipmba_buff_rdy);
    obj->hsm_data.ipmbb_buff_rdy =           (pcs_good_type)ntohl(obj->hsm_data.ipmbb_buff_rdy);

    obj->hsm_data.hsn_init_3v3_valid =       (pcs_valid_type)ntohl(obj->hsm_data.hsn_init_3v3_valid);
    obj->hsm_data.hsn_done_3v3_valid =       (pcs_valid_type)ntohl(obj->hsm_data.hsn_done_3v3_valid);
    obj->hsm_data.pcie_refclk_los_valid =    (pcs_valid_type)ntohl(obj->hsm_data.pcie_refclk_los_valid);
    obj->hsm_data.pcie_reflck_sel_valid =    (pcs_valid_type)ntohl(obj->hsm_data.pcie_reflck_sel_valid);
    obj->hsm_data.ipmba_buff_rdy_valid =     (pcs_valid_type)ntohl(obj->hsm_data.ipmba_buff_rdy_valid);
    obj->hsm_data.ipmbb_buff_rdy_valid =     (pcs_valid_type)ntohl(obj->hsm_data.ipmbb_buff_rdy_valid);
}

void ipmc_hsm_log_parser(FILE *fp, clcmsw_ipmc_hsm_data *obj) {
    ipmc_hsm_ntoh(obj);

    fprintf(fp, "%u,", obj->generic_ipmc.LRM_dev);
    fprintf(fp, "%u,", obj->generic_ipmc.is_auth);
    fprintf(fp, "%u,", obj->generic_ipmc.ipmb_a);
    fprintf(fp, "%u,", obj->generic_ipmc.ipmb_b);
    fprintf(fp, "%u,", obj->generic_ipmc.fw_version_major);
    fprintf(fp, "%u,", obj->generic_ipmc.fw_version_minor);
    fprintf(fp, "%u,", obj->generic_ipmc.fw_version_patch);
    fprintf(fp, "%u,", obj->generic_ipmc.operation_mode);
    fprintf(fp, "%u,", obj->generic_ipmc.sequence_number);
    fprintf(fp, "%u,", obj->generic_ipmc.power_status);
    fprintf(fp, "%u,", obj->generic_ipmc.reset_counter);
    fprintf(fp, "%u,", obj->generic_ipmc.health_data_refresh_counter);

    fprintf(fp, "%u,", obj->hsm_data.pg_0v85_hsn_vccint);
    fprintf(fp, "%u,", obj->hsm_data.pg_3v3_vcc);
    fprintf(fp, "%u,", obj->hsm_data.pg_hsn_3v3_sw);
    fprintf(fp, "%u,", obj->hsm_data.pg_1v8_pex_vdda);
    fprintf(fp, "%u,", obj->hsm_data.pg_0v9_pex_vdd);
    fprintf(fp, "%u,", obj->hsm_data.pg_1v8_hsn);
    fprintf(fp, "%u,", obj->hsm_data.pg_0v9_hsn_mgtavcc);
    fprintf(fp, "%u,", obj->hsm_data.pg_1v8_vcc);
    fprintf(fp, "%u,", obj->hsm_data.pg_0v9_pex_vdda);
    fprintf(fp, "%u,", obj->hsm_data.pg_1v2_hsn);

    fprintf(fp, "%u,", obj->hsm_data.pg_0v85_hsn_vccint_valid);
    fprintf(fp, "%u,", obj->hsm_data.pg_3v3_vcc_valid);
    fprintf(fp, "%u,", obj->hsm_data.pg_hsn_3v3_sw_valid);
    fprintf(fp, "%u,", obj->hsm_data.pg_1v8_pex_vdda_valid);
    fprintf(fp, "%u,", obj->hsm_data.pg_0v9_pex_vdd_valid);
    fprintf(fp, "%u,", obj->hsm_data.pg_1v8_hsn_valid);
    fprintf(fp, "%u,", obj->hsm_data.pg_0v9_hsn_mgtavcc_valid);
    fprintf(fp, "%u,", obj->hsm_data.pg_1v8_vcc_valid);
    fprintf(fp, "%u,", obj->hsm_data.pg_0v9_pex_vdda_valid);
    fprintf(fp, "%u,", obj->hsm_data.pg_1v2_hsn_valid);

    //fprintf(fp, "%u,", obj->hsm_data.tmp451_sensor_1.value);
    //fprintf(fp, "%u,", obj->hsm_data.tmp451_sensor_1.limit_status.is_specified);
    //fprintf(fp, "%u,", obj->hsm_data.tmp451_sensor_1.limit_status.validity_region);

    fprintf(fp, "%d\\%s\\%s,", 
        obj->hsm_data.tmp451_sensor.value,
        obj->hsm_data.tmp451_sensor.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.tmp451_sensor.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.tmp451_sensor.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.tmp451_sensor.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(fp, "%d\\%s\\%s,", 
        obj->hsm_data.tmp451_sensor_1.value,
        obj->hsm_data.tmp451_sensor_1.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.tmp451_sensor_1.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.tmp451_sensor_1.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.tmp451_sensor_1.limit_status.validity_region] : 
            validity_region[4]);
    
    fprintf(fp, "%d\\%s\\%s,", 
        obj->hsm_data.tmp451_sensor_2.value,
        obj->hsm_data.tmp451_sensor_2.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.tmp451_sensor_2.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.tmp451_sensor_2.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.tmp451_sensor_2.limit_status.validity_region] : 
            validity_region[4]);
    
    fprintf(fp, "%d\\%s\\%s,", 
        obj->hsm_data.tmp451_sensor_3.value,
        obj->hsm_data.tmp451_sensor_3.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.tmp451_sensor_3.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.tmp451_sensor_3.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.tmp451_sensor_3.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(fp, "%d\\%s\\%s,", 
        obj->hsm_data.tmp451_sensor_4.value,
        obj->hsm_data.tmp451_sensor_4.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.tmp451_sensor_4.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.tmp451_sensor_4.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.tmp451_sensor_4.limit_status.validity_region] : 
            validity_region[4]);
    
    /* voltage */

    fprintf(fp, "%u\\%s\\%s,", 
        obj->hsm_data.sns_0v9_hsn_mgtavcc.value,
        obj->hsm_data.sns_0v9_hsn_mgtavcc.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.sns_0v9_hsn_mgtavcc.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.sns_0v9_hsn_mgtavcc.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.sns_0v9_hsn_mgtavcc.limit_status.validity_region] : 
            validity_region[4]);
    
    fprintf(fp, "%u\\%s\\%s,", 
        obj->hsm_data.sns_1v8_hsn_vcc.value,
        obj->hsm_data.sns_1v8_hsn_vcc.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.sns_1v8_hsn_vcc.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.sns_1v8_hsn_vcc.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.sns_1v8_hsn_vcc.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(fp, "%u\\%s\\%s,", 
        obj->hsm_data.sns_3v3_vcc.value,
        obj->hsm_data.sns_3v3_vcc.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.sns_3v3_vcc.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.sns_3v3_vcc.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.sns_3v3_vcc.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(fp, "%u\\%s\\%s,", 
        obj->hsm_data.sns_1v2_hsn_vcc.value,
        obj->hsm_data.sns_1v2_hsn_vcc.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.sns_1v2_hsn_vcc.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.sns_1v2_hsn_vcc.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.sns_1v2_hsn_vcc.limit_status.validity_region] : 
            validity_region[4]);
    
    fprintf(fp, "%u\\%s\\%s,", 
        obj->hsm_data.sns_0v9_pex_vdd.value,
        obj->hsm_data.sns_0v9_pex_vdd.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.sns_0v9_pex_vdd.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.sns_0v9_pex_vdd.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.sns_0v9_pex_vdd.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(fp, "%u\\%s\\%s,", 
        obj->hsm_data.sns_0v85_hsn_vccint.value,
        obj->hsm_data.sns_0v85_hsn_vccint.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.sns_0v85_hsn_vccint.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.sns_0v85_hsn_vccint.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.sns_0v85_hsn_vccint.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(fp, "%u\\%s\\%s,", 
        obj->hsm_data.sns_1v8_pex_vdda.value,
        obj->hsm_data.sns_1v8_pex_vdda.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.sns_1v8_pex_vdda.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.sns_1v8_pex_vdda.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.sns_1v8_pex_vdda.limit_status.validity_region] : 
            validity_region[4]);
    
    fprintf(fp, "%u\\%s\\%s,", 
        obj->hsm_data.sns_0v9_pex_vdda.value,
        obj->hsm_data.sns_0v9_pex_vdda.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.sns_0v9_pex_vdda.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.sns_0v9_pex_vdda.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.sns_0v9_pex_vdda.limit_status.validity_region] : 
            validity_region[4]);

    /* current */

    fprintf(fp, "%u\\%s\\%s,", 
        obj->hsm_data.cs_0v85_hsn_vccint.value,
        obj->hsm_data.cs_0v85_hsn_vccint.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.cs_0v85_hsn_vccint.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.cs_0v85_hsn_vccint.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.cs_0v85_hsn_vccint.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(fp, "%u\\%s\\%s,", 
        obj->hsm_data.cs_1v8_pex_vdda.value,
        obj->hsm_data.cs_1v8_pex_vdda.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.cs_1v8_pex_vdda.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.cs_1v8_pex_vdda.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.cs_1v8_pex_vdda.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(fp, "%u\\%s\\%s,", 
        obj->hsm_data.cs_0v9_pex_vdda.value,
        obj->hsm_data.cs_0v9_pex_vdda.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.cs_0v9_pex_vdda.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.cs_0v9_pex_vdda.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.cs_0v9_pex_vdda.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(fp, "%u\\%s\\%s,", 
        obj->hsm_data.cs_0v9_pex_vdd.value,
        obj->hsm_data.cs_0v9_pex_vdd.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.cs_0v9_pex_vdd.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.cs_0v9_pex_vdd.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.cs_0v9_pex_vdd.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(fp, "%u\\%s\\%s,", 
        obj->hsm_data.cs_12v_vpx_vss.value,
        obj->hsm_data.cs_12v_vpx_vss.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.cs_12v_vpx_vss.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.cs_12v_vpx_vss.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.cs_12v_vpx_vss.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(fp, "%u\\%s\\%s,", 
        obj->hsm_data.cs_5v_vpx_vss3.value,
        obj->hsm_data.cs_5v_vpx_vss3.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.cs_5v_vpx_vss3.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.cs_5v_vpx_vss3.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.cs_5v_vpx_vss3.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(fp, "%u\\%s\\%s,", 
        obj->hsm_data.cs_3v3_vcc.value,
        obj->hsm_data.cs_3v3_vcc.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.cs_3v3_vcc.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.cs_3v3_vcc.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.cs_3v3_vcc.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(fp, "%u\\%s\\%s,", 
        obj->hsm_data.cs_1v2_hsn_vcc.value,
        obj->hsm_data.cs_1v2_hsn_vcc.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.cs_1v2_hsn_vcc.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.cs_1v2_hsn_vcc.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.cs_1v2_hsn_vcc.limit_status.validity_region] : 
            validity_region[4]);

    fprintf(fp, "%u\\%s\\%s,", 
        obj->hsm_data.cs_1v8_hsn_vcc.value,
        obj->hsm_data.cs_1v8_hsn_vcc.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.cs_1v8_hsn_vcc.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.cs_1v8_hsn_vcc.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.cs_1v8_hsn_vcc.limit_status.validity_region] : 
            validity_region[4]);
        
    fprintf(fp, "%u\\%s\\%s,", 
        obj->hsm_data.cs_0v9_hsn_mgtavcc.value,
        obj->hsm_data.cs_0v9_hsn_mgtavcc.limit_status.is_specified < 2 ? 
            is_specified[obj->hsm_data.cs_0v9_hsn_mgtavcc.limit_status.is_specified] : 
            is_specified[2],
        obj->hsm_data.cs_0v9_hsn_mgtavcc.limit_status.validity_region < 4 ? 
            validity_region[obj->hsm_data.cs_0v9_hsn_mgtavcc.limit_status.validity_region] : 
            validity_region[4]);


    fprintf(fp, "%u,", obj->hsm_data.hsn_init_3v3);
    fprintf(fp, "%u,", obj->hsm_data.hsn_done_3v3);
    fprintf(fp, "%u,", obj->hsm_data.pcie_refclk_los);
    fprintf(fp, "%u,", obj->hsm_data.pcie_reflck_sel);
    fprintf(fp, "%u,", obj->hsm_data.ipmba_buff_rdy);
    fprintf(fp, "%u,", obj->hsm_data.ipmbb_buff_rdy);

    fprintf(fp, "%u,", obj->hsm_data.hsn_init_3v3_valid);
    fprintf(fp, "%u,", obj->hsm_data.hsn_done_3v3_valid);
    fprintf(fp, "%u,", obj->hsm_data.pcie_refclk_los_valid);
    fprintf(fp, "%u,", obj->hsm_data.pcie_reflck_sel_valid);
    fprintf(fp, "%u,", obj->hsm_data.ipmba_buff_rdy_valid);
    fprintf(fp, "%u\n", obj->hsm_data.ipmbb_buff_rdy_valid);
}
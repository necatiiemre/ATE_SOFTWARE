#pragma once
typedef struct __attribute__((packed)) pcs_hum_board_data
{
    /* pgood */
    pcs_good_type mcu_vin_ok;
    pcs_good_type mcu_holdup_ok;
    pcs_good_type fail_mcu;

    /* valid */
    pcs_valid_type mcu_vin_ok_valid;
    pcs_valid_type mcu_holdup_ok_valid;
    pcs_valid_type fail_mcu_valid;

    /* temperatures */
    Pcs_ipmc_signed_value_type temp_1;
    Pcs_ipmc_signed_value_type temp_2;

    /* voltage */
    Pcs_ipmc_unsigned_value_type inp_v_meas;
    Pcs_ipmc_unsigned_value_type holdp_v_meas;
    Pcs_ipmc_unsigned_value_type outp_v_meas;

    /* current */
    Pcs_ipmc_unsigned_value_type inp_cur_meas;
    Pcs_ipmc_unsigned_value_type holdp_cur_meas;
    Pcs_ipmc_unsigned_value_type outp_cur_meas;

    /* other */

    /* other valid */
} pcs_hum_board_data;

typedef struct{
	clcmsw_generic_ipmc_data_type generic_ipmc;
	pcs_hum_board_data hum_data;
}clcmsw_ipmc_hum_data;

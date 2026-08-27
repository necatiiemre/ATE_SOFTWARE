#pragma once
typedef struct __attribute__((packed)) pcs_psm_board_data
{
    /* pgood */

    /* valid */

    /* temperatures */
    Pcs_ipmc_signed_value_type tmp175_sensor_1;
    Pcs_ipmc_signed_value_type tmp175_sensor_2;
    Pcs_ipmc_signed_value_type tmp175_sensor_3;
    Pcs_ipmc_signed_value_type tmp175_sensor_4;
    Pcs_ipmc_signed_value_type tmp175_sensor_5;

    /* voltage */
    Pcs_ipmc_unsigned_value_type plus_3v3aux_voltage;
    Pcs_ipmc_unsigned_value_type plus_5v_voltage;
    Pcs_ipmc_unsigned_value_type plus_12v_voltage;
    Pcs_ipmc_unsigned_value_type mcotsc27012ft_volt;
    Pcs_ipmc_unsigned_value_type inp_vlt;
    Pcs_ipmc_unsigned_value_type inp_vlt_n;

    /* current */
    Pcs_ipmc_unsigned_value_type plus_3v3aux_current;
    Pcs_ipmc_unsigned_value_type plus_5v_current;
    Pcs_ipmc_unsigned_value_type plus_12v_current;
    Pcs_ipmc_unsigned_value_type inp_cur_p;
    Pcs_ipmc_unsigned_value_type inp_cur_n;

    /* other */
    pcs_good_type input_cur_alrt;
    pcs_good_type plus_3v3_over_cur_alrt;
    pcs_good_type plus_5v_over_cur_al;
    pcs_good_type plus_12v_over_cur_alrt;

    /* other valid */

} pcs_psm_board_data;

typedef struct {
    clcmsw_generic_ipmc_data_type generic_ipmc;
    pcs_psm_board_data hsm_data;
} clcmsw_ipmc_psm_data;



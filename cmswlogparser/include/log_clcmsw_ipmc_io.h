#pragma once
typedef struct __attribute__((packed)) pcs_io_board_data
{
    /* pgood */
    pcs_good_type pg_3v3_main;
    pcs_good_type pg_2v5_ethsw;
    pcs_good_type pg_1v2_main;
    pcs_good_type pg_1v0_ethsw;
    pcs_good_type pg_1v8_dvi;
    pcs_good_type pg_1v8_dvi_xmc;
    pcs_good_type pg_disc;

    /* valid */
    pcs_valid_type pg_3v3_main_valid;
    pcs_valid_type pg_2v5_ethsw_valid;
    pcs_valid_type pg_1v2_main_valid;
    pcs_valid_type pg_1v0_ethsw_valid;
    pcs_valid_type pg_1v8_dvi_valid;
    pcs_valid_type pg_1v8_dvi_xmc_valid;
    pcs_valid_type disc_valid;

    /* temperatures */
    Pcs_ipmc_signed_value_type tmp451_sensor;

    /* voltage */
    Pcs_ipmc_unsigned_value_type volsns_3v3;
    Pcs_ipmc_unsigned_value_type volsns_2v5;
    Pcs_ipmc_unsigned_value_type volsns_1v0;
    Pcs_ipmc_unsigned_value_type volsns_1v2;

    /* current */
    Pcs_ipmc_unsigned_value_type cursns_3v3;
    Pcs_ipmc_unsigned_value_type cursns_2v5;
    Pcs_ipmc_unsigned_value_type cursns_12v;
    Pcs_ipmc_unsigned_value_type cursns_1v;
    Pcs_ipmc_unsigned_value_type cursns_1v2;
    Pcs_ipmc_unsigned_value_type cursns_5v;

    /* other */
    pcs_good_type mpresent;
    pcs_good_type ipmi_a_ready;
    pcs_good_type ipmi_b_ready;

    /* other valid */
    pcs_valid_type mpresent_valid;
    pcs_valid_type ipmi_a_ready_valid;
    pcs_valid_type ipmi_b_ready_valid;
} pcs_io_board_data;

typedef struct{
	clcmsw_generic_ipmc_data_type generic_ipmc;
	pcs_io_board_data io_data;
}clcmsw_ipmc_io_data;


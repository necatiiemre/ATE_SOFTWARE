#pragma once
typedef struct{
	clcmsw_generic_ipmc_data_type generic_ipmc;
	/* pcs_smmm_board_data smmm_data; */
}clcmsw_ipmc_smmm_data;

static void ipmc_smmm_log_header_print(FILE **fp)
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
                fprintf(*fp, "%s\n", "health_data_refresh_counter");
}

static int ipmc_smmm_log_parser(FILE *outfp, clcmsw_ipmc_smmm_data *ipmc_smmm) {

    ipmc_smmm->generic_ipmc.LRM_dev = ntohl(ipmc_smmm->generic_ipmc.LRM_dev);
    ipmc_smmm->generic_ipmc.is_auth = ipmc_smmm->generic_ipmc.is_auth;
    ipmc_smmm->generic_ipmc.ipmb_a = ntohl(ipmc_smmm->generic_ipmc.ipmb_a);
    ipmc_smmm->generic_ipmc.ipmb_b = ntohl(ipmc_smmm->generic_ipmc.ipmb_b);
    ipmc_smmm->generic_ipmc.operation_mode = ntohl(ipmc_smmm->generic_ipmc.operation_mode);
    ipmc_smmm->generic_ipmc.sequence_number = ntohs(ipmc_smmm->generic_ipmc.sequence_number);
    ipmc_smmm->generic_ipmc.power_status = ntohl(ipmc_smmm->generic_ipmc.power_status);
    ipmc_smmm->generic_ipmc.reset_counter = ipmc_smmm->generic_ipmc.reset_counter;
    ipmc_smmm->generic_ipmc.health_data_refresh_counter = ipmc_smmm->generic_ipmc.health_data_refresh_counter;

    fprintf(outfp, "%d,", ipmc_smmm->generic_ipmc.LRM_dev);
    fprintf(outfp, "%u,", ipmc_smmm->generic_ipmc.is_auth);
    fprintf(outfp, "%u,", ipmc_smmm->generic_ipmc.ipmb_a);
    fprintf(outfp, "%u,", ipmc_smmm->generic_ipmc.ipmb_b);
    fprintf(outfp, "%u.%u.%u,", ipmc_smmm->generic_ipmc.fw_version_major,
            ipmc_smmm->generic_ipmc.fw_version_minor,
            ipmc_smmm->generic_ipmc.fw_version_patch);
    fprintf(outfp, "%u,", ipmc_smmm->generic_ipmc.operation_mode);
    fprintf(outfp, "%u,", ipmc_smmm->generic_ipmc.sequence_number);
    fprintf(outfp, "%u,", ipmc_smmm->generic_ipmc.power_status);
    fprintf(outfp, "%u,", ipmc_smmm->generic_ipmc.reset_counter);
    fprintf(outfp, "%u\n", ipmc_smmm->generic_ipmc.health_data_refresh_counter);

    return 0;
}
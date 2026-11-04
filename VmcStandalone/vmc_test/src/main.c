/**
 * vmc_test - server-side VMC test application (raw socket, no DPDK)
 *
 * Built and run on the ATE server, like dpdk_vmc used to be.
 * Empty skeleton for now - content will be added.
 */

#include <stdio.h>

#include "Version.h"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    printf("vmc_test v%s\n", Version);

    return 0;
}

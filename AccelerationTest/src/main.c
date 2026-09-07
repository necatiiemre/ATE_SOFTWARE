/*
 * Acceleration test rig.
 *
 * Pick a unit, run its acceleration test, come back to the menu. Only the DTN
 * test exists so far; VMC and CMC are registered so their menu entries and
 * wiring are already in place when those tests are written.
 */

#include "AppConfig.h"
#include "SafeShutdown.h"
#include "UnitManager.h"

#include <stdio.h>
#include <string.h>

static void banner(void)
{
    puts("======================================");
    puts("  Acceleration Test");
    puts("======================================");
}

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--keep-management") == 0)
            app_config_set_management_vls(true);
        else if (strcmp(argv[i], "--all-ports") == 0)
            app_config_set_all_ports(true);
        else
            printf("Ignoring unknown argument '%s'\n", argv[i]);
    }

    safe_shutdown_install();
    banner();
    printf("VL table: %s\n",
           app_config_management_vls()
               ? "round + the DTN's own management VLs (--keep-management)"
               : "round only, as captured");

    for (;;) {
        const unit_t *unit = unit_manager_select();
        if (!unit) {
            puts("\nBye.");
            return 0;
        }

        printf("\n--- %s acceleration test ---\n", unit->name);
        unit_result_t result = unit_manager_run(unit);
        printf("\n%s acceleration test: %s\n", unit->name, unit_result_to_string(result));
    }
}

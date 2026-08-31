/*
 * Acceleration test rig.
 *
 * Pick a unit, run its acceleration test, come back to the menu. Only the DTN
 * test exists so far; VMC and CMC are registered so their menu entries and
 * wiring are already in place when those tests are written.
 */

#include "SafeShutdown.h"
#include "UnitManager.h"

#include <stdio.h>

static void banner(void)
{
    puts("======================================");
    puts("  Acceleration Test");
    puts("======================================");
}

int main(void)
{
    safe_shutdown_install();
    banner();

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

#include "UnitManager.h"
#include "Prompt.h"

#include "units/CmcTest.h"
#include "units/DtnTest.h"
#include "units/VmcTest.h"

#include <stdio.h>

/* Menu order follows the main ATE software: CMC, VMC, DTN. */
static const unit_t g_units[] = {
    {"CMC", "Control Management Computer", false, cmc_test_run},
    {"VMC", "Vehicle Management Computer",  false, vmc_test_run},
    {"DTN", "Data Transfer Node switch",    true,  dtn_test_run},
};

const unit_t *unit_manager_units(size_t *count)
{
    if (count)
        *count = sizeof g_units / sizeof g_units[0];
    return g_units;
}

const char *unit_result_to_string(unit_result_t result)
{
    switch (result) {
    case UNIT_RESULT_PASS:    return "PASS";
    case UNIT_RESULT_FAIL:    return "FAIL";
    case UNIT_RESULT_ABORTED: return "ABORTED";
    case UNIT_RESULT_ERROR:   return "ERROR";
    }
    return "UNKNOWN";
}

const unit_t *unit_manager_select(void)
{
    size_t count;
    const unit_t *units = unit_manager_units(&count);

    puts("\nSelect unit to test");
    for (size_t i = 0; i < count; i++)
        printf("  %zu) %-4s %s%s\n", i + 1, units[i].name, units[i].description,
               units[i].implemented ? "" : "  -- test not written yet");
    puts("  0) Quit");

    int choice = prompt_menu("Choice", 0, (int)count, 0);
    return choice == 0 ? NULL : &units[choice - 1];
}

unit_result_t unit_manager_run(const unit_t *unit)
{
    if (!unit->implemented || !unit->run) {
        printf("\nThe %s acceleration test has not been written yet.\n", unit->name);
        return UNIT_RESULT_ABORTED;
    }
    return unit->run();
}

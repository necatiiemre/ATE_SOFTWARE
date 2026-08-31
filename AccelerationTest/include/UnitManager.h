/**
 * @file UnitManager.h
 * @brief The registry of testable units and the operator's choice between them.
 */

#ifndef UNIT_MANAGER_H
#define UNIT_MANAGER_H

#include "Unit.h"

#include <stddef.h>

/** All units known to the application, in menu order. */
const unit_t *unit_manager_units(size_t *count);

/**
 * @brief Ask the operator which unit to test.
 * @return the chosen unit, or NULL if they chose to quit
 */
const unit_t *unit_manager_select(void);

/** Run a unit's acceleration test, reporting units that have none yet. */
unit_result_t unit_manager_run(const unit_t *unit);

#endif /* UNIT_MANAGER_H */

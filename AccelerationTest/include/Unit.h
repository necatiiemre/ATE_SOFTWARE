/**
 * @file Unit.h
 * @brief One testable unit of the acceleration test rig.
 *
 * Every unit exposes the same shape, so main() does not need to know which one
 * it is running. Adding a unit means writing its run() and registering it in
 * UnitManager.c - nothing else changes.
 */

#ifndef UNIT_H
#define UNIT_H

#include <stdbool.h>

/** Result of a unit's acceleration test. */
typedef enum {
    UNIT_RESULT_PASS = 0,   /**< test ran and everything checked out */
    UNIT_RESULT_FAIL,       /**< test ran and something failed */
    UNIT_RESULT_ABORTED,    /**< the operator backed out */
    UNIT_RESULT_ERROR       /**< the test could not run at all */
} unit_result_t;

typedef struct {
    const char   *name;         /**< "DTN", "VMC", "CMC" */
    const char   *description;  /**< one line, shown in the menu */
    bool          implemented;  /**< false while the test is still to be written */
    unit_result_t (*run)(void); /**< NULL when !implemented */
} unit_t;

const char *unit_result_to_string(unit_result_t result);

#endif /* UNIT_H */

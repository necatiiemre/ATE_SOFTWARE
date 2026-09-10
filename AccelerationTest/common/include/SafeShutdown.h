/**
 * @file SafeShutdown.h
 * @brief Releases whatever the test switched on, however the test ends.
 *
 * Anything that must be undone - a PSU output, an open socket, a log file -
 * registers an action here as soon as it is switched on and unregisters when it
 * is switched off normally. On Ctrl-C, on an error path, or at exit, the
 * registered actions run in descending priority, so the PSU output drops before
 * anything else. CMC runs at 270 V; this is not optional.
 *
 * The signal handler only raises a flag. Blocking waits use timeouts and poll
 * safe_shutdown_requested(), which keeps the actual teardown - TCP writes to the
 * PSU among them - on the normal call stack where it is safe.
 */

#ifndef SAFE_SHUTDOWN_H
#define SAFE_SHUTDOWN_H

#include <stdbool.h>

/* Descending order of urgency. */
#define SHUTDOWN_PRIO_PSU_OUTPUT   100  /**< electrical safety comes first */
#define SHUTDOWN_PRIO_PSU_CONNECT   90
#define SHUTDOWN_PRIO_SOCKET        50
#define SHUTDOWN_PRIO_LOG           10

typedef void (*shutdown_action_fn)(void *ctx);

/** Catch SIGINT and SIGTERM, and arrange teardown at exit. */
void safe_shutdown_install(void);

/** True once the operator has asked to stop. */
bool safe_shutdown_requested(void);

/** Clear the stop request, so a finished test can return to the menu. */
void safe_shutdown_clear(void);

/**
 * @brief Register something that must be released.
 * @return a handle for safe_shutdown_unregister, or -1 if the table is full
 */
int safe_shutdown_register(const char *name, int priority,
                           shutdown_action_fn action, void *ctx);

/** Drop a registration after releasing the resource normally. */
void safe_shutdown_unregister(int handle);

/** Run every registered action, most urgent first. Idempotent, best-effort. */
void safe_shutdown_execute(void);

#endif /* SAFE_SHUTDOWN_H */

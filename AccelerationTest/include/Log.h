/**
 * @file Log.h
 * @brief Timestamped run log, written to the terminal and to a file at once.
 *
 * Every line is flushed as it is written. An acceleration run is long and can
 * end with the rig cutting power to the workstation; whatever was observed up
 * to that moment has to survive.
 */

#ifndef LOG_H
#define LOG_H

#include <stdbool.h>

/**
 * @brief Start a run log under LOGS/<unit>/<timestamp>/.
 * @return false if the directory or file could not be created; the test can
 *         still run, it just will not be recorded
 */
bool log_open(const char *unit, const char *profile);

/** Path of the open log file, or "" when there is none. */
const char *log_path(void);

/** Write one timestamped line to the terminal and the log. */
void log_line(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/** Write to the log only - for detail that would clutter the live display. */
void log_file_only(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

void log_close(void);

#endif /* LOG_H */

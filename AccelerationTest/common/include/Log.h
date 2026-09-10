/**
 * @file Log.h
 * @brief The run log: a transcript of the terminal, and nothing else.
 *
 * Once a log is open, everything the application prints goes to the file as
 * well as to the screen - the live tables, the banners, the health-monitor
 * dashboards, all of it, in the order it appeared. Nothing is written to the
 * file that was not on the screen, and nothing is shown on the screen that does
 * not reach the file, so reading the log afterwards is reading the run.
 *
 * Every line is flushed as it is written. An acceleration run is long and can
 * end with the rig cutting power to the workstation; whatever was observed up
 * to that moment has to survive.
 */

#ifndef LOG_H
#define LOG_H

#include <stdbool.h>

/**
 * @brief Start a run log under LOGS/<unit>/<profile>_<timestamp>.log.
 *
 * From here until log_close, stdout is a tee: printf, puts and everything built
 * on them reach both the terminal and the file.
 *
 * @return false if the directory or file could not be created; the test can
 *         still run, it just will not be recorded
 */
bool log_open(const char *unit, const char *profile);

/** Path of the open log file, or "" when there is none. */
const char *log_path(void);

/**
 * @brief Print one timestamped line.
 *
 * An ordinary print, so it lands on the terminal and - because the log is a
 * transcript of the terminal - in the log with everything else.
 */
void log_line(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/** Stop teeing and close the file. */
void log_close(void);

#endif /* LOG_H */

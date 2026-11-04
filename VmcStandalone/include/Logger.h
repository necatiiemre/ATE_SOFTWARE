#ifndef VMC_STANDALONE_LOGGER_H
#define VMC_STANDALONE_LOGGER_H

#include <string>

/**
 * @brief Session log file for one vmcStandalone run.
 *
 * logx::start() redirects the process's stdout/stderr (file descriptors 1/2)
 * into a pipe. A background thread reads that pipe and writes every byte
 * both to the terminal (unchanged, live) and to the log file (each line
 * prefixed with a timestamp, colour codes and '\r' removed).
 *
 * Because it works at the file-descriptor level, the log also captures the
 * output of child processes - ssh/scp errors and, most importantly, the live
 * output of vmc_test running on the server (ssh -t) - exactly as the
 * operator saw it.
 *
 * logx::fileOnly() adds detail lines that go to the file only (every SSH
 * command, its output and exit code). They travel through the same pipe, so
 * their order relative to console output is preserved.
 */
namespace logx {

/**
 * @brief Create the log file and start capturing.
 * @param dir     Directory for this run (created if missing)
 * @param file    File name inside dir
 * @param header  Plain text written at the very top of the file
 * @return false if the file cannot be created (the app keeps running
 *         without a log file)
 */
bool start(const std::string& dir, const std::string& file, const std::string& header);

/** @brief Flush, restore stdout/stderr and close the file. Idempotent. */
void stop();

bool active();
std::string dir();        // run directory ("" if not active)
std::string filePath();   // full log file path ("" if not active)

/** @brief Lines that go to the log file only (multi-line text is fine). */
void fileOnly(const std::string& text);

/**
 * @brief Detail message: on the console only in --verbose mode (and then
 *        also in the file via the console capture), otherwise file only.
 * @param console_ok false = never on the console, even in verbose mode
 */
void detail(const std::string& text, bool console_ok = true);

/** @brief Register a secret (password) that must never appear in the log. */
void addSecret(const std::string& secret);

/** @brief Replace registered secrets with "****". */
std::string mask(const std::string& text);

/** @brief "2026-11-04 10:00:00" */
std::string timestamp();

/** @brief "2026-11-04_10-00-00" (safe for directory names) */
std::string fileSafeTimestamp();

} // namespace logx

#endif // VMC_STANDALONE_LOGGER_H

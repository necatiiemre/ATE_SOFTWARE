#ifndef VMC_STANDALONE_LOG_H
#define VMC_STANDALONE_LOG_H

#include <iostream>
#include <sstream>
#include <string>

#include "Logger.h"

// ============================================================================
// Logging helpers for VmcStandalone.
//
// MainSoftware uses a compile-time DEBUG_LOG macro (Utils.h, DEBUG_PRINT=0).
// Here detail messages ALWAYS go to the session log file (Logger.h), and
// also to the console when --verbose is given.
// ============================================================================

namespace logcfg {
    // Set from main() (--verbose). Defined in main.cpp.
    extern bool verbose;
}

// Detail message: log file always, console only with --verbose.
#define VERBOSE_LOG(msg) \
    do { std::ostringstream vlog_ss_; vlog_ss_ << msg; logx::detail(vlog_ss_.str()); } while (0)

// Detail message for the log file only (never on the console).
#define FILE_LOG(msg) \
    do { std::ostringstream vlog_ss_; vlog_ss_ << msg; logx::detail(vlog_ss_.str(), false); } while (0)

/**
 * @brief Consistent, categorized error/warning/info messages
 *        (same format as MainSoftware's ErrorPrinter).
 *
 *   [ERROR][CUMULUS] ...
 *   [WARN][SSH]      ...
 *   [INFO][SYSTEM]   ...
 */
class ErrorPrinter {
public:
    static void error(const std::string& category, const std::string& message) {
        std::cerr << "[ERROR][" << category << "] " << message << std::endl;
    }

    static void warn(const std::string& category, const std::string& message) {
        std::cout << "[WARN][" << category << "] " << message << std::endl;
    }

    static void info(const std::string& category, const std::string& message) {
        std::cout << "[INFO][" << category << "] " << message << std::endl;
    }
};

#endif // VMC_STANDALONE_LOG_H

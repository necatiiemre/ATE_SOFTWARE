#ifndef VMC_STANDALONE_LOG_H
#define VMC_STANDALONE_LOG_H

#include <iostream>
#include <string>

// ============================================================================
// Logging helpers for VmcStandalone.
//
// MainSoftware uses a compile-time DEBUG_LOG macro (Utils.h, DEBUG_PRINT=0).
// Here the same detail level is switchable at runtime with --verbose so the
// operator can debug a switch problem without rebuilding.
// ============================================================================

namespace logcfg {
    // Set from main() (--verbose). Defined in main.cpp.
    extern bool verbose;
}

#define VERBOSE_LOG(msg) \
    do { if (logcfg::verbose) { std::cout << msg << std::endl; } } while (0)

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

#ifndef VMC_STANDALONE_SYSTEM_COMMAND_H
#define VMC_STANDALONE_SYSTEM_COMMAND_H

#include <string>

/**
 * @brief Result of a local shell command.
 */
struct CommandResult {
    int exit_code = -1;     // Command exit code (0 = success)
    std::string output;     // Combined stdout + stderr
    bool success = false;   // exit_code == 0

    explicit operator bool() const { return success; }
};

/**
 * @brief Run a command through the local shell (popen) and capture its
 *        combined stdout/stderr.
 *
 * Trimmed-down version of MainSoftware's SystemCommandExecutor: VmcStandalone
 * only needs "run this and tell me what happened".
 */
namespace syscmd {
    CommandResult execute(const std::string& command);

    /** @brief true if `name` resolves to an executable on the local PATH. */
    bool isToolAvailable(const std::string& name);
}

#endif // VMC_STANDALONE_SYSTEM_COMMAND_H

#include "SystemCommand.h"

#include <array>
#include <cstdio>
#include <sstream>
#include <sys/wait.h>

namespace syscmd {

CommandResult execute(const std::string& command)
{
    CommandResult result;

    // Capture stderr together with stdout.
    const std::string full_command = command + " 2>&1";

    FILE* pipe = popen(full_command.c_str(), "r");
    if (!pipe) {
        result.output = "popen() failed";
        return result;
    }

    std::array<char, 4096> buffer;
    std::stringstream out;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        out << buffer.data();
    }

    int status = pclose(pipe);
    result.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    result.output = out.str();
    result.success = (result.exit_code == 0);
    return result;
}

bool isToolAvailable(const std::string& name)
{
    return execute("command -v " + name + " >/dev/null").success;
}

} // namespace syscmd

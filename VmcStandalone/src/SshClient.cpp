#include "SshClient.h"
#include "SystemCommand.h"
#include "Log.h"

#include <filesystem>
#include <utility>

SshClient::SshClient(std::string host,
                     std::string username,
                     std::string password,
                     std::string name)
    : m_host(std::move(host))
    , m_username(std::move(username))
    , m_password(std::move(password))
    , m_name(std::move(name))
{
}

std::string SshClient::buildSSHCommand(const std::string& remote_command) const
{
    // -n: stdin from /dev/null so ssh never swallows operator keystrokes.
    return "sshpass -p '" + m_password + "' "
           "ssh -n -o StrictHostKeyChecking=no "
           "-o ConnectTimeout=10 "
           + m_username + "@" + m_host + " "
           "\"" + remote_command + "\"";
}

std::string SshClient::buildSCPCommand(const std::string& local_path,
                                       const std::string& remote_path) const
{
    return "sshpass -p '" + m_password + "' "
           "scp -o StrictHostKeyChecking=no "
           "-o ConnectTimeout=10 "
           "\"" + local_path + "\" "
           + m_username + "@" + m_host + ":" + remote_path;
}

bool SshClient::testConnection()
{
    VERBOSE_LOG(logPrefix() << " Testing connection to " << m_host << "...");

    auto result = syscmd::execute(buildSSHCommand("echo 'Connection OK'"));
    if (!result.success) {
        std::cerr << logPrefix() << " Connection to " << m_host
                  << " failed: " << result.output << std::endl;
        return false;
    }

    VERBOSE_LOG(logPrefix() << " Connection successful!");
    return true;
}

bool SshClient::execute(const std::string& command,
                        std::string* output,
                        bool use_sudo,
                        bool silent)
{
    std::string actual_command = command;
    if (use_sudo) {
        actual_command = "echo '" + m_password + "' | sudo -S " + command;
    }

    if (!silent) {
        VERBOSE_LOG(logPrefix() << " Executing: " << command
                                << (use_sudo ? " (with sudo)" : ""));
    }

    auto result = syscmd::execute(buildSSHCommand(actual_command));

    if (output) {
        *output = result.output;
    }

    if (!result.success) {
        // Errors are always shown, even in silent mode.
        std::cerr << logPrefix() << " Command failed (exit " << result.exit_code
                  << "): " << command << std::endl;
        if (!result.output.empty()) {
            std::cerr << logPrefix() << " Output:\n" << result.output << std::endl;
        }
        return false;
    }

    if (!silent && !result.output.empty()) {
        VERBOSE_LOG(logPrefix() << " Output:\n" << result.output);
    }
    return true;
}

bool SshClient::copyFileToPath(const std::string& local_path,
                               const std::string& remote_path,
                               bool use_sudo)
{
    if (!std::filesystem::exists(local_path)) {
        std::cerr << logPrefix() << " Local file not found: " << local_path << std::endl;
        return false;
    }

    const std::string remote_dir =
        std::filesystem::path(remote_path).parent_path().string();

    VERBOSE_LOG(logPrefix() << " Copying " << local_path << " -> " << remote_path
                            << (use_sudo ? " (with sudo)" : ""));

    if (!use_sudo) {
        syscmd::execute(buildSSHCommand("mkdir -p " + remote_dir));
        auto result = syscmd::execute(buildSCPCommand(local_path, remote_path));
        if (!result.success) {
            std::cerr << logPrefix() << " Copy failed: " << result.output << std::endl;
            return false;
        }
        return true;
    }

    // Protected destination: scp to /tmp, then sudo mv into place.
    const std::string filename =
        std::filesystem::path(local_path).filename().string();
    const std::string tmp_path = "/tmp/" + filename;

    auto result = syscmd::execute(buildSCPCommand(local_path, tmp_path));
    if (!result.success) {
        std::cerr << logPrefix() << " Copy to /tmp failed: " << result.output << std::endl;
        return false;
    }

    syscmd::execute(buildSSHCommand(
        "echo '" + m_password + "' | sudo -S mkdir -p " + remote_dir));

    result = syscmd::execute(buildSSHCommand(
        "echo '" + m_password + "' | sudo -S mv " + tmp_path + " " + remote_path));
    if (!result.success) {
        std::cerr << logPrefix() << " sudo mv failed: " << result.output << std::endl;
        return false;
    }
    return true;
}

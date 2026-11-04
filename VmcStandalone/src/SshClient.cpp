#include "SshClient.h"
#include "SystemCommand.h"
#include "Log.h"

#include <cstdlib>
#include <filesystem>
#include <sys/wait.h>
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

bool SshClient::copyDirectory(const std::string& local_dir,
                              const std::string& remote_parent,
                              const std::string& remote_name)
{
    if (!std::filesystem::is_directory(local_dir)) {
        std::cerr << logPrefix() << " Local directory not found: " << local_dir << std::endl;
        return false;
    }

    const std::string remote_path = remote_parent + "/" + remote_name;
    VERBOSE_LOG(logPrefix() << " Copying directory " << local_dir << " -> " << remote_path);

    // One tar stream over one SSH session: replaces the remote directory
    // with the local one (behaves the same on every OpenSSH version, unlike
    // `scp -r dir/.`). No `-n` here - ssh must read the tar data from stdin.
    const std::string cmd =
        "tar -C \"" + local_dir + "\" -cf - . | "
        "sshpass -p '" + m_password + "' "
        "ssh -o StrictHostKeyChecking=no -o ConnectTimeout=30 "
        + m_username + "@" + m_host + " "
        "\"rm -rf " + remote_path + " && mkdir -p " + remote_path +
        " && tar -C " + remote_path + " -xf -\"";

    auto result = syscmd::execute(cmd);
    if (!result.success) {
        std::cerr << logPrefix() << " Directory copy to " << remote_path << " failed: "
                  << result.output << std::endl;
        return false;
    }
    return true;
}

bool SshClient::executeInteractive(const std::string& command, int* exit_code)
{
    VERBOSE_LOG(logPrefix() << " Executing interactively: " << command);

    // -t: allocate a remote PTY so the program behaves as if run locally.
    const std::string ssh_cmd =
        "sshpass -p '" + m_password + "' "
        "ssh -t -o StrictHostKeyChecking=no -o ConnectTimeout=10 "
        + m_username + "@" + m_host + " "
        "\"" + command + "\"";

    // system(): stdin/stdout/stderr stay connected to this terminal.
    const int status = std::system(ssh_cmd.c_str());
    const int code = (status != -1 && WIFEXITED(status)) ? WEXITSTATUS(status) : -1;
    if (exit_code) *exit_code = code;

    // The caller reports a non-zero exit code (it knows what was run).
    return code == 0;
}

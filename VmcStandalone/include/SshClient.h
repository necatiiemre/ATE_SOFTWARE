#ifndef VMC_STANDALONE_SSH_CLIENT_H
#define VMC_STANDALONE_SSH_CLIENT_H

#include <string>

/**
 * @brief Minimal sshpass-based SSH/SCP client.
 *
 * Extracted from MainSoftware's SSHDeployer - only the parts the Cumulus
 * configuration needs (connection test, remote execute, copy-to-path).
 * Command strings are built exactly the same way, so behaviour on the
 * switch is identical to MainSoftware.
 *
 * Requires `sshpass`, `ssh` and `scp` on the local machine.
 */
class SshClient {
public:
    SshClient(std::string host,
              std::string username,
              std::string password,
              std::string name);

    const std::string& host() const { return m_host; }

    /** @brief Run `echo 'Connection OK'` on the remote side. */
    bool testConnection();

    /**
     * @brief Execute a command on the remote host.
     * @param command  Remote command
     * @param output   Optional captured combined output
     * @param use_sudo Wrap as `echo <pw> | sudo -S <command>`
     * @param silent   Don't print the command in verbose mode (errors are
     *                 always printed)
     */
    bool execute(const std::string& command,
                 std::string* output = nullptr,
                 bool use_sudo = false,
                 bool silent = false);

    /**
     * @brief Copy a local file to an absolute remote path.
     *        With use_sudo the file goes to /tmp first, then `sudo mv`.
     */
    bool copyFileToPath(const std::string& local_path,
                        const std::string& remote_path,
                        bool use_sudo = false);

    /**
     * @brief Copy a local directory to <remote_parent>/<remote_name>.
     *        Any existing remote directory with that name is removed first.
     */
    bool copyDirectory(const std::string& local_dir,
                       const std::string& remote_parent,
                       const std::string& remote_name);

    /**
     * @brief Run a command with a remote pseudo-terminal (ssh -t), wired
     *        straight to this terminal: output is live, keyboard input and
     *        Ctrl+C reach the remote program. Blocks until it exits.
     * @param exit_code Remote command's exit code (optional)
     * @return true if the remote command exited with 0
     */
    bool executeInteractive(const std::string& command, int* exit_code = nullptr);

    const std::string& password() const { return m_password; }

private:
    std::string m_host;
    std::string m_username;
    std::string m_password;
    std::string m_name;

    std::string logPrefix() const { return "[" + m_name + "]"; }
    std::string buildSSHCommand(const std::string& remote_command) const;
    std::string buildSCPCommand(const std::string& local_path,
                                const std::string& remote_path) const;
};

#endif // VMC_STANDALONE_SSH_CLIENT_H

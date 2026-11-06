#include "VmcTestRunner.h"
#include "Log.h"
#include "ServerConfig.h"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

std::filesystem::path executableDir()
{
    std::error_code ec;
    auto exe = std::filesystem::canonical("/proc/self/exe", ec);
    return ec ? std::filesystem::current_path() : exe.parent_path();
}

void printBanner(const std::string& text)
{
    std::cout << "\n========================================" << std::endl;
    std::cout << text << std::endl;
    std::cout << "========================================" << std::endl;
}

} // namespace

VmcTestRunner::VmcTestRunner(SshClient& ssh)
    : m_ssh(ssh)
{
}

std::string VmcTestRunner::localPrebuiltDir()
{
    return (executableDir() / VMC_TEST_REMOTE_DIR_NAME).string();
}

bool VmcTestRunner::deployAndRun(const std::string& args)
{
    printBanner(logPrefix() + " Deploy & run on server " + m_ssh.host());

    const std::string local_dir = localPrebuiltDir();
    const std::string local_bin = local_dir + "/" + VMC_TEST_APP_NAME;
    if (!std::filesystem::exists(local_bin)) {
        std::cerr << logPrefix() << " Prebuilt binary not found: " << local_bin << std::endl;
        std::cerr << logPrefix() << " Build dpmDsmStandalone with the server reachable "
                     "(it compiles " VMC_TEST_APP_NAME " there), or run 'make " VMC_TEST_APP_NAME "'." << std::endl;
        return false;
    }

    std::string version;
    {
        std::ifstream vf(local_dir + "/VERSION.txt");
        std::getline(vf, version);
    }
    std::cout << logPrefix() << " Binary: " << local_bin
              << (version.empty() ? "" : " (v" + version + ")") << std::endl;

    std::cout << logPrefix() << " [1/3] Testing connection..." << std::endl;
    if (!m_ssh.testConnection()) {
        std::cerr << logPrefix() << " Cannot connect to server" << std::endl;
        return false;
    }

    const std::string remote_dir =
        std::string(SERVER_REMOTE_DIR) + "/" + VMC_TEST_REMOTE_DIR_NAME;

    std::cout << logPrefix() << " [2/3] Copying to " << remote_dir << "..." << std::endl;
    if (!m_ssh.copyDirectory(local_dir, SERVER_REMOTE_DIR, VMC_TEST_REMOTE_DIR_NAME)) {
        return false;
    }
    if (!m_ssh.execute(std::string("chmod +x ") + remote_dir + "/" + VMC_TEST_APP_NAME)) {
        return false;
    }

    // Raw sockets need root. Prime sudo's credential cache with the password
    // first, so the program itself keeps the terminal's stdin (same trick as
    // MainSoftware uses for dpdk_vmc).
    std::string command = "cd " + remote_dir + " && "
                          "echo '" + m_ssh.password() + "' | sudo -S -v && "
                          "sudo ./" + VMC_TEST_APP_NAME;
    if (!args.empty()) command += " " + args;

    std::cout << logPrefix() << " [3/3] Running: sudo ./" << VMC_TEST_APP_NAME
              << (args.empty() ? "" : " " + args) << std::endl;
    std::cout << "---------------- " VMC_TEST_APP_NAME " output ----------------" << std::endl;
    int exit_code = -1;
    const bool ok = m_ssh.executeInteractive(command, &exit_code);
    std::cout << "-----------------------------------------------------" << std::endl;

    if (!ok) {
        std::cerr << logPrefix() << " " VMC_TEST_APP_NAME " exited with code " << exit_code << std::endl;
        return false;
    }
    std::cout << logPrefix() << " " VMC_TEST_APP_NAME " finished successfully" << std::endl;
    return true;
}

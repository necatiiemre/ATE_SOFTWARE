#ifndef VMC_STANDALONE_VMC_TEST_RUNNER_H
#define VMC_STANDALONE_VMC_TEST_RUNNER_H

#include <string>

#include "SshClient.h"

/**
 * @brief Deploys the prebuilt vmc_test binary to the ATE server and runs it.
 *
 * The binary was compiled on the server during the vmcStandalone build and
 * fetched to <exe_dir>/vmc_test/ (see scripts/build_vmc_test_on_server.sh).
 * Same pattern as MainSoftware's Vmc::runDpdkVmcInteractive() for dpdk_vmc:
 *
 *   1. SSH connection test
 *   2. copy <exe_dir>/vmc_test/ -> <SERVER_REMOTE_DIR>/vmc_test/
 *   3. sudo ./vmc_test <args>   (interactive, live output, Ctrl+C works)
 */
class VmcTestRunner {
public:
    explicit VmcTestRunner(SshClient& ssh);

    /** @brief Local folder holding the prebuilt binary (<exe_dir>/vmc_test). */
    static std::string localPrebuiltDir();

    /** @brief Steps 1-3 above. @param args extra arguments for vmc_test */
    bool deployAndRun(const std::string& args);

private:
    SshClient& m_ssh;

    static std::string logPrefix() { return "[vmc_test]"; }
};

#endif // VMC_STANDALONE_VMC_TEST_RUNNER_H

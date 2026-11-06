#ifndef VMC_STANDALONE_VMC_TEST_RUNNER_H
#define VMC_STANDALONE_VMC_TEST_RUNNER_H

#include <string>

#include "SshClient.h"

/**
 * @brief Deploys the prebuilt dpm_dsm_test binary to the ATE server and runs it.
 *
 * The binary was compiled on the server during the vmcStandalone build and
 * fetched to <exe_dir>/dpm_dsm_test/ (see scripts/build_dpm_dsm_test_on_server.sh).
 * Same pattern as MainSoftware's Vmc::runDpdkVmcInteractive() for dpdk_vmc:
 *
 *   1. SSH connection test
 *   2. copy <exe_dir>/dpm_dsm_test/ -> <SERVER_REMOTE_DIR>/dpm_dsm_test/
 *   3. sudo ./vmc_test <args>   (interactive, live output, Ctrl+C works)
 */
class VmcTestRunner {
public:
    explicit VmcTestRunner(SshClient& ssh);

    /** @brief Local folder holding the prebuilt binary (<exe_dir>/dpm_dsm_test). */
    static std::string localPrebuiltDir();

    /** @brief Steps 1-3 above. @param args extra arguments for vmc_test */
    bool deployAndRun(const std::string& args);

private:
    SshClient& m_ssh;

    static std::string logPrefix() { return "[dpm_dsm_test]"; }
};

#endif // VMC_STANDALONE_VMC_TEST_RUNNER_H

#ifndef VMC_STANDALONE_SERVER_CONFIG_H
#define VMC_STANDALONE_SERVER_CONFIG_H

// ============================================================================
// ATE server connection settings (where vmc_test is built and run).
// Must match MainSoftware (SSHDeployer.cpp -> g_ssh_deployer_server).
//
// This header is ALSO read by scripts/build_vmc_test_on_server.sh during the
// CMake build, so it is the single place to change these values. Keep the
// `#define NAME "value"` form on one line.
// ============================================================================
#define SERVER_HOST        "10.1.33.2"
#define SERVER_USER        "user"
#define SERVER_PASSWORD    "q"
#define SERVER_REMOTE_DIR  "/home/user/Desktop"

// Build-time: vmc_test sources are compiled in <SERVER_REMOTE_DIR>/vmc_test_build
// Run-time:   the prebuilt binary is deployed to <SERVER_REMOTE_DIR>/vmc_test
#define VMC_TEST_REMOTE_DIR_NAME  "vmc_test"
#define VMC_TEST_APP_NAME         "vmc_test"

#endif // VMC_STANDALONE_SERVER_CONFIG_H

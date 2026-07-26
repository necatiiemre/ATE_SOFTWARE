#ifndef CMC_H
#define CMC_H

#include <string>
#include <vector>
#include <atomic>
#include "DeviceManager.h"
#include "Utils.h"
#include "SerialPort.h"
#include "Server.h"
#include "SystemCommand.h"

class Cmc
{
private:
    /**
     * @brief Ensure LOGS directory structure exists
     * @return true on success
     */
    bool ensureLogDirectories();

    /**
     * @brief Fetch DPDK CMC log + MMMS logs from server to local PC.
     *
     * Idempotent (guarded by m_logs_fetched): runs at most once whether it is
     * reached via the normal completion path or via a signal-triggered
     * SafeShutdown log-fetch callback. This guard is important because the MMMS
     * fetch clears the local target directory first — a second run would wipe
     * already-saved logs if the remote copy is gone.
     */
    void fetchDpdkAndMmmsLogs();

    // Set once fetchDpdkAndMmmsLogs() has run (normal path or signal path).
    std::atomic<bool> m_logs_fetched{false};

public:
    Cmc();
    ~Cmc();

    bool configureSequence();

    /**
     * @brief Deploy, build and run DPDK CMC interactively
     *
     * Mirrors Vmc::runDpdkVmcInteractive():
     * 1. Deploys DPDK CMC source/binary to remote server
     * 2. Runs DPDK CMC interactively (user can answer y/n prompts)
     * 3. After ATE/latency selection, DPDK CMC forks to background
     * 4. SSH connection closes, main software continues
     *
     * @param eal_args  EAL arguments (e.g. "-l 0-255 -n 16")
     * @param make_args Optional make arguments (currently unused; reserved
     *                  for symmetry with Vmc)
     * @return true on success
     */
    bool runDpdkCmcInteractive(const std::string& eal_args = "-l 0-255 -n 16",
                                const std::string& make_args = "");
};

extern Cmc g_cmc;

#endif // CMC_H
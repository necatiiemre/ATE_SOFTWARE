#ifndef REPORT_MANAGER_H
#define REPORT_MANAGER_H

#include <string>

class ReportManager
{
public:
    ReportManager();
    ~ReportManager();

    // Collects test information from user
    bool collectTestInfo();

    // Returns the test name
    std::string getTestName() const;
    std::string getTesterName() const;
    std::string getQualityCheckerName() const;
    std::string getSerialNumber() const;
    std::string getBilgemNumber() const;
    std::string getAteSerialNumber() const;

    void setUnitName(std::string name);

    // Writes report header to the beginning of the log file
    bool writeReportHeader();

    // Creates PDF report from log file
    bool createPdfReport();

    // Per-test output directory: LOGS/<unit>/<test_name>-<session-timestamp>.
    // Created on access so every caller is safe to write into it.
    std::string getTestLogDir();

    // Full path of the main log inside the per-test dir:
    // <dir>/<prefix>_EQ_Test_Result_Log_Files.log
    std::string getTestLogFilePath();

    // Full path of the summary log inside the per-test dir:
    // <dir>/<prefix>_EQ_Test_Result_Summary_Log_Files.log
    std::string getSummaryLogFilePath();

    // Timestamp recording methods
    void recordSoftwareStartTime();
    void recordUnitPowerOnTime();
    void recordPowerOffTime();
    void recordSoftwareEndTime();

    // Returns current timestamp as formatted string
    static std::string getCurrentTimestamp();

private:
    // Returns a filesystem-safe timestamp (e.g. 2026-06-09_14-30-00)
    static std::string getFileSafeTimestamp();

    // Returns the ReportGenerator binary path (compiled from Python via PyInstaller)
    std::string getPythonScriptPath() const;
    // Checks if input contains Turkish characters
    bool containsTurkishCharacter(const std::string &input) const;

    // Checks if input contains only digits
    bool containsOnlyDigits(const std::string &input) const;

    // Returns the log directory path for the given unit name
    std::string getLogPathForUnit() const;

    // Returns the log-file name prefix for the current unit. DTN and HSN carry
    // the "IRSW" designator (e.g. "DTN_IRSW"); CMC/VMC/MMC use the bare unit
    // name (e.g. "CMC"). Combined with the shared "_EQ_Test_Result_*" suffix.
    std::string getLogFilePrefix() const;

    // Builds the "TEST REPORT" header block shared by the main log and the
    // DTN summary log.
    std::string buildReportHeaderText() const;

    // Returns the DPDK app version deployed for the current unit
    // (DTN -> dpdk, CMC -> dpdk_cmc, VMC -> dpdk_vmc). Units that run no DPDK
    // app (MMC, HSN) return "N/A". Values are injected at build time from each
    // component's include/<Name>Version.h via CMake compile definitions.
    std::string dpdkVersionForUnit() const;

    // Prepends `header` to an existing file at `path`. No-op returning false if
    // the file does not exist (e.g. no summary log was fetched for this unit).
    bool prependHeaderToFile(const std::string &path, const std::string &header) const;

    std::string m_testName;
    std::string m_test_name_correction;
    std::string m_ate_serial_number;
    std::string m_ate_serial_number_correction;
    std::string m_bilgem_number;
    std::string m_bilgem_number_correction;
    std::string m_serial_number;
    std::string m_serial_number_correction;
    std::string m_tester_name;
    std::string m_quality_checker_name;
    std::string m_unit_name;

    // Captured once at software start; names the per-test log directory so all
    // outputs of a single run land together and never overwrite a previous run.
    std::string m_session_timestamp;

    // Timestamp fields
    std::string m_software_start_time;
    std::string m_unit_power_on_time;
    std::string m_power_off_time;
    std::string m_software_end_time;
};

// Global singleton declaration
extern ReportManager g_ReportManager;

#endif // REPORT_MANAGER_H
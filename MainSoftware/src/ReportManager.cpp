#include "ReportManager.h"
#include "Dtn.h"
#include "SSHDeployer.h"
#include "Utils.h"
#include "DebugConfig.h"   // MAIN_DEBUG_MODE + DEBUG_* test info
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <ctime>
#include <cstdlib>
#include <array>

// Component versions are injected as compile definitions by CMake (read from
// each component's include/<Name>Version.h). Provide fallbacks so the file still
// builds if compiled outside the CMake flow.
#ifndef MAINSOFTWARE_VERSION
#define MAINSOFTWARE_VERSION "unknown"
#endif
#ifndef DPDK_DTN_VERSION
#define DPDK_DTN_VERSION "unknown"
#endif
#ifndef DPDK_CMC_VERSION
#define DPDK_CMC_VERSION "unknown"
#endif
#ifndef DPDK_VMC_VERSION
#define DPDK_VMC_VERSION "unknown"
#endif

// Global singleton
ReportManager g_ReportManager;

ReportManager::ReportManager()
{
}

ReportManager::~ReportManager()
{
}

bool ReportManager::containsTurkishCharacter(const std::string &input) const
{
    // UTF-8 encoded Turkish characters:
    // ç (c3 a7), Ç (c3 87)
    // ş (c5 9f), Ş (c5 9e)
    // ğ (c4 9f), Ğ (c4 9e)
    // ü (c3 bc), Ü (c3 9c)
    // ö (c3 b6), Ö (c3 96)
    // ı (c4 b1), İ (c4 b0)
    const std::string turkishChars[] = {
        "\xc3\xa7", "\xc3\x87",  // ç, Ç
        "\xc5\x9f", "\xc5\x9e",  // ş, Ş
        "\xc4\x9f", "\xc4\x9e",  // ğ, Ğ
        "\xc3\xbc", "\xc3\x9c",  // ü, Ü
        "\xc3\xb6", "\xc3\x96",  // ö, Ö
        "\xc4\xb1", "\xc4\xb0"   // ı, İ
    };

    for (const auto &tc : turkishChars)
    {
        if (input.find(tc) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

bool ReportManager::containsOnlyDigits(const std::string &input) const
{
    for (const auto &c : input)
    {
        if (!std::isdigit(static_cast<unsigned char>(c)))
        {
            return false;
        }
    }
    return true;
}

std::string ReportManager::getCurrentTimestamp()
{
    std::time_t now = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%B %d, %Y %H:%M:%S", std::localtime(&now));
    return std::string(buf);
}

std::string ReportManager::getFileSafeTimestamp()
{
    std::time_t now = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", std::localtime(&now));
    return std::string(buf);
}

void ReportManager::recordSoftwareStartTime()
{
    m_software_start_time = getCurrentTimestamp();
    // Anchor the per-test log directory to the software start moment.
    m_session_timestamp = getFileSafeTimestamp();
    DEBUG_LOG("Software Start Time recorded: " << m_software_start_time);
}

void ReportManager::recordUnitPowerOnTime()
{
    m_unit_power_on_time = getCurrentTimestamp();
    DEBUG_LOG("Unit Power On Time recorded: " << m_unit_power_on_time);
}

void ReportManager::recordPowerOffTime()
{
    m_power_off_time = getCurrentTimestamp();
    DEBUG_LOG("Power Off Time recorded: " << m_power_off_time);
}

void ReportManager::recordSoftwareEndTime()
{
    m_software_end_time = getCurrentTimestamp();
    DEBUG_LOG("Software End Time recorded: " << m_software_end_time);
}

bool ReportManager::collectTestInfo()
{
#if MAIN_DEBUG_MODE
    // Debug/otomasyon: test bilgileri DebugConfig.h'tan hard-coded doldurulur.
    m_testName                     = DEBUG_TEST_NAME;
    m_test_name_correction         = DEBUG_TEST_NAME;
    m_ate_serial_number            = DEBUG_ATE_SERIAL;
    m_ate_serial_number_correction = DEBUG_ATE_SERIAL;
    m_bilgem_number                = DEBUG_BILGEM_NUMBER;
    m_bilgem_number_correction     = DEBUG_BILGEM_NUMBER;
    m_serial_number                = DEBUG_SERIAL_NUMBER;
    m_serial_number_correction     = DEBUG_SERIAL_NUMBER;
    m_tester_name                  = DEBUG_TESTER_NAME;
    m_quality_checker_name         = DEBUG_QC_NAME;
    std::cout << "[DEBUG] Test bilgileri otomatik dolduruldu "
              << "(test=" << m_testName
              << ", ate_serial=" << m_ate_serial_number
              << ", bilgem=" << m_bilgem_number
              << ", serial=" << m_serial_number
              << ", tester=" << m_tester_name << ")" << std::endl;
    return true;
#else
    std::cout << "========================================" << std::endl;
    std::cout << "         REPORT MANAGER" << std::endl;
    std::cout << "========================================" << std::endl;

    while (true)
    {
        std::cout << "Enter test name: ";
        std::getline(std::cin, m_testName);

        if (m_testName.empty())
        {
            std::cout << "Test name can not be empty!" << std::endl;
            continue;
        }

        if (containsTurkishCharacter(m_testName))
        {
            std::cout << "Error! Test name must not include Turkish letters.(ç, ş, ğ, ü, ö, ı)." << std::endl;
            std::cout << "Please enter again!" << std::endl;
            continue;
        }

        std::cout << "Enter test name for correction: ";
        std::getline(std::cin, m_test_name_correction);

        if (m_test_name_correction.empty())
        {
            std::cout << "Test name can not be empty!" << std::endl;
            continue;
        }

        // No Turkish character, ask again for confirmation
        std::cout << "Test name: " << m_testName << std::endl;

        if (m_testName.compare(m_test_name_correction) == 0)
        {
            break;
        }

        std::cout << "Invalid test name. Plase try again." << std::endl;
    }

    std::cout << "Test name saved: " << m_testName << std::endl;
    std::cout << "========================================" << std::endl;

    while (true)
    {
        std::cout << "Enter ATE serial number (max 3 digits): ";
        std::getline(std::cin, m_ate_serial_number);

        if (m_ate_serial_number.empty())
        {
            std::cout << "ATE serial number can not be empty!" << std::endl;
            continue;
        }

        if (!containsOnlyDigits(m_ate_serial_number))
        {
            std::cout << "Error! ATE serial number must contain only digits." << std::endl;
            std::cout << "Please enter again!" << std::endl;
            continue;
        }

        if (m_ate_serial_number.length() > 3)
        {
            std::cout << "Error! ATE serial number must be at most 3 digits." << std::endl;
            std::cout << "Please enter again!" << std::endl;
            continue;
        }

        std::cout << "Enter ATE serial number for correction: ";
        std::getline(std::cin, m_ate_serial_number_correction);

        if (m_ate_serial_number_correction.empty())
        {
            std::cout << "ATE serial number can not be empty!" << std::endl;
            continue;
        }

        std::cout << "ATE serial number: " << m_ate_serial_number << std::endl;

        if (m_ate_serial_number.compare(m_ate_serial_number_correction) == 0)
        {
            break;
        }

        std::cout << "Invalid ATE serial number. Please try again." << std::endl;
    }

    std::cout << "ATE serial number saved: " << m_ate_serial_number << std::endl;
    std::cout << "========================================" << std::endl;

    while (true)
    {
        std::cout << "Enter Bilgem number (9 digits): ";
        std::getline(std::cin, m_bilgem_number);

        if (m_bilgem_number.empty())
        {
            std::cout << "Bilgem number can not be empty!" << std::endl;
            continue;
        }

        if (!containsOnlyDigits(m_bilgem_number))
        {
            std::cout << "Error! Bilgem number must contain only digits." << std::endl;
            std::cout << "Please enter again!" << std::endl;
            continue;
        }

        if (m_bilgem_number.length() != 9)
        {
            std::cout << "Error! Bilgem number must be exactly 9 digits." << std::endl;
            std::cout << "Please enter again!" << std::endl;
            continue;
        }

        std::cout << "Enter Bilgem number for correction: ";
        std::getline(std::cin, m_bilgem_number_correction);

        if (m_bilgem_number_correction.empty())
        {
            std::cout << "Bilgem number can not be empty!" << std::endl;
            continue;
        }

        std::cout << "Bilgem number: " << m_bilgem_number << std::endl;

        if (m_bilgem_number.compare(m_bilgem_number_correction) == 0)
        {
            break;
        }

        std::cout << "Invalid Bilgem number. Please try again." << std::endl;
    }

    std::cout << "Bilgem number saved: " << m_bilgem_number << std::endl;
    std::cout << "========================================" << std::endl;

    while (true)
    {
        std::cout << "Enter serial number: ";
        std::getline(std::cin, m_serial_number);

        if (m_serial_number.empty())
        {
            std::cout << "Serial number can not be empty!" << std::endl;
            continue;
        }

        if (!containsOnlyDigits(m_serial_number))
        {
            std::cout << "Error! Serial number must contain only digits." << std::endl;
            std::cout << "Please enter again!" << std::endl;
            continue;
        }

        std::cout << "Enter serial number for correction: ";
        std::getline(std::cin, m_serial_number_correction);

        if (m_serial_number_correction.empty())
        {
            std::cout << "Serial number can not be empty!" << std::endl;
            continue;
        }

        std::cout << "Serial number: " << m_serial_number << std::endl;

        if (m_serial_number.compare(m_serial_number_correction) == 0)
        {
            break;
        }

        std::cout << "Invalid serial number. Please try again." << std::endl;
    }

    std::cout << "Serial number saved: " << m_serial_number << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << "Enter tester name: ";
    std::getline(std::cin, m_tester_name);
    std::cout << "Tester name saved: " << m_tester_name << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << "Enter quality checker name: ";
    std::getline(std::cin, m_quality_checker_name);
    std::cout << "Quality checker name saved: " << m_quality_checker_name << std::endl;
    std::cout << "========================================" << std::endl;

    return true;
#endif
}

std::string ReportManager::getTestName() const
{
    return m_testName;
}

std::string ReportManager::getTesterName() const
{
    return m_tester_name;
}

std::string ReportManager::getQualityCheckerName() const
{
    return m_quality_checker_name;
}

std::string ReportManager::getSerialNumber() const
{
    return m_serial_number;
}

std::string ReportManager::getBilgemNumber() const
{
    return m_bilgem_number;
}

std::string ReportManager::getAteSerialNumber() const
{
    return m_ate_serial_number;
}

void ReportManager::setUnitName(std::string name)
{
    m_unit_name = name;
    DEBUG_LOG("Unit name saved: " << m_unit_name);
    DEBUG_LOG("========================================");
}

std::string ReportManager::getLogPathForUnit() const
{
    if (m_unit_name == "CMC") return LogPaths::CMC();
    if (m_unit_name == "VMC") return LogPaths::VMC();
    if (m_unit_name == "MMC") return LogPaths::MMC();
    if (m_unit_name == "DTN") return LogPaths::DTN();
    if (m_unit_name == "HSN") return LogPaths::HSN();
    return LogPaths::baseDir();
}

std::string ReportManager::getLogFilePrefix() const
{
    // DTN and HSN units carry the "IRSW" designator in their file names;
    // CMC/VMC/MMC use the bare unit name.
    if (m_unit_name == "DTN" || m_unit_name == "HSN")
        return m_unit_name + "_IRSW";
    return m_unit_name;
}

std::string ReportManager::getTestLogDir()
{
    // Fallback in case recordSoftwareStartTime() was never called.
    if (m_session_timestamp.empty())
        m_session_timestamp = getFileSafeTimestamp();

    // Per-test directory is named "<test_name>-<timestamp>" so a run is
    // identifiable by test name at a glance and never overwrites a previous one.
    std::string dir = getLogPathForUnit() + "/" + m_testName + "-" + m_session_timestamp;

    // Idempotent: make sure the run's directory exists for every writer.
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);

    return dir;
}

std::string ReportManager::getTestLogFilePath()
{
    return getTestLogDir() + "/" + getLogFilePrefix() + "_EQ_Test_Result_Log_Files.log";
}

std::string ReportManager::getSummaryLogFilePath()
{
    return getTestLogDir() + "/" + getLogFilePrefix() + "_EQ_Test_Result_Summary_Log_Files.log";
}

std::string ReportManager::buildReportHeaderText() const
{
    std::ostringstream h;
    h << "========================================" << "\n";
    h << "         TEST REPORT" << "\n";
    h << "========================================" << "\n";
    h << "Software Start Time : " << m_software_start_time << "\n";
    h << "Unit Power On Time  : " << m_unit_power_on_time << "\n";
    h << "Power Off Time      : " << m_power_off_time << "\n";
    h << "Software End Time   : " << m_software_end_time << "\n";
    h << "Test Name           : " << m_testName << "\n";
    h << "ATE Serial Number   : " << m_ate_serial_number << "\n";
    h << "Bilgem Number       : " << m_bilgem_number << "\n";
    h << "Serial Number       : " << m_serial_number << "\n";
    h << "Tester Name         : " << m_tester_name << "\n";
    h << "Quality Checker     : " << m_quality_checker_name << "\n";
    h << "Unit Name           : " << m_unit_name << "IRSW" << "\n";
    h << "----------------------------------------" << "\n";
    h << "Software Version    : " << MAINSOFTWARE_VERSION << "\n";
    h << "DPDK Version        : " << dpdkVersionForUnit() << "\n";
    h << "========================================" << "\n";
    h << "\n";
    return h.str();
}

std::string ReportManager::dpdkVersionForUnit() const
{
    // Map the selected unit to the DPDK variant it actually deploys. MMC runs
    // FlickerDetection and HSN runs no DPDK app, so both report "N/A".
    if (m_unit_name == "DTN") return DPDK_DTN_VERSION;
    if (m_unit_name == "CMC") return DPDK_CMC_VERSION;
    if (m_unit_name == "VMC") return DPDK_VMC_VERSION;
    return "N/A";
}

bool ReportManager::prependHeaderToFile(const std::string &path, const std::string &header) const
{
    // Read existing content; if the file isn't there, skip silently.
    std::string existingContent;
    {
        std::ifstream inFile(path);
        if (!inFile.is_open())
        {
            return false;
        }
        std::stringstream ss;
        ss << inFile.rdbuf();
        existingContent = ss.str();
    }

    std::ofstream outFile(path);
    if (!outFile.is_open())
    {
        std::cerr << "Error: Could not open file for header prepend: " << path << std::endl;
        return false;
    }
    outFile << header;
    outFile << existingContent;
    outFile.close();
    return true;
}

bool ReportManager::writeReportHeader()
{
    const std::string header = buildReportHeaderText();
    std::string logFile = getTestLogFilePath();

    // Read existing content if present
    std::string existingContent;
    {
        std::ifstream inFile(logFile);
        if (inFile.is_open())
        {
            std::stringstream ss;
            ss << inFile.rdbuf();
            existingContent = ss.str();
            inFile.close();
        }
    }

    // Open the file and write report header at the beginning
    std::ofstream outFile(logFile);
    if (!outFile.is_open())
    {
        std::cerr << "Error: Could not open log file: " << logFile << std::endl;
        return false;
    }

    outFile << header;

    // Append existing content
    if (!existingContent.empty())
    {
        outFile << existingContent;
    }

    outFile.close();

    DEBUG_LOG("Report header written to: " << logFile);

    // Add the same header to the DTN summary log, if one was fetched into this
    // test's directory. No-op for units that don't produce a summary log.
    std::string summaryFile = getSummaryLogFilePath();
    if (prependHeaderToFile(summaryFile, header))
    {
        DEBUG_LOG("Report header written to summary log: " << summaryFile);
    }

    return true;
}

std::string ReportManager::getPythonScriptPath() const
{
    return SSHDeployer::getPrebuiltRoot() + "/PdfReportGenerator/ReportGenerator";
}

bool ReportManager::createPdfReport()
{
    // 1. Build log file path
    std::string logDir = getTestLogDir();
    std::string logFile = getTestLogFilePath();

    // 2. Check if log file exists
    if (!std::filesystem::exists(logFile))
    {
        std::cerr << "Error: Log file not found: " << logFile << std::endl;
        return false;
    }

    // 3. Determine output PDF path and ensure parent directory exists
    std::string pdfFile = logDir + "/" + m_testName + ".pdf";
    std::filesystem::path pdfParent = std::filesystem::path(pdfFile).parent_path();
    if (!std::filesystem::exists(pdfParent))
    {
        std::filesystem::create_directories(pdfParent);
    }

    // 4. Get Python script and logo paths
    std::string scriptPath = getPythonScriptPath();
    std::string logoPath = SSHDeployer::getPrebuiltRoot() + "/PdfReportGenerator/Assets/company_logo.png";

    if (!std::filesystem::exists(scriptPath))
    {
        std::cerr << "Error: PDF generator script not found: " << scriptPath << std::endl;
        return false;
    }

    // 5. Build and execute command (compiled binary, no python3 needed)
    std::string cmd = "\"" + scriptPath + "\""
                    + " -i \"" + logFile + "\""
                    + " -o \"" + pdfFile + "\""
                    + " --logo \"" + logoPath + "\""
                    + " 2>&1";

    std::cout << "========================================" << std::endl;
    std::cout << "  PDF Report Generation" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Input:  " << logFile << std::endl;
    std::cout << "Output: " << pdfFile << std::endl;

    int ret = std::system(cmd.c_str());

    if (ret != 0)
    {
        std::cerr << "Error: PDF generation failed! (exit code: " << ret << ")" << std::endl;
        std::cerr << "Possible causes:" << std::endl;
        std::cerr << "  - Log file has no valid test phases with Health data" << std::endl;
        std::cerr << "  - ReportGenerator binary not found in prebuilt/" << std::endl;
        return false;
    }

    // 6. Verify PDF was created
    if (!std::filesystem::exists(pdfFile))
    {
        std::cerr << "Error: PDF file was not created: " << pdfFile << std::endl;
        return false;
    }

    std::cout << "PDF report created successfully: " << pdfFile << std::endl;
    std::cout << "========================================" << std::endl;
    return true;
}
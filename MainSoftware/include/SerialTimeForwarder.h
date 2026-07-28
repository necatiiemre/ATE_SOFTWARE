#ifndef SERIAL_TIME_FORWARDER_H
#define SERIAL_TIME_FORWARDER_H

#include "SerialPort.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

namespace serial {

/**
 * @brief Forwards time data from MicroChip SyncServer to another serial port
 *
 * Reads time data from input port (format: YYYY DDD HH:MM:SS DZZ) and sends
 * two packets back-to-back on the output port for each reading:
 *   1) Time packet (69 bytes): seconds-since-midnight as a Q14 fixed-point
 *      little-endian value at bytes 49-52.
 *   2) Date packet (55 bytes): year/month/day as little-endian 16-bit fields
 *      at bytes 29-34.
 * Both carry a trailing two's-complement checksum over their data field.
 *
 * Runs in a separate thread, non-blocking to main application.
 *
 * Usage:
 *   SerialTimeForwarder forwarder("/dev/ttyUSB0", "/dev/ttyUSB1");
 *   forwarder.start();
 *   // ... main application runs ...
 *   forwarder.stop();
 */
class SerialTimeForwarder {
public:
    /**
     * @brief Constructor
     * @param input_device  Input serial port (default: /dev/ttyUSB0, 9600 baud)
     * @param output_device Output serial port (default: /dev/ttyUSB1, 38400 baud)
     * @param verify_device Verification port to read back sent data (optional, empty = disabled)
     * @param utc_enable    When true, the incoming local time is converted to
     *                      UTC before sending (the timezone offset in the
     *                      SyncServer line, e.g. "+03", is subtracted, with
     *                      correct day/month/year rollback). When false, the
     *                      time is forwarded as received (current behavior).
     */
    SerialTimeForwarder(const std::string& input_device = "/dev/ttyUSB0",
                        const std::string& output_device = "/dev/ttyUSB1",
                        const std::string& verify_device = "",
                        bool utc_enable = false);

    /**
     * @brief Destructor - stops thread if running
     */
    ~SerialTimeForwarder();

    // Non-copyable
    SerialTimeForwarder(const SerialTimeForwarder&) = delete;
    SerialTimeForwarder& operator=(const SerialTimeForwarder&) = delete;

    /**
     * @brief Starts the forwarder thread
     * @return true if started successfully
     */
    bool start();

    /**
     * @brief Stops the forwarder thread
     */
    void stop();

    /**
     * @brief Checks if forwarder is running
     */
    bool isRunning() const { return m_running.load(); }

    /**
     * @brief Returns number of packets sent
     */
    uint64_t getPacketsSent() const { return m_packets_sent.load(); }

    /**
     * @brief Returns last Unix timestamp sent
     */
    uint32_t getLastTimestamp() const { return m_last_timestamp.load(); }

    /**
     * @brief Returns last received time string from SyncServer
     */
    std::string getLastTimeString() const;

    /**
     * @brief Returns last error message
     */
    std::string getLastError() const;

private:
    // Serial ports
    SerialPort m_input_port;
    SerialPort m_output_port;
    SerialPort m_verify_port;

    // Thread control
    std::thread m_worker_thread;
    std::thread m_verify_thread;
    std::atomic<bool> m_running{false};
    bool m_verify_enabled{false};  // True if verify device is specified
    bool m_utc_enable{false};      // True: convert incoming local time to UTC before sending

    // Statistics
    std::atomic<uint64_t> m_packets_sent{0};
    std::atomic<uint32_t> m_last_timestamp{0};

    // Timing for end-to-end latency measurement
    std::atomic<int64_t> m_send_time_us{0};  // Time when packet was sent (microseconds since epoch)

    // Error tracking
    mutable std::mutex m_error_mutex;
    std::string m_last_error;

    // Last received time string
    mutable std::mutex m_time_string_mutex;
    std::string m_last_time_string;

    // Packet constants (1-based byte numbers from the protocol doc are noted
    // in comments; the offsets below are 0-based for array indexing).
    static constexpr size_t PACKET_SIZE = 69;         // full system message
    static constexpr size_t TS_OFFSET = 48;           // time field, bytes 49-52 (Q14, little-endian)
    static constexpr size_t CHECKSUM_OFFSET = 68;     // trailing checksum, byte 69
    static constexpr size_t CHECKSUM_SUM_BEGIN = 4;   // data field starts at byte 5
    static constexpr size_t CHECKSUM_SUM_END = 68;    // exclusive: sums bytes 5-68 (only header + checksum byte excluded)
    static constexpr uint32_t Q14_SCALE = 16384;      // 2^14 fixed-point scale

    // Fallback timezone offset used only when utc_enable is on but the line
    // has no parseable TZ field (Turkey is fixed GMT+3 year-round).
    static constexpr int DEFAULT_TZ_OFFSET_SEC = 3 * 3600;

    // Fixed system message template. Only the time field (bytes 49-52) and the
    // trailing checksum (byte 69) are overwritten per packet; every other byte
    // is constant. Captured from a reference packet emitted by the device.
    static constexpr uint8_t PACKET_TEMPLATE[PACKET_SIZE] = {
        0xCA, 0xE3, 0x40, 0x13, 0xC9, 0x27, 0x9F, 0x44, 0x80, 0xD0,  // 0-9
        0xAE, 0x24, 0xF2, 0x19, 0x00, 0x00, 0x01, 0x0B, 0x71, 0x1C,  // 10-19
        0xF0, 0x46, 0x4A, 0x17, 0x03, 0x03, 0x7A, 0x00, 0x91, 0xBB,  // 20-29
        0x0B, 0x00, 0x09, 0xD9, 0x12, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // 30-39
        0x07, 0x20, 0x89, 0x09, 0x67, 0x04, 0x40, 0x02, 0x00, 0x00,  // 40-49 (48-51: time field)
        0x00, 0x00, 0x2D, 0x30, 0x30, 0x34, 0x00, 0x00, 0x00, 0x00,  // 50-59
        0x00, 0x00, 0x2F, 0x40, 0xB4, 0x15, 0xB9, 0x07, 0xF1         // 60-68 (68: checksum)
    };

    // --- Date packet (sent back-to-back right after the time packet) --------
    // Carries the calendar date as three little-endian 16-bit fields:
    //   year  @ bytes 29-30, month @ bytes 31-32, day @ bytes 33-34.
    // Checksum uses the same rule as the time packet: sum data field bytes
    // (byte 5 .. byte before the checksum), two's complement, trailing byte.
    static constexpr size_t DATE_PACKET_SIZE = 55;          // full date message
    static constexpr size_t DATE_YEAR_OFFSET = 28;          // bytes 29-30 (LE)
    static constexpr size_t DATE_MONTH_OFFSET = 30;         // bytes 31-32 (LE)
    static constexpr size_t DATE_DAY_OFFSET = 32;           // bytes 33-34 (LE)
    static constexpr size_t DATE_CHECKSUM_OFFSET = 54;      // trailing checksum, byte 55
    static constexpr size_t DATE_CHECKSUM_SUM_BEGIN = 4;    // data field starts at byte 5
    static constexpr size_t DATE_CHECKSUM_SUM_END = 54;     // exclusive: sums bytes 5-54

    // Fixed date message template. Only the date fields (bytes 29-34) and the
    // trailing checksum (byte 55) are overwritten per packet.
    static constexpr uint8_t DATE_PACKET_TEMPLATE[DATE_PACKET_SIZE] = {
        0xCA, 0xE5, 0x32, 0x1F, 0x30, 0x2E, 0x9F, 0x44, 0x00, 0x00,  // 0-9
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 10-19
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEA, 0x07,  // 20-29 (28-29: year)
        0x06, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 30-39 (30-31: month, 32-33: day)
        0x1A, 0x4F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 40-49
        0x00, 0x00, 0x00, 0x00, 0x47                                 // 50-54 (54: checksum)
    };

    /**
     * @brief Worker thread main loop
     */
    void workerLoop();

    /**
     * @brief Verification thread - reads packets from verify port
     */
    void verifyLoop();

    /**
     * @brief Parses time string and converts to Unix timestamp
     * @param time_str Time string in format "YYYY DDD HH:MM:SS"
     * @return Unix timestamp, or 0 on parse error
     */
    uint32_t parseTimeString(const std::string& time_str);

    /**
     * @brief Extracts the timezone offset from a SyncServer line
     *
     * Handles trailing offset fields like "+03", "-05", "S+03", "+0530",
     * "+05:30". The offset is how far the local time is ahead of UTC, so to
     * convert local->UTC the caller subtracts it.
     *
     * @param time_str       Full input line
     * @param offset_seconds Output: offset in seconds (e.g. +3h -> 10800)
     * @return true if an offset field was found and parsed
     */
    bool parseTzOffset(const std::string& time_str, int& offset_seconds);

    /**
     * @brief Converts day of year to month and day
     * @param year Year (for leap year calculation)
     * @param doy Day of year (1-366)
     * @param month Output: month (1-12)
     * @param day Output: day of month (1-31)
     * @return true if valid
     */
    bool dayOfYearToDate(int year, int doy, int& month, int& day);

    /**
     * @brief Checks if year is a leap year
     */
    bool isLeapYear(int year);

    /**
     * @brief Builds the output packet for a given time of day
     *
     * Copies the fixed template, writes the Q14 fixed-point seconds-of-day
     * value into the time field (bytes 49-52, little-endian) and recomputes
     * the trailing checksum.
     *
     * @param buffer         Output buffer (must be PACKET_SIZE bytes)
     * @param seconds_of_day Seconds since midnight UTC (0-86399)
     */
    void buildPacket(uint8_t* buffer, uint32_t seconds_of_day);

    /**
     * @brief Builds the date packet for a given calendar date
     *
     * Copies the date template, writes year/month/day as little-endian 16-bit
     * fields (bytes 29-34) and recomputes the trailing checksum (byte 55).
     *
     * @param buffer Output buffer (must be DATE_PACKET_SIZE bytes)
     * @param year   Full year (e.g. 2026)
     * @param month  Month (1-12)
     * @param day    Day of month (1-31)
     */
    void buildDatePacket(uint8_t* buffer, int year, int month, int day);

    /**
     * @brief Computes a trailing checksum over a data field
     *
     * Sums buffer bytes in [begin, end) modulo 256 and returns the two's
     * complement of the low 8 bits, so that the data field plus the checksum
     * sum to zero (mod 256). Shared by the time and date packets.
     *
     * @param buffer Packet buffer
     * @param begin  First data-field byte offset (inclusive)
     * @param end    One past the last data-field byte offset (exclusive)
     * @return Checksum byte
     */
    uint8_t computeChecksum(const uint8_t* buffer, size_t begin, size_t end) const;

    /**
     * @brief Sets last error message
     */
    void setLastError(const std::string& error);
};

} // namespace serial

#endif // SERIAL_TIME_FORWARDER_H

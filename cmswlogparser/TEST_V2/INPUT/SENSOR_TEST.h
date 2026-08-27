#include <stdint.h>
#include <stdbool.h>

/* ---- Enums ---- */

typedef enum {
    DEVICE_OK      = 0,
    DEVICE_ERROR   = 1,
    DEVICE_OFFLINE = 2
} DeviceStatus;

typedef enum {
    LOG_DEBUG   = 0,
    LOG_INFO    = 1,
    LOG_WARNING = 2,
    LOG_ERROR   = 3
} LogLevel;

/* ---- Nested Structs (Level 2) ---- */

typedef struct __attribute__((packed)) {
    uint32_t seconds;
    uint16_t milliseconds;
} Timestamp;

typedef struct __attribute__((packed)) {
    uint32_t latitude;
    uint32_t longitude;
    uint16_t altitude;
} GpsCoord;

typedef struct __attribute__((packed)) {
    uint8_t  major;
    uint8_t  minor;
    uint16_t patch;
} FirmwareInfo;

/* ---- Nested Structs (Level 1) ---- */

typedef struct __attribute__((packed)) {
    GpsCoord coord;
    uint8_t  satelliteCount;
    bool     hasFix;
} Location;

/* ---- Top-Level Structs ---- */

typedef struct __attribute__((packed)) {
    uint32_t    magicNumber;
    uint16_t    version;
    Timestamp   createdAt;
    uint8_t     flags;
} FileHeader;

typedef struct __attribute__((packed)) {
    uint32_t      deviceId;
    DeviceStatus  status;
    FirmwareInfo  firmware;
    uint32_t      uptime;
} DeviceInfo;

typedef struct __attribute__((packed)) {
    uint16_t    readingId;
    Location    location;
    Timestamp   ts;
    LogLevel    level;
    int16_t     temperature;
    uint16_t    humidity;
    uint32_t    pressure;
    uint16_t    batteryVoltage;
    uint8_t     signalStrength;
    uint32_t    errorCode;
    bool        isValid;
    uint16_t    mode : 5;
    uint16_t    priority : 4;
    uint16_t    flags : 7;
    uint16_t    checksum;
    uint32_t    sequenceNum;
    uint16_t    sampleRate;
    uint32_t    duration;
    uint16_t    amplitude;
    uint16_t    frequency;
    uint8_t     gain;
} SensorReading;

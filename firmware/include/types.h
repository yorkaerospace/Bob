#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

/* Defines types for this project so we arent constantly pulling in whole
 * modules for no reason. Also avoids circular dependencies */

typedef struct {
    uint64_t time;       // ms from boot
    // Raw data
    uint32_t pres;       // Pascals
    int32_t  temp;       // CentiDegrees
    // Filtered
    int32_t  vVel;       // Arbitrary units
} baro_t;

typedef struct {
    uint64_t time;       // ms from boot
    int16_t  compass[3]; // Arbitrary units
} comp_t;

typedef struct {
    uint64_t time;       // ms from boot
    // Raw
    int16_t  accl[3];    // Arbitrary units
    int16_t  gyro[3];    // Arbitrary units
    // Processed
    int32_t  accl_mag;   // Arbitrary units ^ 2
} imu_t;

typedef struct {
    uint64_t time;       // ms from boot
    uint8_t  utc[3];     // Hours, minutes seconds
    int32_t  lon, lat;
    uint16_t sats;
} gps_t;

typedef struct {
    uint8_t marker;      // 0xAA
    uint8_t size;        // Bytes, not including header
    uint8_t type;
    uint8_t data[UINT8_MAX];
} log_t;

// Configuration values written to flash
typedef struct {
    uint32_t glPres;     // Pressure at ground level
    uint32_t mainPres;   // Pressure to deploy main at
} conf_t;

enum states {
    BOOT,         // Sit here for some time, let the filters filter.
    PLUGGED_IN,   // Connected to USB
    GROUNDED,     // On the ground and unplugged
    BOOST,        // Motor is firing. Guard against deployment.
    COAST,        // Motor burnout.
    DROGUE_FIRE,  // Apogee
    DROGUE_OUT,   // Drogue deployed
    MAIN_FIRE,    // Below altitude gate
    MAIN_OUT,     // Main deployed
    LANDED,       // Woot back on ground
    FAULT_HANDLER // Oh god we're flying
};

#endif

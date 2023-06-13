#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

/* Defines types for this project so we arent constantly pulling in whole
 * Header files. Also means everything is in one place. */

typedef struct {
    uint64_t time;       // ms from boot
    // Raw data
    uint32_t pres;       // Pascals
    int32_t  temp;       // CentiDegrees
    // Filtered
    int32_t  vVel;       // Arbitrary units
    uint32_t agl;        // Meters
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
    int16_t  accl_mag;   // Arbitrary units
} imu_t;

enum states {
    BOOT,         // Sit here for some time, let the filters filter.
    PLUGGED_IN,   // Connected to USB
    DATA_OUT,     // Printing flight data to USB
    DEBUG_PRINT,  // Printing debug data to USB
    DEBUG_LOG,    // Logging data while plugged in
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

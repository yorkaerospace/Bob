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



#endif

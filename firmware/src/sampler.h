#ifndef SAMPLER_H
#define SAMPLER_H

#include <pico/stdlib.h>
#include <stdint.h>

typedef struct {
    uint8_t status;
    uint32_t time;    // Time since boot in ms

    uint32_t pres;    // Pressure in pascals
    int32_t temp;     // Temperature in centidegrees.

    int16_t mag[3];   // Raw magentometer output X, Y, Z;

    int16_t accel[3]; // Raw IMU output X, Y, Z;
    int16_t gyro[3];
} sample_t;

/* Initialises the sensors and the associated i2c bus */
void configureSensors(void);

/* Gets a sample from the sensors.
 * No longer attempts to determine if sensors are functional.
 * If they don't respond, they dont respond. */
sample_t getSample(void);

/* Writes a sample to flash.
 * No longer faffs around with buffering. Just yheets it onto flash. */
void logSample(sample_t sample);

/* Reads a sample from flash */
uint8_t readSample (size_t index, sample_t * sample);

/* Clears the flash */
void clearFlash(void)

#endif

#ifndef SENSORS_H
#define SENSORS_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include <hp203b.h>
#include <qmc5883l.h>
#include <qmi8658c.h>
#include <stdint.h>
#include <string.h>

// Consitent naming schemes are for chumps.
typedef struct {
    uint32_t pres;    // Pressure in pascals
    int32_t temp;     // Temperature in centidegrees.

    int16_t mag[3];   // Raw magentometer output X, Y, Z;

    int16_t accel[3]; // Raw IMU output X, Y, Z;
    int16_t gyro[3];

    uint32_t time;    // Time since boot in ms
    uint8_t status;   // Bitfield for status flags. See below.
} data_t;

enum statusFlags {
    NO_BARO = 1 << 0, // Sensors disabled or faulty
    NO_GYRO = 1 << 1,
    NO_ACCL = 1 << 2,
    NO_COMP = 1 << 3,
    LAUNCH  = 1 << 4, // State changes on launch
    APOGEE  = 1 << 5,
    LANDING = 1 << 6,
    FAULT   = 1 << 7  // Recovered from a fault
};

/* Initialises the sensors and the associated i2c bus */
void configureSensors(void);

/* Tests all the sensors and returns a bitfield showing which sensors
 * are active.
 * Bit | Sensor
 *  0  | HP203B   Barometer
 *  1  | QMI8658C Gyroscope
 *  2  | QMI8658C Accelerometer
 *  3  | QMC5883L Compass
 * Bit set high indicates a faulty sensor. */
uint8_t testSensors(void);

/* Polls the sensors and returns a data struct
 * Takes a bitfield showing the status of all sensors. */
data_t pollSensors(uint8_t status);

#endif

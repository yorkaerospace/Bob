#ifndef SENSORS_H
#define SENSORS_H

/* A module for managing and polling the sensors as a unit
 * Includes functions and structs for managing the data produced, as they're
 * dependent on the configuration of the sensors. */

#include <stdint.h>

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

/* Returns the magnitude of the vector passed to it.
 * Basically just pythagoras. */
float magnitude(int16_t vector[3]);

/* Takes a raw accelerometer reading and returns a value in G */
float accelToG(int16_t accel);

/* Takes a value in G and calculates what the QMI would read */
int16_t gToAccel(float g);

/* Takes a raw reading from the gyro and returns a value in dps */
float gyroToDps(int16_t gyro);

/* Applies calibration coefficients to a magnetometer reading */
void compCalib(int16_t * reading[3], int16_t calib[3]);

#endif

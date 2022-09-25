#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>
#include "data.h"

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

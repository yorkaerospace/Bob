#include "sensors.h"

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include <hp203b.h>
#include <qmc5883l.h>
#include <qmi8658c.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "dataBuf.h"
#include "states.h"

// Constants defining constants is... mildly cursed, but its the nicest way
// I could think of to set these universally
static const enum QMIGyroScale GYRO_SCALE = QMI_GYRO_256DPS;
static const enum QMIAccelScale ACC_SCALE = QMI_ACC_16G;
static const enum QMCScale COMP_SCALE = QMC_SCALE_2G;

extern volatile uint8_t state;

// Sensor structs
static hp203_t hp203;
static qmc_t qmc;
static qmi_t qmi;


/* Initialises the sensors and the associated i2c bus */
void configureSensors(void) {
    struct qmc_cfg qmcCfg;
    uint8_t status; // System status
    int8_t result;  // Stores the results of tests until we need them.

    // Configure the i2c bus.
    i2c_init(i2c_default, 400*1000);
    gpio_set_function(16, GPIO_FUNC_I2C);
    gpio_set_function(17, GPIO_FUNC_I2C);
    gpio_pull_up(16);
    gpio_pull_up(17);

    // Initialise sensor structs
    hp203 = HP203Init(i2c_default);
    qmc = QMCInit(i2c_default);
    qmi = QMIInit(i2c_default, true);

    // Configure the QMI's gyro
    QMIGyroConfig(&qmi, QMI_GYRO_125HZ, GYRO_SCALE);
    QMISetOption(&qmi, QMI_GYRO_ENABLE, true);
    QMISetOption(&qmi, QMI_GYRO_SNOOZE, false);

    // Configure the QMI's accelerometer
    QMIAccConfig(&qmi, QMI_ACC_125HZ, ACC_SCALE);
    QMISetOption(&qmi, QMI_ACC_ENABLE, true);

    // Configure the QMC
    qmcCfg.mode = QMC_CONTINUOUS;
    qmcCfg.ODR = QMC_ODR_10HZ;
    qmcCfg.OSR = QMC_OSR_256;
    qmcCfg.scale = COMP_SCALE;
    qmcCfg.pointerRoll = true;
    qmcCfg.enableInterrupt = false;

    QMCSetCfg(&qmc, qmcCfg);

}

/* Tests all the sensors and returns a bitfield showing which sensors
 * are active.
 * Bit | Sensor
 *  0  | HP203B   Barometer
 *  1  | QMI8658C Gyroscope
 *  2  | QMI8658C Accelerometer
 *  3  | QMC5883L Compass
 * Bit set high indicates a faulty sensor. */
uint8_t testSensors(void) {
    uint8_t result;
    int8_t status;

    status = HP203Test(&hp203);

    result = status == 0 ? 0 : NO_BARO;

    status = QMITest(&qmi);

    switch (status) {
    case QMI_OK:
        break;
    case QMI_NO_ACCEL:
        result |= NO_ACCL;
        break;
    case QMI_NO_GYRO:
        result |= NO_GYRO;
        break;
    default:
        result |= NO_ACCL | NO_GYRO;
    }

    QMCTest(&qmc);

    result |= status == 0 ? 0 : NO_COMP;

    return result;
}

/* Polls the sensors and returns a data struct
 * Takes a bitfield showing the status of all sensors. */
data_t pollSensors(uint8_t status) {
    data_t data;
    int32_t i2cStatus;
    absolute_time_t hp203Ready;

    struct hp203_data barometer;
    struct qmi_data imu;

    data.time = to_ms_since_boot(get_absolute_time());

    if((status & NO_BARO) == 0) {
        i2cStatus = HP203Measure(&hp203, HP203_PRES_TEMP, HP203_OSR_256);
        if(i2cStatus < HP203_OK) {    // TODO: Retest sensors when appropriate.
            status |= NO_BARO;
        } else {
            hp203Ready = make_timeout_time_us(i2cStatus);
        }
    }

    if((status & NO_ACCL) == 0 || (status & NO_GYRO) == 0) {
        i2cStatus = QMIReadData(&qmi, &imu);
        if(i2cStatus != QMI_OK) {
            status |= NO_ACCL | NO_GYRO;
        } else {
            memcpy(data.accel, imu.accel, 6);
            memcpy(data.gyro, imu.gyro, 6);
        }
    }

    if((status & NO_COMP) == 0) {
        i2cStatus = QMCGetMag(&qmc, data.mag);
        status |= i2cStatus != QMC_OK ? 0 : NO_COMP;
    }

    if((status & NO_BARO) == 0) {
        sleep_until(hp203Ready);
        i2cStatus = HP203GetData(&hp203, &barometer);
        if(i2cStatus < HP203_OK) {
            status |= NO_BARO;
        } else {
            data.pres = barometer.pres;
            data.temp = barometer.temp;
        }
    }

    data.status = status;
    return data;
}

/* Returns the magnitude of the vector passed to it.
 * Basically just pythagoras. */
float magnitude(int16_t vector[3]) {
    return sqrt(pow(vector[0], 2) + pow(vector[1], 2) + pow(vector[2], 2));
}

/* Takes a raw accelerometer reading and returns a value in G */
float accelToG(int16_t accel) {
    uint8_t scale = pow(2, ACC_SCALE + 1);
    return ((float) accel / __INT16_MAX__) * (float) scale;
}

/* Takes a value in G and calculates what the QMI would read */
int16_t gToAccel(float g) {
    uint8_t scale = pow(2, ACC_SCALE + 1);
    return (__INT16_MAX__/scale) * g;
}

/* Takes a raw reading from the gyro and returns a value in dps */
float gyroToDps(int16_t gyro) {
    uint16_t scale = pow(2, GYRO_SCALE + 4);
    return ((float) gyro / __INT16_MAX__) * (float) scale;
}

/* Applies calibration coefficients to a magnetometer reading */
void compCalib(int16_t * reading[3], int16_t calib[3]) {
    reading[0] -= calib[0];
    reading[1] -= calib[1];
    reading[2] -= calib[2];
}

/* Finds the difference in pressure between the most recent
 * data packet and the one that was t samples ago . */
int32_t deltaPres(size_t t) {
    data_t curr, past;
    dataHead(&curr);
    dataRel(&past, t);
    uint32_t currPres = curr.pres;
    uint32_t pastPres = past.pres;
    return currPres - pastPres;
}

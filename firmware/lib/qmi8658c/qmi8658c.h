#ifndef QMI8658C_H
#define QMI8658C_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"

/* A libary for operating the QMI8658C IMU from QST Corporation
 * Does not support Attitude Engine or Magnetometer integration.*/

enum QMIRegister {
    // WHOAMI?
    QMI_WHO_AM_I = 0x00,       // Should contain 0x05

    // Settings registers
    QMI_CTRL_ACC = 0x03,       // Accelerometer settings
    QMI_CTRL_GYRO = 0x04,      // Gyro settings
    QMI_CTRL_LPF = 0x06,       // Low pass filter settings
    QMI_CTRL_ENB = 0x08,       // Enable sensors
    QMI_CTRL_CMD = 0x0A,       // Host commands

    // Status registers
    QMI_STATUSINT = 0x2D,      // Sensor data availability
    QMI_STATUS0 = 0x2E,        // Output data overrun
    QMI_STATUS1 = 0x2F,        // Misc status

    // Sample timestamp
    QMI_TIMESTAMP_LSB = 0x30,
    QMI_TIMESTAMP_MID = 0x31,
    QMI_TIMESTAMP_MSB = 0x32,

    // Output registers
    QMI_TEMP_LSB = 0x33,
    QMI_TEMP_MSB = 0x34,

    QMI_ACC_X_LSB = 0x35,
    QMI_ACC_X_MSB = 0x36,
    QMI_ACC_Y_LSB = 0x37,
    QMI_ACC_Y_MSB = 0x38,
    QMI_ACC_Z_LSB = 0x39,
    QMI_ACC_Z_MSB = 0x3A,

    QMI_GYRO_X_LSB = 0x3B,
    QMI_GYRO_X_MSB = 0x3C,
    QMI_GYRO_Y_LSB = 0x3D,
    QMI_GYRO_Y_MSB = 0x3E,
    QMI_GYRO_Z_LSB = 0x3F,
    QMI_GYRO_Z_MSB = 0x40,

    QMI_RESET = 0x60
};

#define QMI_SCALE_OFFSET 4

enum QMIAccelScale {
    QMI_ACC_2G  = 0x00,
    QMI_ACC_4G  = 0x01,
    QMI_ACC_8G  = 0x02,
    QMI_ACC_16G = 0x03
};

enum QMIGyroScale {
    QMI_GYRO_16DPS = 0x00,
    QMI_GYRO_32DPS = 0x01,
    QMI_GYRO_64DPS = 0x02,
    QMI_GYRO_128DPS = 0x03,
    QMI_GYRO_256DPS = 0x04,
    QMI_GYRO_512DPS = 0x05,
    QMI_GYRO_1024DPS = 0x06,
    QMI_GYRO_2048DPS = 0x07
};

enum QMIAccelODR {
    QMI_ACC_8KHZ = 0x00,
    QMI_ACC_4KHZ = 0x01,
    QMI_ACC_2KHZ = 0x02,
    QMI_ACC_1KHZ = 0x03,
    QMI_ACC_500HZ = 0x04,
    QMI_ACC_250HZ = 0x05,
    QMI_ACC_125HZ = 0x06,
    QMI_ACC_63HZ = 0x07,
    QMI_ACC_32HZ = 0x08,

    // Low power settings
    QMI_ACC_LP_128HZ = 0x0C,
    QMI_ACC_LP_21HZ = 0x0D,
    QMI_ACC_LP_11HZ = 0x0E,
    QMI_ACC_LP_3HZ = 0x0F
};

enum QMIGyroODR {
    QMI_GYRO_8KHZ = 0x00,
    QMI_GYRO_4KHZ = 0x01,
    QMI_GYRO_2KHZ = 0x02,
    QMI_GYRO_1KHZ = 0x03,
    QMI_GYRO_500HZ = 0x04,
    QMI_GYRO_250HZ = 0x05,
    QMI_GYRO_125HZ = 0x06,
    QMI_GYRO_63HZ = 0x07,
    QMI_GYRO_32HZ = 0x08
};

// Settings for the CTRL7 register
enum QMIOpt {
    QMI_ACC_ENABLE  = 1 << 0,
    QMI_GYRO_ENABLE = 1 << 1,
    QMI_GYRO_SNOOZE = 1 << 4,
    QMI_SYS_HS      = 1 << 6,
    QMI_SYNC_SMPL   = 1 << 7
};

// Status codes
enum QMIStatus {
    QMI_NO_GYRO        =  3,
    QMI_NO_ACCEL       =  2,
    QMI_NO_SENSORS     =  1,
    QMI_OK             =  0,
    QMI_ERROR_TIMEOUT  = -1,
    QMI_ERROR_GENERIC  = -2,
    QMI_ERROR_BAD_CONF = -3
};

#define QMI_ADDR 0x6A       // Can be changed to 0x6B by pulling SA0 high.
#define QMI_TIMEOUT 1000    // This hasnt been done on Bob, but its worth noting

typedef struct {
    i2c_inst_t * i2c;
    uint8_t addr;
} qmi_t;

// The QMI really wants you to take big reads. Its kinda weird tbh.
struct qmi_data {
    uint32_t timestamp;
    int16_t temp;
    int16_t accel[3];
    int16_t gyro[3];
};

/* Generates a QMI_T struct.
 * SA0 is used to set the address, on Bob this should be set to false. */
qmi_t QMIInit(i2c_inst_t * i2c, bool SA0);

/* The QMI has a self test system, but THEY HAVENT DOCUMENTED IT YET >:(
 * Checks if the QMI talks. Returns:
 * QMI_OK if everything seems good.
 * QMI_ERROR_TIMEOUT if the I2C timesout.
 * QMI_ERROR_GENERIC for other errors.
 *
 * N.B. If you're getting QMI_GENERIC errors, check that SA0 is set correctly */
int8_t QMITest(qmi_t * qmi);

/* Turns an option on or off.
 * Currently the following options are supported:
 * QMI_ACC_ENABLE  - Enables/disables accelerometer
 * QMI_GYRO_ENABLE - Enables/disables gyroscope
 * QMI_GYRO_SNOOZE - Puts the gyro into snooze mode?
 * QMI_SYS_HS      - Enables/disables the high speed clock?
 * QMI_SYNC_SMPL   - Enables/disables simple sync.

 * Returns:
 * The current value of CTRL7 if successful
 * QMI_ERROR_TIMEOUT if the I2C timesout.
 * QMI_ERROR_GENERIC for other errors. */
int16_t QMISetOption(qmi_t * qmi, enum QMIOpt option, bool set);

/* Configures the accelerometer.
 * Returns:
 * The value of CTRL2 if successful.
 * QMI_ERROR_TIMEOUT if the I2C timesout.
 * QMI_ERROR_GENERIC for other errors */
int8_t QMIAccConfig(qmi_t * qmi, enum QMIAccelODR odr, enum QMIAccelScale scl);

/* Configures the gyroscope.
 * Returns:
 * The value of CTRL3 if successful.
 * QMI_ERROR_TIMEOUT if the I2C timesout.
 * QMI_ERROR_GENERIC for other errors */
int8_t QMIGyroConfig(qmi_t * qmi, enum QMIGyroODR odr, enum QMIGyroScale scl);

/* Reads the data off the QMI and writes it to data
 * Returns:
 * QMI_OK if successful.
 * QMI_ERROR_TIMEOUT if the I2C timesout.
 * QMI_ERROR_GENERIC for other errors */
int8_t QMIReadData(qmi_t * qmi, struct qmi_data * data);

#endif

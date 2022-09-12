#include "qmc5883l.h"

/* Wrapper around i2c write function */
static int QMCWriteByte(qmc_t * sensor, enum QMCRegister reg, uint8_t value) {
    uint8_t buffer[2];
    buffer[0] = reg;
    buffer[1] = value;
    return i2c_write_timeout_us(sensor->i2c, QMC_ADDR, &buffer,
                                2, true, QMC_TIMEOUT);
}

/* Wrapper around i2c read function */
static int QMCReadBytes(qmc_t * sensor, enum QMCRegister reg, size_t len, uint8_t * buffer) {
    uint8_t regShort = reg; // Trim the enum to a uint8_t.
    i2c_write_timeout_us(sensor->i2c, QMC_ADDR, &regShort,
                        1, true, QMC_TIMEOUT);
    return i2c_read_timeout_us(sensor->i2c, QMC_ADDR, buffer,
                               len, false, QMC_TIMEOUT);
}

/* Creates a QMC_T */
qmc_t QMCInit(i2c_inst_t * i2c) {
    qmc_t sensor;
    sensor.i2c = i2c;
    QMCWriteByte(&sensor, QMC_SETRESET, 0x01);
    // Attempt to get the current config
    QMCGetCfg(&sensor);
    return sensor;
}

/* The QMC5883L doesnt have any real self-test capability, but we can
 * at least make sure it is talking properly and if it is configured correctly.
 * Returns:
 * 0 if the QMC is talking and is configured to produce data.
 * -1 if the QMC is talking, has a valid configuration, but is on standby.
 * -2 if the QMC has an invalid configuration.
 * -3 if the I2C hits a timeout
 * -4 for other errors */
int8_t QMCTest(qmc_t * sensor) {
    switch(QMCGetCfg(sensor)) {
    case 0:
        if(sensor->config.mode != QMC_STANDBY) {
            return 0;
        } else {
            return -1;
        }
    case -1:
        return -2;
    case -2:
        return -3;
    default:
        return -4;
    }
}

/* Reads and parses the QMC status register, places the result in status
 * Returns:
 * 0 if successful
 * -1 if the I2C times out
 * -2 for other errors */
int8_t QMCGetStatus(qmc_t * sensor, struct qmc_status * status) {
    uint8_t reg;
    int8_t i2cState = QMCReadBytes(sensor, QMC_STATUS, 1, &reg);

    if(i2cState == 1) {
        status->dataReady = reg & 1 << QMC_DRDY;
        status->dataOverflow = reg & 1 << QMC_DOVL;
        status->dataSkip = reg & 1 << QMC_DSKIP;
        return 0;
    } else if (i2cState == PICO_ERROR_TIMEOUT) {
        return -1;
    } else {
        return -2;
    }
}

/* A quick way to configure the QMC5883L, based on config.
 * N.B. the control 1 & 2 raw fields in config can be safely ignored
 * Returns:
 * 0 if successful
 * -1 if the I2C times out
 * -2 for other errors */
int8_t QMCSetCfg(qmc_t * sensor, struct qmc_cfg config) {

    config.control1Raw = config.mode            << QMC_MODE_SHIFT
                       | config.ODR             << QMC_ODR_SHIFT
                       | config.OSR             << QMC_OSR_SHIFT
                       | config.scale           << QMC_SCALE_SHIFT;

    config.control2Raw = config.pointerRoll     << QMC_ROL_PNT
                       | config.enableInterrupt << QMC_INT_ENB;

    int8_t i2cState[2];

    i2cState[0] = QMCWriteByte(sensor, QMC_CONTROL1, config.control1Raw);
    i2cState[1] = QMCWriteByte(sensor, QMC_CONTROL2, config.control2Raw);
    /* The QMC only increments some pointers, so configuration has to be done as
     * 2 seperate writes. */
    if(i2cState[0] == 2 && i2cState[1] == 2) {
        sensor->config = config;
        return 0;
    } else if (i2cState[0] == PICO_ERROR_GENERIC
               || i2cState[1] == PICO_ERROR_GENERIC) {
        return -2;
    } else if (i2cState[0] == PICO_ERROR_TIMEOUT
               || i2cState[1] == PICO_ERROR_TIMEOUT) {
        return -1;
    } else {
        return -2;
    }
}

/* Reads and parses the config registers on the QMC5883L
 * Stores the result in sensor->config, alongside the raw registers.
 * Returns:
 * 0 if successful
 * -1 if the config is invalid
 * -2 if the i2c times out
 * -3 for other errors*/
int8_t QMCGetCfg(qmc_t * sensor) {
    uint8_t buffer[2];
    int8_t i2cState[2];
    bool invalid = false;

    // Again, the pointers dont increment, so we need to do 2 reads.
    i2cState[0] = QMCReadBytes(sensor, QMC_CONTROL1, 1, buffer);
    i2cState[1] = QMCReadBytes(sensor, QMC_CONTROL2, 1, buffer + 1);

    if(i2cState[0] == 1 && i2cState[1] == 1) {

        sensor->config.ODR = (buffer[0] >> QMC_ODR_SHIFT) & 3;
        sensor->config.OSR = (buffer[0] >> QMC_OSR_SHIFT) & 3;
        sensor->config.mode = buffer[0] & 1;
        sensor->config.scale = (buffer[0] >> QMC_SCALE_SHIFT) & 1;

        if(buffer[0] & 0x22) {
            invalid = true;
        }

        return invalid ? -1 : 0;

    } else if (i2cState[0] == PICO_ERROR_GENERIC
            || i2cState[1] == PICO_ERROR_GENERIC) {
        return -3;
    } else if (i2cState[0] == PICO_ERROR_TIMEOUT
            || i2cState[1] == PICO_ERROR_TIMEOUT) {
        return -2;
    } else {
        return -3;
    }
}

/* Reads data from the magnetometer and stores it in data
 * Returns:
 * 0 if successful.
 * -1 if the i2c times out.
 * -2 for other errors */
int8_t QMCGetMag(qmc_t * sensor, struct magData * data) {
    uint8_t buffer[6];
    int8_t i2cState = QMCReadBytes(sensor, QMC_XOUT_LSB, 6, buffer);
    if(i2cState == 6) {
        data->x = buffer[0] | (buffer[1] << 8);
        data->y = buffer[2] | (buffer[3] << 8);
        data->z = buffer[4] | (buffer[5] << 8);
    } else if (i2cState == PICO_ERROR_TIMEOUT) {
        return -1;
    } else {
        return -2;
    }
}

/* Reads temperature from the magnetometer and stores it in result
 * Returns:
 * 0 if successful.
 * -1 if the i2c times out.
 * -2 for other errors */
int8_t QMCGetTemp(qmc_t * sensor, int16_t * result) {
    uint8_t buffer[2];
    int8_t i2cState[2];

    i2cState[0] = QMCReadBytes(sensor, QMC_TEMP_LSB, 1, buffer);
    i2cState[1] = QMCReadBytes(sensor, QMC_TEMP_MSB, 1, buffer + 1);
    // Why doesnt it increment pointers thats so annoying >:(
    if(i2cState[0] == 1 && i2cState[1] == 1) {
        *result = buffer[0] | (buffer[1] << 8);
        return 0;
    } else if (i2cState[0] == PICO_ERROR_GENERIC
               || i2cState[1] == PICO_ERROR_GENERIC) {
        return -2;
    } else if (i2cState[0] == PICO_ERROR_TIMEOUT
               || i2cState[1] == PICO_ERROR_TIMEOUT) {
        return -1;
    } else {
        return -2;
    }
}

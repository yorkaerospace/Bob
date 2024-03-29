#include "qmi8658c.h"

static int8_t QMIWriteByte(qmi_t *qmi, enum QMIRegister reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    i2c_write_timeout_per_char_us(qmi->i2c, qmi->addr, buf,
                                  2, false, QMI_TIMEOUT);
}

static int8_t QMIReadBytes(qmi_t *qmi, enum QMIRegister reg, uint8_t *buffer, size_t len)
{
    uint8_t buf = reg;
    uint8_t i2cState[2];
    uint32_t i;

    // The QMI is supposed to autoincrement register pointers.
    // It does not. :(
    for(i = 0; i < len; i++)
    {
        i2cState[0] = i2c_write_timeout_per_char_us(qmi->i2c, qmi->addr,
                      &buf, 1, false,
                      QMI_TIMEOUT);
        i2cState[1] = i2c_read_timeout_per_char_us(qmi->i2c, qmi->addr,
                      buffer + i, 1, false,
                      QMI_TIMEOUT);

        if(i2cState[0] < 0 && i2cState[1] < 0)
        {
            return i2cState[0] < i2cState[1] ?
                   i2cState[0] : i2cState[1];
        }

        buf++;
    }

    return 0;
}

/*  Generates a QMI_T struct.
    SA0 is used to set the address, on Bob this should be set to false. */
qmi_t QMIInit(i2c_inst_t *i2c, bool SA0)
{
    qmi_t qmi;
    qmi.i2c = i2c;
    qmi.addr = QMI_ADDR | SA0;
    return qmi;
}

/*  The QMI has a self test system, but THEY HAVENT DOCUMENTED IT YET >:(
    Checks if the QMI talks and if the config is good. Returns:

    QMI_NO_GYRO if the gyro is disabled
    QMI_NO_ACCL if the accelerometer is disabled
    QMI_NO_SENSORS if both sensors are disabled
    QMI_OK if everything seems good, and both sensors are enabled
    QMI_ERROR_TIMEOUT if the I2C timesout.
    QMI_ERROR_BAD_CONF if the sensor configuration is invalid.
    QMI_ERROR_GENERIC for other errors.

    N.B. If you're getting QMI_GENERIC errors, check that SA0 is set correctly */
int8_t QMITest(qmi_t *qmi)
{
    uint8_t CTRL7;
    uint8_t CTRLACC;
    int8_t i2cStatus[2];
    int8_t worstErr;

    i2cStatus[0] = QMIReadBytes(qmi, QMI_CTRL_ENB, &CTRL7, 1);
    i2cStatus[1] = QMIReadBytes(qmi, QMI_CTRL_ACC, &CTRLACC, 1);

    worstErr = MIN(i2cStatus[0], i2cStatus[1]);

    if(worstErr < 0)
    {
        // If theres an error with the i2c, return the worst one
        return worstErr;
    }
    else if(CTRL7 & QMI_GYRO_ENABLE && CTRLACC & (0x0F > QMI_ACC_LP_128HZ))
    {
        // Tests if the user is attempting to use the QMI gyro in low power mode
        return QMI_ERROR_BAD_CONF;
    }
    else if(CTRL7 & ((QMI_GYRO_ENABLE | QMI_ACC_ENABLE) == 0))
    {
        // Tests if both sensors are disabled
        return QMI_NO_SENSORS;
    }
    else if(CTRL7 & (QMI_GYRO_ENABLE == 0))
        return QMI_NO_GYRO;

    else if(CTRL7 & (QMI_ACC_ENABLE == 0))
        return QMI_NO_ACCEL;

    return QMI_OK;

}

/*  Turns an option on or off.
    Currently the following options are supported:
    QMI_ACC_ENABLE  - Enables/disables accelerometer
    QMI_GYRO_ENABLE - Enables/disables gyroscope
    QMI_GYRO_SNOOZE - Puts the gyro into snooze mode?
    QMI_SYS_HS      - Enables/disables the high speed clock?
    QMI_SYNC_SMPL   - Enables/disables simple sync.

    Returns:
    The current value of CTRL7 if successful
    QMI_ERROR_TIMEOUT if the I2C timesout.
    QMI_ERROR_GENERIC for other errors. */
int16_t QMISetOption(qmi_t *qmi, enum QMIOpt option, bool set)
{
    uint8_t CTRL7;
    int8_t i2cStatus = QMIReadBytes(qmi, QMI_CTRL_ENB, &CTRL7, 1);

    if(i2cStatus == QMI_OK)
    {
        CTRL7 = set ? CTRL7 | option : CTRL7 & ~(option);
        i2cStatus = QMIWriteByte(qmi, QMI_CTRL_ENB, CTRL7);

        if(i2cStatus == QMI_OK)
            return CTRL7;
    }

    return i2cStatus;
}

/*  Configures the accelerometer.
    Returns:
    The value of CTRL2 if successful.
    QMI_ERROR_TIMEOUT if the I2C timesout.
    QMI_ERROR_GENERIC for other errors */
int8_t QMIAccConfig(qmi_t *qmi, enum QMIAccelODR odr, enum QMIAccelScale scl)
{
    uint8_t buf = (scl << QMI_SCALE_OFFSET) | odr;
    int8_t i2cStatus = QMIWriteByte(qmi, QMI_CTRL_ACC, buf);
    return i2cStatus == QMI_OK ? buf : i2cStatus;
}

/*  Configures the gyroscope.
    Returns:
    The value of CTRL3 if successful.
    QMI_ERROR_TIMEOUT if the I2C timesout.
    QMI_ERROR_GENERIC for other errors */
int8_t QMIGyroConfig(qmi_t *qmi, enum QMIGyroODR odr, enum QMIGyroScale scl)
{
    uint8_t buf = (scl << QMI_SCALE_OFFSET) | odr;
    int8_t i2cStatus = QMIWriteByte(qmi, QMI_CTRL_GYRO, buf);
    return i2cStatus == QMI_OK ? buf : i2cStatus;
}

/*  Reads the data off the QMI and writes it to data
    Returns:
    QMI_OK if successful.
    QMI_ERROR_TIMEOUT if the I2C timesout.
    QMI_ERROR_GENERIC for other errors */
int8_t QMIReadData(qmi_t *qmi, struct qmi_data *data)
{
    uint8_t buf[17];
    int8_t i2cStatus = QMIReadBytes(qmi, QMI_TIMESTAMP_LSB, buf, 17);

    if(i2cStatus == QMI_OK)
    {
        data->timestamp = buf[0] | buf[1] << 8 | buf[2] << 16;
        data->temp = buf[3] | buf [4] << 8;
        data->accel[0] = buf[5] | buf[6] << 8;
        data->accel[1] = buf[7] | buf[8] << 8;
        data->accel[2] = buf[9] | buf[10] << 8;
        data->gyro[0] = buf[11] | buf[12] << 8;
        data->gyro[1] = buf[13] | buf[14] << 8;
        data->gyro[2] = buf[15] | buf[16] << 8;

        return QMI_OK;
    }

    return i2cStatus;
}

/* Takes a raw accelerometer reading and returns a value in G */
float QMIAccG(int16_t accel, enum QMIAccelScale scl)
{
    uint8_t scale = pow(2, scl + 1);
    return ((float) accel / __INT16_MAX__) * (float) scale;
}

/* Takes a raw reading from the gyro and returns a value in dps */
float QMIGyroDPS(int16_t gyro, enum QMIGyroScale scl)
{
    uint16_t scale = pow(2, scl + 4);
    return ((float) gyro / __INT16_MAX__) * (float) scale;
}

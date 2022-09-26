#include "hp203b.h"

/* Simple init function for HP203. */
hp203_t HP203Init(i2c_inst_t * i2c) {
    hp203_t sensor;
    sensor.i2c = i2c;
    return sensor;
}

/* Sends a command to the HP203 */
static int HP203SendCommand(hp203_t * sensor, uint8_t command) {
    return i2c_write_timeout_per_char_us(sensor->i2c,
                                        HP203_ADDR,
                                        &command,
                                        1,
                                        false,
                                        HP203_TIMEOUT);
}

/* Reads bytes from the HP203 */
static int HP203ReadBytes(hp203_t * sensor, uint8_t * buffer, size_t len) {
    return i2c_read_blocking(sensor->i2c,
                                   HP203_ADDR,
                                   buffer,
                                   len,
                                   false);
}

/* Tests if the HP203 is functioning
 * Returns:
 * HP203_OK if chip is functioning normally.
 * HP203_ERROR_TIMEOUT if an i2c request times out.
 * HP203_ERROR_BADCHIP if the chip is somehow bad.
 * HP203_ERROR_GENERIC for other errors.
 *
 * Function takes approximately 10 ms to run. */
int8_t HP203Test(hp203_t * sensor) {
    uint8_t buffer[1];
    int result[3];              // Used to store the result of operations

    result[0] = HP203SendCommand(sensor, HP203_RESET);

    sleep_ms(10);               // Wait until the sensor is stable.

    result[1] = HP203SendCommand(sensor, HP203_READ_REG | HP203_INT_SRC);
    result[2] = HP203ReadBytes(sensor, buffer, 1);

    // Look over the results and figure out what to return.
    sleep_ms(10);
    int i;
    // If theres an error, find the most bad error
    if(result[0] < 0 || result[1] < 0 || result[2] < 0) {
        return result[0] < result[1] ?
              (result[0] < result[2] ? result[0] : result[2]):
              (result[1] < result[2] ? result[1] : result[2]);
    } else { // Check the 6th bit.
        return (buffer[0] & 0x20) != 0 ?
               HP203_ERROR_BADCHIP : HP203_OK;
    }
}

/* Tells the HP203 to start measuring data.
 * Returns:
 * The expected measurement time in us if successful
 * HP203_ERROR_TIMEOUT if the I2C write times out
 * HP203_ERROR_GENERIC for other errors */
int32_t HP203Measure(hp203_t * sensor, enum HP203_CHN channel, enum HP203_OSR OSR) {
    static const uint32_t timeLookup[7] =
        {131100, 65600, 32800, 16400, 8200, 4100, 2100};

    uint8_t command = HP203_ADC_SET
                    | OSR << HP203_OSR_SHIFT
                    | channel;

    int i2cState = HP203SendCommand(sensor, command);

    return i2cState == 1 ? timeLookup[channel + OSR] : i2cState;
}

/* Gets the pressure. Must be ran after a measurement has finished
 * Returns:
 * HP203_OK on success,
 * HP203_ERROR_TIMEOUT if the I2C write times out
 * HP203_ERROR_GENERIC for other errors */
int8_t HP203GetPres(hp203_t * sensor, uint32_t * result) {
    uint8_t buffer[3];
    int i2cState[2];

    i2cState[0] = HP203SendCommand(sensor, HP203_READ_P);
    i2cState[1] = HP203ReadBytes(sensor, buffer, 3);

    if(i2cState[0] == 1 && i2cState[1] == 3) {
        *result = buffer[2] | buffer[1] << 8 | buffer[0] << 16;
        return 0;
    } else {   // Return the worst bad error.
        return i2cState[0] < i2cState[1] ?
               i2cState[0] : i2cState[1];
    }
}

/* Gets the Temperature. Must be ran after a measurement has finished
 * Returns:
 * HP203_OK on success,
 * HP203_ERROR_TIMEOUT if the I2C write times out
 * HP203_ERROR_GENERIC for other errors */
int8_t HP203GetTemp(hp203_t * sensor, int32_t * result) {
    uint8_t buffer[3];
    int i2cState[2];

    i2cState[0] = HP203SendCommand(sensor, HP203_READ_T);
    i2cState[1] = HP203ReadBytes(sensor, buffer, 3);

    if(i2cState[0] == 1 && i2cState[1] == 3) {
        *result = buffer[2] | buffer[1] << 8 | buffer[0] << 16;
        *result |= *result & 1 << 20 ? 0xFFF00000 : 0;
        return 0;
    } else {   // Return the worst bad error.
        return i2cState[0] < i2cState[1] ?
               i2cState[0] : i2cState[1];
    }
}

/* Gets pressure and temperature in a single i2c read.
 * Returns:
 * HP203_OK on success,
 * HP203_ERROR_TIMEOUT if the I2C write times out
 * HP203_ERROR_GENERIC for other errors */
int8_t HP203GetData(hp203_t * sensor, struct hp203_data * result) {
    uint8_t buffer[6];
    int i2cState[2];

    i2cState[0] = HP203SendCommand(sensor, HP203_READ_PT);
    i2cState[1] = HP203ReadBytes(sensor, buffer, 6);

    if(i2cState[0] == 1 && i2cState[1] == 6) {
        result->pres = buffer[5] | buffer[4] << 8 | buffer[3] << 16;
        result->temp = buffer[2] | buffer[1] << 8 | buffer[0] << 16;
        result->temp |= result->temp & 1 << 20 ? 0xFFF00000 : 0;
    } else {   // Return the worst bad error.
        return i2cState[0] < i2cState[1] ?
               i2cState[0] : i2cState[1];
    }
}

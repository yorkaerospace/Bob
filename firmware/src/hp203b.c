#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hp203.h"

/* Simple init function for HP203. */
hp203_t HP203Init(i2c_inst_t * i2c) {
    hp203_t sensor;
    sensor.i2c = i2c;
    return sensor;
}

/* Sends a command to the HP203 */
int HP203SendCommand(hp203_t * sensor, uint8_t command) {
   return i2c_write_timeout_us(sensor->i2c,
                               HP203_ADDR,
                               &command,
                               1,
                               false,
                               HP203_TIMEOUT);
}

/* Reads bytes from the HP203 */
int HP203ReadBytes(hp203_t * sensor, uint8_t * buffer, size_t len) {
    return i2c_read_timeout_us(sensor->i2c,
                               HP203_ADDR,
                               buffer,
                               len,
                               false,
                               HP203_TIMEOUT);
}

/* Tests if the HP203 is functioning
 * Returns:
 * 0 if chip is functioning normally.
 * 1 if an i2c request times out.
 * 2 if the chip is either not connected, or somehow bad.
 * 3 if chip is not found.
 *
 * Function takes approximately 10 ms to run. */
uint8_t HP203Test(hp203_t * sensor) {
    uint8_t buffer[1];
    int result[3];              // Used to store the result of operations

    result[0] = sendCommandHP203(sensor, HP203_RESET);

    sleep_ms(10);               // Wait until the sensor is stable.

    result[1] = sendCommandHP203(sensor, HP203_READ_REG | HP203_INT_SRC);
    result[2] = readbytesHP203(sensor, buffer, 1);

    // Look over the results and figure out what to return.

    int i;

    for(i=0; i<3; i++) {
        if (result[i] == PICO_ERROR_GENERIC) {
            return 2;
        } else if (result[i] == PICO_ERROR_TIMEOUT) {
            return 1;
        }
    }

    if(buffer[0] & 0x20 == 0) { // Check the 6th bit.
        return 2;
    }

    return 0;
}

/* Tells the HP203 to start measuring data.
 * Returns the expected measurement duration in us. */
uint32_t HP203Measure(hp203_t * sensor) {
    static const uint32_t timeLookup[7] =
        {131100, 65600, 32800, 16400, 8200, 4100, 2100};

    uint8_t command = HP203_ADC_SET | sensor->channel | sensor->oversample;
    sendCommandHP203(sensor, command);

    return timeLookup[sensor->oversample >> 3 + sensor->channel >> 1];
}

/* Gets the pressure. Must be ran after a measurement has finished*/
uint32_t HP203GetPres(hp203_t * sensor) {
    uint8_t buffer[3];

    HP203SendCommand(sensor, HP203_READ_P);
    HP203ReadBytes(sensor, buffer, 3);

    return buffer[2] | buffer[1] << 8 | buffer[0] << 16;
}

/* Gets the Temperature. Must be ran after a measurement has finished*/
uint32_t HP203GetTemp(hp203_t * sensor) {
    uint8_t buffer[3];

    HP203SendCommand(sensor, HP203_READ_T);
    HP203ReadBytes(sensor, buffer, 3);

    return buffer[2] | buffer[1] << 8 | buffer[0] << 16;
}

/* Gets pressure and temperature in a single i2c read.
 * Returns a struct containing both. */
struct presTemp HP203GetPresTemp(hp203_t * sensor) {
    uint8_t buffer[6];
    struct presTemp output;

    HP203SendCommand(sensor, HP203_READ_PT);
    HP203ReadBytes(sensor, buffer, 6);

    output.pres = buffer[5] | buffer[4] << 8 | buffer[3] << 16;
    output.temp = buffer[2] | buffer[1] << 8 | buffer[0] << 16;

    return output;
}

/* Simple, and probably bad, driver for the HP203B chip from HopeRF.
 * A correctly-configured HP203 struct can be created using the
 * HP203Init function.
 *
 * To read values off the chip start a measurement using HP203Measure
 * then read the values off the chip using one of the HP203Get...
 * functions. Values are returned in pascals and centidegrees, because
 * floating point maths is the devil. */

#ifndef HP203_H
#define HP203_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define HP203_ADDR 	0x76
#define HP203_TIMEOUT	1000

// HP203 Commands
#define HP203_RESET	0x06
#define HP203_READ_PT	0x10
#define HP203_READ_AT	0x11
#define HP203_READ_P	0x30
#define HP203_READ_A	0x31
#define HP203_READ_T	0x32
#define HP203_ADC_SET	0x40
#define HP203_READ_REG	0x80
#define HP203_WRITE_REG 0xC0

// HP203 Registers
#define HP203_INT_SRC	0x0D

// HP203 ADC_CVT settings
#define HP203_OSR_SHIFT 2

// Enum containing the settings for the channel
enum HP203_CHN {
    HP203_PRES_TEMP = 0x00,
    HP203_TEMP_ONLY = 0x01
};

// Enum containing the various settings for the oversample rate
enum HP203_OSR {
    HP203_OSR_4096 = 0x00,
    HP203_OSR_2028 = 0x01,
    HP203_OSR_1024 = 0x02,
    HP203_OSR_512  = 0x03,
    HP203_OSR_256  = 0x04,
    HP203_OSR_128  = 0x05
};

typedef struct hp203_t {
    i2c_inst_t * i2c;
} hp203_t;

struct presTemp {
    uint32_t pres;
    uint32_t temp;
};

/* Simple init function for HP203. */
hp203_t HP203Init(i2c_inst_t * i2c);

/* Tests if the HP203 is functioning
 * Returns:
 * 0 if chip is functioning normally.
 * -1 if an i2c request times out.
 * -2 if the chip is either not connected, or somehow bad.
 * -3 if chip is not found.
 *
 * Function takes approximately 10 ms to run. */
int8_t HP203Test(hp203_t * sensor);

/* Tells the HP203 to start measuring data.
 * Returns:
 * The expected measurement time in us if successful
 * -1 if the I2C write times out
 * -2 for other errors */
int32_t HP203Measure(hp203_t * sensor, enum HP203_CHN channel, enum HP203_OSR OSR);

/* Gets the pressure. Must be ran after a measurement has finished
 * Returns:
 * 0 on success,
 * -1 if the I2C write times out
 * -2 for other errors */
int8_t HP203GetPres(hp203_t * sensor, uint32_t * result);

/* Gets the Temperature. Must be ran after a measurement has finished
 * Returns:
 * 0 on success,
 * -1 if the I2C write times out
 * -2 for other errors */
int8_t HP203GetTemp(hp203_t * sensor, uint32_t * result);

/* Gets pressure and temperature in a single i2c read.
 * Returns:
 * 0 on success
 * 1 on failure */
int8_t HP203GetPresTemp(hp203_t * sensor, struct presTemp * result);

#endif

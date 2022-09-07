#ifndef HP203_H
#define HP203_H

#include "hardware/i2c.h"

#define HP203_ADDR 	0x76
#define HP203_TIMEOUT	100

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
#define HP203_CHN_PT	0x00
#define HP203_CHN_T	0x02

#define HP203_OSR_4096	0x00
#define HP203_OSR_2048	0x08
#define HP203_OSR_1024	0x10
#define HP203_OSR_512	0x18
#define HP203_OSR_256	0x20
#define HP203_OSR_128	0x28

typedef struct hp203_t {
    i2c_inst_t * i2c;
    uint8_t channel;
    uint8_t oversample;
} hp203_t;

struct presTemp {
    uint32_t pres;
    uint32_t temp;
};

/* Simple init function for HP203. */
hp203_t HP203Init(i2c_inst_t * i2c);

/* Reads bytes from the HP203 */
int HP203ReadBytes(hp203_t * sensor, uint8_t * buffer, size_t len);

/* Tests if the HP203 is functioning
 * Returns:
 * 0 if chip is functioning normally.
 * 1 if an i2c request times out.
 * 2 if the chip is either not connected, or somehow bad.
 * 3 if chip is not found.
 *
 * Function takes approximately 10 ms to run. */
uint8_t HP203Test(hp203_t * sensor);

/* Tells the HP203 to start measuring data.
 * Returns the expected measurement duration in us. */
uint32_t HP203Measure(hp203_t * sensor);

/* Gets the pressure. Must be ran after a measurement has finished*/
uint32_t HP203GetPres(hp203_t * sensor);

/* Gets the Temperature. Must be ran after a measurement has finished*/
uint32_t HP203GetTemp(hp203_t * sensor);

/* Gets pressure and temperature in a single i2c read.
 * Returns a struct containing both. */
struct presTemp HP203GetPresTemp(hp203_t * sensor)
#endif

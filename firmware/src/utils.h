#ifndef UTILS
#define UTILS

#include "hardware/i2c.h"

/* Reads a number of bytes from an i2c device, starting at the register start
   Puts the bytes at dest. */
void i2cReadBytes(i2c_t *i2c, uint8_t addr, uint8_t start, uint8_t bytes, void *dest)

#endif

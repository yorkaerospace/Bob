#include "hardware/i2c.h"

/* Reads a number of bytes from an i2c device, starting at the register start
   Puts the bytes at dest. */
void i2cReadBytes(i2c_t *i2c, uint8_t addr, uint8_t start, uint8_t bytes, void *dest) {
	uint8_t buffer[bytes]; 
	i2c_write_blocking(i2c, address, &start, 1, true);
	i2c_read_blocking(i2c, address, buffer, bytes, false);
	memcpy(dest, buffer, bytes);		// There has to be a less cursed way to do this right?
}


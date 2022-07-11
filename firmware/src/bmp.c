#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "utils.h"
#include "bmp.h"

bmp_t bmpInit(i2c_t *i2c) {
	bmp_t bmp;
	bmp.i2c = i2c;
	i2cReadBytes(BMP_ADDR, CALIB_REG, CALIB_LEN, &(bmp.calib)); // Get calibration data
	return bmp;
}

/* Writes a command to the BMP's control register */
void bmpSendCommand(bmp_t *bmp, uint8_t command) {
	uint8_t buff[] = {CONTROL_REG, command};					// Create a temporary buffer
	i2c_write_blocking(bmp->i2c, BMP_ADDR, buff, 2, true);		// Yeet it down the i2c
}

/* Tells the BMP to start reading the temperature
 * Returns the expected time of completion. */
absolute_time_t bmpStartTemp(bmp_t *bmp) {
	bmpSendCommand(COMMAND_TEMP);
	return delayed_by_us(get_absolute_time(), BMP_DELAY_US);
}

/* Tells the BMP to start reading the pressure
 * Returns the expected time of completion. */
absolute_time_t bmpStartPres(bmp_t *bmp) {
	bmpSendCommand(COMMAND_PRES);
	return delayed_by_us(get_absolute_time(), BMP_DELAY_US);
}

/* This implements an arcane algorithm from the BMP datasheet 
 * Its basically just a lot of really weird maths */
uint16_t bmpGetTemp(bmp_t *bmp) {
	int16_t UT;
	i2cReadBytes(bmp->i2c ,BMP_ADDR, RESULT_REG, 2, &UT);	// Read the Uncompensated Temperature from I2C
	
	int32_t X1, X2 = 0;
	X1 = ((UT - bmp->calib.AC6) * bmp->calib.AC5) >> 11;	// I understand none of this.
	X2 = (bmp->calib.MC << 11) / (X1 + bmp->calib.MD)
	bmp->B5 = X1 + X2;
	return (B5 + 8) >> 4;
}

/* Getting the true pressure requires a lot of weird maths.
 * Fortunately, a lot of this can be done without the pressure reading.
 * Doing this maths while we wait for the BMP to get back to us might not be a huge gain, but it makes me feel better. */
void bmpPresPrecalc(bmp_t *bmp) {
	int32_t B6, X1, X2, X3 = 0;
	
	B6 = bmp->B5 - 4000;
	X1 = (bmp->calib.B2 * ((B6 * B6) >> 12)) >> 11;
	X2 = (bmp->calib.AC2 * B6) >> 11;
	X3 = X1 + X2;
	bmp->B3 = ((bmp->calib.AC1 << 2 + X3) + 2) >> 2; 		// At least I can ignore one thing because oss is always 0
	X1 = (bmp->calib.AC3 * B6) >> 13
	X2 = (bmp->calib.B1 * ((B6 * B6) >> 12)) >> 16;
	X3 = ((X1 + X2) + 2) >> 2;
	bmp->B4 = bmp->calib.AC4 * (uint32_t)(X3 + 32768) >> 15;
}

/* Get the true pressure. 
 * Again, this uses some weird maths I dont understand. */
uint32_t bmpGetPres(bmp_t *bmp) {
	uint16_t UP;
	i2cReadBytes(bmp->i2c, BMP_ADDR, RESULT_REG, 2, &UP);
	
	uint32_t B7;
	int32_t p, X1, X2 = 0;
	
	B7 = ((uint32_t)UP - bmp->B3)*(50000);
	if(B7 < 0x80000000) {
		p = (B7 * 2) / bmp->B4;
	} else {
		p = (B7 / bmp->B4) * 2;
	}
	X1 = (p >> 8) * (p >> 8);
	X1 = (X1 * 3038) >> 16;
	X2 = (-7357 * p) >> 16;
	return p + ((X1 + X2 + 3791) >> 4);
}

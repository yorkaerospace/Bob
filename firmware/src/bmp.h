#ifndef BMP
#define BMP

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "utils.h"
#include "bmp.h"

#define BMP_ADDR		0x77 // The BMP is weird. You send it a command and a few ms later, you read from a result register.
#define CONTROL_REG		0xF4 // That delay is long enough that I think I could interpolate the MPU reads and the BMP reads.
#define RESULT_REG		0xF6 // Actually that delay is going to be the limiting factor here.
#define COMMAND_TEMP	0x2E
#define COMMAND_PRES	0x34
#define CALIB_REG		0xAA
#define CALIB_LEN		0x16
#define BMP_DELAY_US	4500 // Fucking grandma over here takes 5 ms to return any data.

#pragma pack(2)
typedef struct {
    int16_t  AC1;		// The BMP chucks these at us and we're expected to know what to do with them.
    int16_t  AC2;		
    int16_t  AC3;
    uint16_t AC4;
    uint16_t AC5;		// No fucking clue what any of these are.
    uint16_t AC6;
    int16_t  B1;
    int16_t  B2;
    int16_t  MB;
    int16_t  MC;
    int16_t  MD;		// Also if you re-arrange these I will break your kneecaps.
} calib_t;

typedef struct {		// Stores data about the current state of the BMP
	i2c_t *i2c;
	
	calib_t calib;
	int32_t B5;
	uint32_t B4;
	int32_t B3;
}	bmp_t;

bmp_t bmpInit(i2c_t *i2c);

/* Writes a command to the BMP's control register */
void bmpSendCommand(bmp_t *bmp, uint8_t command);

/* Tells the BMP to start reading the temperature
 * Returns the expected time of completion. */
absolute_time_t bmpStartTemp(bmp_t *bmp);

/* Tells the BMP to start reading the pressure
 * Returns the expected time of completion. */
absolute_time_t bmpStartPres(bmp_t *bmp);

/* This implements an arcane algorithm from the BMP datasheet 
 * Its basically just a lot of really weird maths */
uint16_t bmpGetTemp(bmp_t *bmp);

/* Getting the true pressure requires a lot of weird maths.
 * Fortunately, a lot of this can be done without the pressure reading.
 * Doing this maths while we wait for the BMP to get back to us might not be a huge gain, but it makes me feel better. */
void bmpPresPrecalc(bmp_t *bmp);

/* Get the true pressure. 
 * Again, this uses some weird maths I dont understand. */
uint32_t bmpGetPres(bmp_t *bmp);

#endif

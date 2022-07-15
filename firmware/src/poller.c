#include <stdio.h>
#include <stdlib.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// MPU-6050 information
#define MPU_ADDR 		0x68 	// May vary between boards. Should probably autodetect or add something in pre-processing.
#define ACCEL_REG		0x3B
#define GYRO_REG		0x43
#define TEMP_REG		0x41

bmp_t bmp;
mpu_t mpu;
uint_8 t; 

void pollerInit(void) {
	
}

void pollerStart(void) {

}

/* Polls the I2C devices and puts the data packets into a queue. */
void pollerLoop(void) {
	while(true) {
		while(flags) {		 	// Iterate until all flags are cleared.
			
		}
		_wfi();					// Sleep until an interrupt is recived.
	}
}

#ifndef FLASH_H
#define FLASH_H

#include "types.h"

/* Implements a simple journaling system in flash
 * Uses a circular buffer to store data ahead of it being written.
 * This allows us to store a period of data from before launch, and write it
 * in flight. */

enum types {
    BARO = 'b', // baro_t
    IMU  = 'i', // imu_t
    COMP = 'c', // comp_t
    CFG  = 'C', // conf_t
    MSG  = 'm'  // ASCII Message <= 255 Chars
};

/* Reads the next log from the flash and puts it into buf */
int fRead(log_t * buf);

/* Creates a log from the data in buf and pushes it to the circular buffer
 * N.B. May overwrite data if the circular buffer is full. */
void fPush(uint8_t * buf, uint8_t size, enum types type);

/* Writes a log direct to flash, skipping the buffer.  */
void fWrite(uint8_t * buf, uint8_t size, enum types type);

/* Erases all data in the flash. Returns the expected time in ms. */
void fErase(void);

/* Spins up the flash task and associated timers. */
void fInit(void);

#endif

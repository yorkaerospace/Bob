#ifndef LOGGER_H
#define LOGGER_H

#include "sensors.h"

/* Clears data from the flash.
 * Has no error checking because apparently the SDK thinks we dont need that. */
void clearData(void);

/* Flushes the page buffer into the flash proper.
 * Returns:
 * The number of data structs written if successful
 * -1 if an error occured.
 *
 * Will also probably hardfault if an error occurs >:( */
uint8_t flushData(void);

/* Pushes data to the write buffer, then flushes it if it's full
 * Returns:
 *  1 if data was written to flash
 *  0 if data was buffered
 * -1 if an error occured during a write */
uint8_t writeData(data_t data);

/* Writes all the data in databuf to the flash
 * Returns the number of structs written. */
uint8_t writeAll(void);

/* Writes the logs to STDOUT as a nice CSV */
void dumpLogs(void);

#endif

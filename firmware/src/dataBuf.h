/* dataBuf.h
 *
 * Implements a simple circular buffer with a mutex for thread safety.
 *
 * 2022 Will H */

#ifndef DATABUF_H
#define DATABUF_H

#include "stdlib.h"
#include "sensors.h"

// A 10 second buffer should be ~30 kB
#define BUF_SIZE 10 * 100
#define BUF_TIMEOUT 1

/* Pushes a piece of data into the buffer.
 * Blocks for BUF_TIMEOUT. The buffer is allowed to overrun.
 * Returns:
 *  0 if successful
 * -1 if the mutex times out
 *  1 in the event of a buffer overrun. */
int8_t dataPush(data_t d);

/* Returns the amount of data in the buffer */
uint16_t dataSize(void);

/* Pops a piece of data from the end of the buffer
 * Blocks indefinitely.
 * Returns:
 *  0 if successful
 * -1 if no data is in the buffer */
int8_t dataPop(data_t * ptr);

/* Gets the data at the head of the buffer
 * Blocks indefinitely. */
void dataHead(data_t * ptr);

/* Gets the data that was offset samples ago */
void dataRel(data_t * ptr, size_t offset);
#endif

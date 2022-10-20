/* dataBuf.c
 *
 * Implements a simple circular buffer with a mutex for thread safety.
 *
 * 2022 Will H */

#include "sensors.h"

#include "pico/stdlib.h"
#include "pico/sync.h"

#include "dataBuf.h"


// Simple circular buffer.
static volatile data_t dataBuf[BUF_SIZE];
static volatile uint16_t tail = 0;
static volatile uint16_t head = 0;

auto_init_mutex(mtx);

/* Increments an index with looping */
static inline uint16_t incIndex(uint16_t i) {
    i++;
    if(i >= BUF_SIZE) {
        i = 0;
    }
    return i;
}

/* Pushes a piece of data into the buffer.
 * Blocks for BUF_TIMEOUT. Overwrites data if needed.
 * Returns:
 *  0 if successful
 * -1 if the mutex times out
 *  1 if data has been overwritten */
int8_t dataPush(data_t d){
    int8_t res;
    if(mutex_enter_timeout_ms(&mtx, BUF_TIMEOUT)) {
        head = incIndex(head);
        dataBuf[head] = d;
        // Increment tail if we've overwritten data
        if (head == tail) {
            tail = incIndex(tail);
            res = 1;
        } else {
            res = 0;
        }
        mutex_exit(&mtx);
        return res;
    } else {
        return -1;
    }
}

/* Returns the amount of data in the buffer */
uint16_t dataSize(void) {
    return head >= tail ?
           head - tail :
           BUF_SIZE - (tail - head);
}

/* Pops a piece of data from the end of the buffer
 * Blocks indefinitely.
 * Returns:
 *  0 if successful
 * -1 if no data is in the buffer */
int8_t dataPop(data_t * ptr) {
    mutex_enter_blocking(&mtx);
    if(head == tail + 1) {
        return -1;
    } else {
        *ptr = dataBuf[tail];
        tail = incIndex(tail);
        mutex_exit(&mtx);
        return 0;
    }
}

/* Gets the data at the head of the buffer
 * Blocks indefinitely. */
void dataHead(data_t * ptr) {
    mutex_enter_blocking(&mtx);
    *ptr = dataBuf[head];
    mutex_exit(&mtx);
}

/* Gets the data that was offset samples ago */
void dataRel(data_t * ptr, size_t offset) {
    mutex_enter_blocking(&mtx);
    *ptr = dataBuf[(head - offset) % BUF_SIZE];
    mutex_exit(&mtx);
}

#include "hardware/flash.h"
#include "hardware/timer.h"
#include <hardware/sync.h>
#include "flash.h"
#include "taskList.h"

#include <pico/stdlib.h>
#include <string.h>

// About 2 seconds worth of data, also about half our RAM.
#define CIRC_BUF  512
#define PROG_RESERVED (1024 * 1024)
#define START_PTR XIP_BASE + PROG_RESERVED

// Add on the header to find the actual size of a log.
#define TRUE_SIZE(a) sizeof(log_t) - UINT8_MAX + a

// Increment with loop around
// This would normaly be a modulo, but ARM doesnt have that.
#define LOOPINC(i, max) i + 1 >= max ? 0 : i + 1;

// Externs
extern taskList_t tl;
extern enum states state;

static log_t buf[CIRC_BUF];
static uint16_t head = 0;
static uint16_t tail = 1;
static void * readPtr = (void * ) START_PTR;

/* /\* ----------------------- TASKS ------------------------ *\/ */

/* static void fTask(void * data) { */
/*     asm volatile("nop \n nop \n nop"); */
/* } */

/* /\* ----------------------- IRQs ------------------------  *\/ */

/* static bool flashIRQ(repeating_timer_t *rt) { */
/*     tlAdd(&tl, fTask, NULL); */
/*     return true; */
/* } */

/* /\* ------------------ PUBLIC FUNCTIONS ------------------ *\/ */

/* /\* Reads the next log from the flash and puts it into buf *\/ */
/* int fRead(log_t * buf) { */
/*     log_t * ptr = (log_t * ) readPtr; */
/*     if(ptr->marker == 0xAA) { */
/*         memcpy(buf, readPtr, sizeof(log_t)); */
/*         readPtr += TRUE_SIZE(ptr->size); */
/*         return 0; */
/*     } else { */
/*         return 1; */
/*     } */
/* } */

/* /\* Creates a log from the data in buf and pushes it to the circular buffer */
/*  * N.B. May overwrite data if the circular buffer is full. *\/ */
/* void fPush(uint8_t * buf, uint8_t size, enum types type) { */
/*     log_t l; */

/*     // Set e to 0xFF to avoid writing nonsense. */
/*     memset(&l, 0xFF, sizeof(log_t)); */

/*     // Package up the rest of the entry */
/*     l.marker = 0xAA; */
/*     l.size = size; */
/*     l.type = type; */
/*     memcpy(l.data, buf, size); */

/*     buf[head] = l; */

/*     // Increment read pointer */
/*     head = LOOPINC(head, CIRC_BUF); */
/*     // Increment write ptr if needed */
/*     if (head == tail) { */
/*         tail = LOOPINC(tail, CIRC_BUF); */
/*     } */
/* } */

/* /\* Writes a log direct to flash, skipping the buffer.  *\/ */
/* void fWrite(uint8_t * buf, uint8_t size, enum types type); */

/* /\* Erases all data in the flash. Returns the expected time in ms. *\/ */
/* void fErase(void); */

/* /\* Spins up the flash task and associated timers. *\/ */
/* void fInit(void); */

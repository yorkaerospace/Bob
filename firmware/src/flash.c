#include "hardware/flash.h"
#include "hardware/timer.h"
#include <hardware/sync.h>
#include "flash.h"
#include "taskList.h"

#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>

// About 2 seconds worth of data, also about half our RAM.
#define CIRC_BUF  512
#define PROG_RESERVED (1024 * 1024)
#define FLASH_SIZE 8 * 1024 * 1024
#define START_PTR XIP_BASE + PROG_RESERVED

// Add on the header to find the actual size of a log.
#define TRUE_SIZE(a) a + 3

// Increment with loop around
// This would normaly be a modulo, but ARM doesnt have that.
#define LOOPINC(i, max) (i + 1) % max;

// Externs
extern taskList_t tl;
extern enum states state;

static log_t buf[CIRC_BUF];
static uint16_t head     = 0;
static uint16_t tail     = 0;

/* The XIP flash is memory-mapped, meaning we can read from it using
 * pointer wizardry. However to write we have to use flash addresses. */
static void * readPtr    = (void * ) START_PTR;
static uint32_t writeAdr = PROG_RESERVED;

static repeating_timer_t flashTimer;

/* ---------------------- HELPERS ----------------------- */

/* Attempts to pop a record from the input buffer */
static int fPop(log_t * log) {
    if (head != tail) {
        memcpy(log, &buf[tail], sizeof(log_t));
        tail = (tail + 1) % CIRC_BUF;
        return 0;
    } else {
        return 1;
    }
}

static int fPeek(log_t * log) {
    if (head != tail) {
        memcpy(log, &buf[tail], sizeof(log_t));
        return 0;
    } else {
        return 1;
    }
}

/* ----------------------- TASKS ------------------------ */

/* Checks if there is data, and if we're in an appropriate state to write it,
 * then buffers up about two pages worth, and writes it. */
static void fTask(void * data) {
    uint32_t mask         = ~0xFF;
    uint16_t writeIndex   = writeAdr & 0xFF;
    uint16_t bytesWritten = 0;
    uint8_t writeBuf[512];
    log_t l;

    memset(writeBuf, 0xFF, 512);

    if(state != GROUNDED && state != BOOT           // Are we in the right state?
    && !fPeek(&l) && writeAdr + 512 < FLASH_SIZE) { // Is there data and room?
        // Pack the write buffer until we either have no more data or no room.
        while (!fPeek(&l) && TRUE_SIZE(l.size) + writeIndex <= 512 ) {
            fPop(&l); // Kinda redundant. Its 2am.
            memcpy(writeBuf + writeIndex, &l, TRUE_SIZE(l.size));
            writeIndex += TRUE_SIZE(l.size);
            bytesWritten += TRUE_SIZE(l.size);
        }

        int ints = save_and_disable_interrupts();
        flash_range_program (writeAdr & mask, writeBuf, 512);
        restore_interrupts(ints);
        writeAdr += bytesWritten;
    }

}

/* ----------------------- IRQs ------------------------  */

static bool flashIRQ(repeating_timer_t *rt) {
    tlAdd(&tl, fTask, NULL);
    return true;
}

/* ------------------ PUBLIC FUNCTIONS ------------------ */

/* Reads the next log from the flash and puts it into buf */
int fRead(log_t * buf) {
    log_t * ptr = (log_t * ) readPtr;
    if(ptr->marker == 0xAA) {
        memcpy(buf, ptr, sizeof(log_t));
        readPtr += TRUE_SIZE(ptr->size);
        return 0;
    } else {
        return 1;
    }
}

/* Creates a log from data and pushes it to the circular buffer
 * N.B. May overwrite data if the circular buffer is full. */
void fPush(uint8_t * data, uint8_t size, enum types type) {
    log_t l;

    // Set e to 0xFF to avoid writing nonsense.
    memset(&l, 0xFF, sizeof(log_t));

    // Package up the rest of the entry
    l.marker = 0xAA;
    l.size = size;
    l.type = type;
    memcpy(&l.data, data, size);

    buf[head] = l;

    // Increment read pointer
    head = LOOPINC(head, CIRC_BUF);

    // Increment write ptr if needed
    if (head == tail) {
        tail = LOOPINC(tail, CIRC_BUF);
    }
}

/* Erases all data in the flash.
 * Returns nothing. If the erase fails, the 2040 will hardfault >:( */
void fErase(void) {
    int ints = save_and_disable_interrupts();
    flash_range_erase (PROG_RESERVED, FLASH_SIZE - PROG_RESERVED);
    restore_interrupts(ints);

    readPtr  = (void * ) START_PTR;
    writeAdr = PROG_RESERVED;
}

/* Puts the read pointer back at the start */
void fRewind(void) {
    readPtr  = (void * ) START_PTR;
}

/* Returns total flash used in kiB */
int fUsed(void) {
    return (writeAdr - PROG_RESERVED) >> 10;
}

/* Spins up the flash task and associated timers. */
void fInit(void) {
    log_t l;

    // Figure out whats already in flash
    while(!fRead(&l)) {
        asm volatile("nop");
    }

    writeAdr = ((int) readPtr) - XIP_BASE;
    readPtr = (void *) START_PTR;

    add_repeating_timer_ms(100, flashIRQ, NULL, &flashTimer);
}

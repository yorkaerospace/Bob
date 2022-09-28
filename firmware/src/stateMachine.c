#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "pico/multicore.h"

#include "stateMachine.h"
#include "sensors.h"
#include "cmd.h"

// A 10 second buffer should be ~30 kB
#define BUF_SIZE 10 * 100
// Pressure threshold for a launch
#define PRES_L -100  // About 10m
// Acceleration threshold for a launch
#define ACCL_L (gToAccel(10))
// Pressure threshold to detect apogee
#define PRES_A 100  // About 10m
// Pressure threshold for landing
#define PRES_G 100

static volatile uint8_t state = GROUNDED;

// Simple circular buffer.
static volatile data_t dataBuf[BUF_SIZE];
static volatile uint16_t tail = 0;
static volatile uint16_t head = 0;

/* Finds the difference in pressure between the most recent
 * data packet and the one that was time ago. */
static int32_t deltaPres(size_t time) {
    uint32_t currPres = dataBuf[head].pres;
    uint32_t pastPres = dataBuf[(head - time) % BUF_SIZE].pres;

    return currPres - pastPres;
}

/* Copies the newest packet from the data buffer
 * Should make writing terminal stuff a bit safer as we dont
 * need to play with that data buffer. */
data_t getLatest(void) {
    return dataBuf[head];
}

void __not_in_flash_func(fifoIRQ)(void) {
    data_t * data_p;
    int16_t accel_mag;

    // Copy everything out of the FIFO and into the buffer
    while(multicore_fifo_rvalid()) {
        data_p = multicore_fifo_pop_blocking();
        head++;
        if(head >= BUF_SIZE) {
            head = 0;
        }
        dataBuf[head] = *data_p;
        free(data_p);
    }

    // Figure out if state changes are required.
    switch (state) {
    case GROUNDED:
        if (deltaPres(100) < PRES_L ||
            magnitude(dataBuf[head].accel) > ACCL_L) {
            state = ASCENDING;
            dataBuf[head].status |= LAUNCH;
        }
        break;
    case ASCENDING:
        if (deltaPres(100) > PRES_A) {
            state = DESENDING;
            dataBuf[head].status |= APOGEE;
        }
        break;
    case DESENDING:
        if (abs(deltaPres(BUF_SIZE)) < PRES_G) {
            state = GROUNDED;
            dataBuf[head].status |= LANDING;
        }
    }

}

void stateLoop(void) {
    while (true) {
        switch (state) {
        case CONNECTED:
            if (!stdio_usb_connected()) {
                state = GROUNDED;
            } else {
                pollUsb();
            }
            break;
        case GROUNDED:
            if (stdio_usb_connected()) {
                state = CONNECTED;
            }
            break;
        case ASCENDING:
            break;
        case DESENDING:
            break;
        }
    }
}

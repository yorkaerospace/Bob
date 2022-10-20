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

// Pressure threshold for a launch
#define PRES_L -100  // About 10m
// Acceleration threshold for a launch
#define ACCL_L (gToAccel(10))
// Pressure threshold to detect apogee
#define PRES_A 100  // About 10m
// Pressure threshold for landing
#define PRES_G 100

static volatile uint8_t state = GROUNDED;

/* Updates the state based on the data passed to it */
void stateDetect(data_t cur) {
    int16_t accel_mag;
    // Figure out if state changes are required.
    switch (state) {
    case GROUNDED:
        if (deltaPres(100) < PRES_L ||
            magnitude(cur.accel) > ACCL_L) {
            state = ASCENDING;
            cur.status |= LAUNCH;
        }
        break;
    case ASCENDING:
        if (deltaPres(100) > PRES_A) {
            state = DESENDING;
            cur.status |= APOGEE;
        }
        break;
    case DESENDING:
        if (abs(deltaPres(1000)) < PRES_G) {
            state = GROUNDED;
            cur.status |= LANDING;
        }
    }

}

/* Sets the state to a certian value */
void stateSet(enum states newState) {
    state = newState;
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

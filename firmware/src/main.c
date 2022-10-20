#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "pico/multicore.h"

#include "sensors.h"
#include "cmd.h"
#include "dataBuf.h"

// Pressure threshold for a launch
#define PRES_L -100  // About 10m
// Acceleration threshold for a launch
#define ACCL_L (gToAccel(10))
// Pressure threshold to detect apogee
#define PRES_A 100  // About 10m
// Pressure threshold for landing
#define PRES_G 100

enum states {
    CONNECTED, // If system is connected over usb
    GROUNDED,  // If system is not connected, but not in flight
    ASCENDING,
    DESENDING
};

volatile uint8_t state = GROUNDED;

/* Takes a data packet and decides if state changes are needed */
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

/* The code that runs on core 1 */
void core1Entry(void) {
    data_t d;
    uint8_t status;
    absolute_time_t nextPoll;

    configureSensors();
    status = testSensors();

    while (true) {
        nextPoll = make_timeout_time_ms(10);
        d = pollSensors(status);
        dataPush(d);
        stateDetect(d);
        sleep_until(nextPoll);
        status = d.status;
    }
}

int main() {

    stdio_init_all();

    multicore_launch_core1(core1Entry);

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

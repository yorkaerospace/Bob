#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "pico/multicore.h"

#include "sensors.h"
#include "cmd.h"
#include "dataBuf.h"
#include "states.h"

volatile uint8_t state = GROUNDED;

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

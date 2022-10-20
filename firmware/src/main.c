#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "pico/multicore.h"

#include "stateMachine.h"
#include "sensors.h"
#include "dataBuf.h"

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
        sleep_until(nextPoll);
        status = d.status;
    }
}

int main() {
    uint8_t status;

    stdio_init_all();

    multicore_launch_core1(core1Entry);

    stateLoop();
}

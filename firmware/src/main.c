#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "pico/multicore.h"

#include "stateMachine.h"
#include "sensors.h"

void core1Entry(void) {
    data_t * data_p;
    uint8_t status;
    absolute_time_t nextPoll;

    configureSensors();
    status = testSensors();

    while (true) {
        nextPoll = make_timeout_time_ms(10);
        data_p = malloc(sizeof(data_t));
        *data_p = pollSensors(status);
        sleep_until(nextPoll);
        multicore_fifo_push_timeout_us(data_p, 500);
        status = data_p->status;
    }
}

int main() {
    uint8_t status;

    stdio_init_all();

    multicore_fifo_clear_irq();
    irq_set_exclusive_handler(SIO_IRQ_PROC0, fifoIRQ);

    irq_set_enabled(SIO_IRQ_PROC0, true);

    multicore_launch_core1(core1Entry);

    stateLoop();
}

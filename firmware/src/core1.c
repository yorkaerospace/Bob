#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/irq.h"

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

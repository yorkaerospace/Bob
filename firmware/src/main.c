#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "pico/multicore.h"

#include "stateMachine.h"
#include "sensors.h"
#include "core1.h"

int main() {
    uint8_t status;

    stdio_init_all();

    multicore_fifo_clear_irq();
    irq_set_exclusive_handler(SIO_IRQ_PROC0, fifoIRQ);

    irq_set_enabled(SIO_IRQ_PROC0, true);

    multicore_launch_core1(core1Entry);

    stateLoop();
}

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "pico/multicore.h"

#include "usbcmd.h"
#include "sensors.h"
#include "core1.h"

volatile data_t data;

void fifoIRQ() {
    data_t * data_p;
    data_p = multicore_fifo_pop_blocking();
    data = *data_p;
    free(data_p);
}

int main() {
    uint8_t status;

    stdio_init_all();
    usbCmdInit();

    multicore_fifo_clear_irq();
    irq_set_exclusive_handler(SIO_IRQ_PROC0, fifoIRQ);

    irq_set_enabled(SIO_IRQ_PROC0, true);

    multicore_launch_core1(core1Entry);

    while (true) {
        status = data.status;
        printf("Status: %d\n"
               "IMU:\n"
               "AX: %d, AY: %d, AZ: %d\n"
               "GX: %d, GY: %d, GZ: %d\n"
               "BARO:\n"
               "Pressure: %d, Temp: %d\n"
               "COMP:\n"
               "X: %d, Y: %d, Z: %d\n",
               status,
               data.accel[0], data.accel[1], data.accel[2],
               data.gyro[0], data.gyro[1], data.gyro[2],
               data.pres, data.temp,
               data.mag[0], data.mag[1], data.mag[2]);
        sleep_ms(1000);
    }


}

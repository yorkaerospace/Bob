#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "usbcmd.h"
#include "sensors.h"



int main() {
    uint8_t status;
    data_t data;

    stdio_init_all();
    usbCmdInit();
    configureSensors();

    sleep_ms(10000);
    status = testSensors();
    printf("Status: %d\n");
    while (true) {
        data = pollSensors(status);
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

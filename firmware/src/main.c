#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include <hp203b.h>
#include <qmc5883l.h>
#include <qmi8658c.h>
#include "usbcmd.h"

void loopPrint(char * str) {
    while(true) {
        printf(str);
        sleep_ms(2000);
    }
}

int main() {
    hp203_t hp203;
    qmc_t qmc;
    qmi_t qmi;
    struct qmc_cfg qmcCfg;
    struct presTemp pt;
    struct magData mag;
    struct qmi_data imu;
    int16_t magTemp;

    qmcCfg.mode = QMC_CONTINUOUS;
    qmcCfg.ODR = QMC_ODR_10HZ;
    qmcCfg.OSR = QMC_OSR_256;
    qmcCfg.scale = QMC_SCALE_2G;
    qmcCfg.pointerRoll = true;
    qmcCfg.enableInterrupt = false;

    stdio_init_all();
    i2c_init(i2c_default, 100*1000);
    usbCmdInit();

    gpio_set_function(16, GPIO_FUNC_I2C);
    gpio_set_function(17, GPIO_FUNC_I2C);
    gpio_pull_up(16);
    gpio_pull_up(17);
    hp203 = HP203Init(i2c_default);
    qmc = QMCInit(i2c_default);
    qmi = QMIInit(i2c_default, true);


    printf("Starting config\n");
    printf("Configuring IMU\n");
    QMIGyroConfig(&qmi, QMI_GYRO_125HZ, QMI_GYRO_256DPS);
    QMISetOption(&qmi, QMI_GYRO_ENABLE, true);
    QMISetOption(&qmi, QMI_GYRO_SNOOZE, false);

    QMIAccConfig(&qmi, QMI_ACC_125HZ, QMI_ACC_16G);
    QMISetOption(&qmi, QMI_ACC_ENABLE, true);
    printf("Configuring Compass\n");
    QMCSetCfg(&qmc, qmcCfg);

    printf("Testing Barometer\n");
    switch(HP203Test(&hp203)) {
    case 0:
        printf("good chip\n");
        sleep_ms(10);
        while(true) {
            sleep_us(HP203Measure(&hp203, HP203_PRES_TEMP, HP203_TEMP_ONLY));
            HP203GetPresTemp(&hp203, &pt);
            QMCGetMag(&qmc, &mag);
            QMCGetTemp(&qmc, &magTemp);
            QMIReadData(&qmi, &imu);
            printf("Pressure: %u, Temp %u \n", pt.pres, pt.temp);
            sleep_ms(10);
            printf("X: %d, Y: %d, Z: %d, Temp: %d,\n", mag.x, mag.y, mag.z, magTemp);
            sleep_ms(10);
            printf("AX: %d, AY: %d, AZ: %d, GX: %d, GY: %d, GZ: %d CTRL7: %x\n",
                   imu.accel[0], imu.accel[1], imu.accel[2], imu.gyro[0], imu.gyro[1], imu.gyro[2], CTRL7);
            sleep_ms(1000);
        }
        break;
    case -1:
        loopPrint("I2C timeout\n");
        break;
    case -2:
        loopPrint("Bad chip\n");
        break;
    case -3:
        loopPrint("Chip not found\n");
        break;
    }
}

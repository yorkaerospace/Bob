#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "hp203b.h"
#include "qmc5883l.h"
#include "qmi8658c.h"
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
    //usbCmdInit();

    gpio_set_function(16, GPIO_FUNC_I2C);
    gpio_set_function(17, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    hp203 = HP203Init(i2c_default);
    qmc = QMCInit(i2c_default);
    qmi = QMIInit(i2c_default, true);

    QMIAccConfig(&qmi, QMI_ACC_125HZ, QMI_ACC_16G);
    QMIGyroConfig(&qmi, QMI_GYRO_125HZ, QMI_GYRO_256DPS);
    QMISetOption(&qmi, QMI_ACC_ENABLE, true);
    QMISetOption(&qmi, QMI_GYRO_ENABLE, true);
    QMCSetCfg(&qmc, qmcCfg);

    switch(0) {
    case 0:
        while(true) {
            //sleep_us(HP203Measure(&hp203, HP203_PRES_TEMP, HP203_TEMP_ONLY));
            //HP203GetPresTemp(&hp203, &pt);
            QMCGetMag(&qmc, &mag);
            QMCGetTemp(&qmc, &magTemp);
            QMIReadData(&qmi, &imu);
            printf("Pressure: %u, Temp %u \n", pt.pres, pt.temp);
            printf("X: %d, Y: %d, Z: %d, Temp: %d,\n", mag.x, mag.y, mag.z, magTemp);
            printf("AX: %d, AY: %d, AZ: %d, GX: %d, GY: %d, GZ: %d\n",
                   imu.accel[0], imu.accel[1], imu.accel[2], imu.gyro[0], imu.gyro[1], imu.gyro[2]);
            sleep_ms(1000);
        }
        break;
    case 1:
        loopPrint("I2C timeout\n");
        break;
    case 2:
        loopPrint("Bad chip\n");
        break;
    case 3:
        loopPrint("Chip not found\n");
        break;
    }
}

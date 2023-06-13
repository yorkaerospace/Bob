#include "sampler.h"
#include "hp203b.h"
#include "qmc5883l.h"
#include "qmi8658c.h"
#include "ansi.h"
#include "taskList.h"

#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <hardware/sync.h>
#include <hardware/timer.h>
#include <string.h>

#define PROG_RESERVED (1024 * 1024)

#define GYRO_RANGE QMI_GYRO_256DPS
#define ACCL_RANGE QMI_ACC_16G

// Sensor structs
static hp203_t hp203;
static qmc_t qmc;
static qmi_t qmi;

static repeating_timer_t qmcTimer;
static repeating_timer_t qmiTimer;
static repeating_timer_t hpStartTimer;
static repeating_timer_t hpEndTimer;

extern taskList_t tl;

/* ------------------------- IRQs ------------------------- */

static bool addRepeat(repeating_timer_t *rt) {
    tlAdd(&tl, rt->user_data, NULL);
    return true;
}

static bool addOnce(repeating_timer_t *rt) {
    tlAdd(&tl, rt->user_data, NULL);
    return false;
}

/* ------------------------- TASKS ------------------------ */

/* Gets data from the HP203. */
static void hpEndTask(void * data) {
    struct hp203_data baro;

    HP203GetData(&hp203, &baro);

    printf("%7u Pa, %6d C \n", baro.pres, baro.temp);
}

/* Tells the HP203 to start a reading, then sets a timer for
 * when its done */
static void hpStartTask(void * data) {
    // Its easier to just use a repeating timer once than an alarm pool.
    int32_t i2cStatus;

    i2cStatus = HP203Measure(&hp203, HP203_PRES_TEMP, HP203_OSR_1024);
    if(i2cStatus > HP203_OK) {
        add_repeating_timer_us(i2cStatus, addOnce, hpEndTask, &hpEndTimer);
    }
}

/* Gets IMU data */
static void qmiTask(void * data) {
    struct qmi_data imu;

}

/* ------------------------ CONFIG ------------------------ */

/* Initialises the sensors and the associated i2c bus,
 * Kicks off the timers used to.. time things */
void configureSensors(void)
{

    struct qmc_cfg qmcCfg;

    // Configure the i2c bus.
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(16, GPIO_FUNC_I2C);
    gpio_set_function(17, GPIO_FUNC_I2C);
    gpio_pull_up(16);
    gpio_pull_up(17);

    // Initialise sensor structs
    hp203 = HP203Init(i2c_default);
    qmc = QMCInit(i2c_default);
    qmi = QMIInit(i2c_default, true);

    // Configure the QMI's gyro
    QMIGyroConfig(&qmi, QMI_GYRO_125HZ, GYRO_RANGE);
    QMISetOption(&qmi, QMI_GYRO_ENABLE, true);
    QMISetOption(&qmi, QMI_GYRO_SNOOZE, false);

    // Configure the QMI's accelerometer
    QMIAccConfig(&qmi, QMI_ACC_125HZ, ACCL_RANGE);
    QMISetOption(&qmi, QMI_ACC_ENABLE, true);

    // Configure the QMC
    qmcCfg.mode = QMC_CONTINUOUS;
    qmcCfg.ODR = QMC_ODR_100HZ;
    qmcCfg.OSR = QMC_OSR_256;
    qmcCfg.scale = QMC_SCALE_2G;
    qmcCfg.pointerRoll = true;
    qmcCfg.enableInterrupt = false;

    QMCSetCfg(&qmc, qmcCfg);

    add_repeating_timer_ms(100, addRepeat, hpStartTask, &hpStartTimer);

}

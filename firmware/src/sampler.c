#include "sampler.h"
#include "hp203b.h"
#include "qmc5883l.h"
#include "qmi8658c.h"
#include "ansi.h"
#include "taskList.h"
#include "types.h"

#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <hardware/sync.h>
#include <hardware/timer.h>
#include <string.h>

// Helper functions
#define NOW_MS to_ms_since_boot(get_absolute_time())

// Flash config
#define PROG_RESERVED (1024 * 1024)

// Sensor config
#define GYRO_RANGE QMI_GYRO_256DPS
#define ACCL_RANGE QMI_ACC_16G

// State config
#define BOOT_TIME_MS 1000 // How long do we wait for the filters to filter

// Sensor structs
static hp203_t hp203;
static qmc_t qmc;
static qmi_t qmi;

static repeating_timer_t qmcTimer;
static repeating_timer_t qmiTimer;
static repeating_timer_t hpStartTimer;
static repeating_timer_t hpEndTimer;

// Externs
extern taskList_t tl;
extern enum states state;

// Latest data packets from the sensors.
// Made global for other tasks to have access to them
// Since context switching isn't a thing, we can just do this!
baro_t baroData;
imu_t  imuData;
comp_t compData;

/* ------------------------- IRQs ------------------------- */

static bool addRepeat(repeating_timer_t *rt) {
    tlAdd(&tl, rt->user_data, NULL);
    return true;
}

static bool addOnce(repeating_timer_t *rt) {
    tlAdd(&tl, rt->user_data, NULL);
    return false;
}

/* ------------------- DATA PROCESSING -------------------- */

/* Filters the barometer data */
static baro_t baroProcessor(struct hp203_data raw) {
    static uint32_t dn1, dn2, bn1, bn2;
    baro_t out = {0};

    // Apply andy's filter
    out.vVel = (5*raw.pres - 5*bn2 + 4*dn1 - 2*dn2) >> 4;
    dn1 = out.vVel; dn2 = dn1; bn1 = raw.pres; bn2 = bn1;

    out.pres = raw.pres;
    out.temp = raw.temp;
    out.time = NOW_MS;

    return out;
}

/* Calculates the magnitude of the accelerometer data  */
static imu_t imuProcessor(struct qmi_data raw) {
    imu_t out = {0};

    out.time = NOW_MS;
    memcpy(out.accl, raw.accel, 6);
    memcpy(out.gyro, raw.gyro, 6);

    out.accl_mag = out.accl[0] * out.accl[0]
                 + out.accl[1] * out.accl[1]
                 + out.accl[2] * out.accl[2];

    return out;
}

/* This... doesnt do much. Eh. */
static comp_t compProcessor(int16_t * raw) {
    comp_t out = {0};

    memcpy(out.compass, raw, 6);
    out.time = NOW_MS;

    return out;
}

/* ------------------------- TASKS ------------------------ */

/* Gets data from the HP203. */
static void hpEndTask(void * data) {
    struct hp203_data hpRaw;
    int32_t i2cStatus;

    i2cStatus = HP203GetData(&hp203, &hpRaw);
    baroData = baroProcessor(hpRaw);

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
    int32_t i2cStatus;

    i2cStatus = QMIReadData(&qmi, &imu);
    imuData = imuProcessor(imu);
}

/* Gets IMU data */
static void qmcTask(void * data) {
    int16_t mag[3];
    int32_t i2cStatus;

    i2cStatus = QMCGetMag(&qmc, mag);
    compData = compProcessor(mag);
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

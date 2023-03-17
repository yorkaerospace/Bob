#include "sampler.h"
#include "hp203b.h"
#include "qmc5883l.h"
#include "qmi8658c.h"
#include "ansi.h"

#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <hardware/flash.h>
#include <string.h>

#define PROG_RESERVED (1024 * 1024)
#define FLASH_SIZE (8 * 1024 * 1024)

// Sensor structs
static hp203_t hp203;
static qmc_t qmc;
static qmi_t qmi;

static sample_t * writePtr = (sample_t *)(XIP_BASE + PROG_RESERVED);

/* Takes a sample and a message and prints it to the console in
 * a nice pretty format */
void prettyPrint(sample_t s, char * msg) {
    static const char prompt[] =
        MOV(1,1) NORM
        "Bob Rev 3 running build: %s %s" CLRLN
        "Timestamp: %ld" CLRLN
        CLRLN
        "Accelerometer: X: %6d     Y: %6d     Z: %6d" "\n"
        NORM
        "Gyroscope:     X: %6d     Y: %6d     Z: %6d" "\n"
        NORM
        "Compass:       X: %6d     Y: %6d     Z: %6d" "\n"
        NORM // Alacritty *really* likes to bold stuff.
        "Barometer:     Pressure: %7ld Pa     Temp: %6ld" "\n"
        NORM
        "Flash:         Used: %6ld kiB"
        CLRLN NORM
        "%s.\x1b[0J\n";


    uint32_t bytesUsed = (int) writePtr - (XIP_BASE + PROG_RESERVED);
    uint32_t kiBUsed = bytesUsed >> 10;
//  float temp = (float)s.temp / 100;

    printf(prompt, __TIME__, __DATE__, s.time,
           s.accel[0], s.accel[1], s.accel[2],
           s.gyro[0], s.gyro[1], s.gyro[2],
           s.mag[0], s.mag[1], s.mag[2],
           s.pres, s.temp, kiBUsed, msg);

}


/* Initialises the sensors and the associated i2c bus */
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
    QMIGyroConfig(&qmi, QMI_GYRO_125HZ, QMI_GYRO_256DPS);
    QMISetOption(&qmi, QMI_GYRO_ENABLE, true);
    QMISetOption(&qmi, QMI_GYRO_SNOOZE, false);

    // Configure the QMI's accelerometer
    QMIAccConfig(&qmi, QMI_ACC_125HZ, QMI_ACC_16G);
    QMISetOption(&qmi, QMI_ACC_ENABLE, true);

    // Configure the QMC
    qmcCfg.mode = QMC_CONTINUOUS;
    qmcCfg.ODR = QMC_ODR_100HZ;
    qmcCfg.OSR = QMC_OSR_256;
    qmcCfg.scale = QMC_SCALE_2G;
    qmcCfg.pointerRoll = true;
    qmcCfg.enableInterrupt = false;

    QMCSetCfg(&qmc, qmcCfg);

}

/* Gets a sample from the sensors.
 * No longer attempts to determine if sensors are functional.
 * If they don't respond, they dont respond. */
sample_t getSample(void) {
    sample_t sample;
    int32_t i2cStatus;
    absolute_time_t hp203Ready;

    struct hp203_data barometer;
    struct qmi_data imu;

    sample.time = to_ms_since_boot(get_absolute_time());

    // The HP203 is slow, so when we ask it for a sample, it responds with the
    // amount of time it'll take. We'll do other stuff while we wait.
    i2cStatus = HP203Measure(&hp203, HP203_PRES_TEMP, HP203_OSR_256);
    if(i2cStatus > HP203_OK) {
        hp203Ready = make_timeout_time_us(i2cStatus);
    } else {
        // absolute_time_t is a 64 bit int.
        // The battery will die before it equals 0 again.
        hp203Ready = 0;
    }

    // Get data from the IMU
    i2cStatus = QMIReadData(&qmi, &imu);
    if(i2cStatus == QMI_OK) {
        memcpy(sample.accel, imu.accel, 6);
        memcpy(sample.gyro, imu.gyro, 6);
    }

    // Get data from the compass
    i2cStatus = QMCGetMag(&qmc, sample.mag);

    // Then check the HP203
    if(hp203Ready != 0) {
        sleep_until(hp203Ready);
        i2cStatus = HP203GetData(&hp203, &barometer);
        sample.pres = barometer.pres;
        sample.temp = barometer.temp;
    }

    // Zero this for now. Used to check if where we've written in flash.
    sample.status = 0;

    return sample;
}

/* Writes a sample to flash.
 * No longer faffs around with buffering. Just yheets it onto flash. */
void logSample(sample_t sample) {
    uint8_t buf[512]; // 2 page buffer

    // Flash is initialised as all 1s, and then can selectively set to 0
    // Setting our write buffer to all 1s lets us write areas smaller than a page
    memset(buf, 0xFF, 512);

    // Skip past written structs:
    while (writePtr->status != 0xFF && (int) writePtr - XIP_BASE < FLASH_SIZE - 512) {
        writePtr++;
    }

    uint32_t addrInPage = ((int) writePtr) % 256;
    memcpy(buf + addrInPage, &sample, sizeof(sample_t));

    uint32_t ints = save_and_disable_interrupts();
    uint32_t addrInFlash = ((int) writePtr) - XIP_BASE - addrInPage;
    flash_range_program(addrInFlash, buf, 512);
    restore_interrupts(ints);
}

/* Reads a sample from flash.
 * Returns 0 on success, or 1 on failure. */
uint8_t readSample (size_t index, sample_t * sample) {
    sample_t * ptr = (sample_t *)(XIP_BASE + PROG_RESERVED) + index;
    if(ptr->status == 0xFF) {
        return 1;
    } else {
        memcpy(sample, ptr, sizeof(sample_t));
        return 0;
    }
}

void clearFlash(void) {
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(PROG_RESERVED, 7*1024*1024);
    restore_interrupts(ints);
    writePtr = (sample_t *)(XIP_BASE + PROG_RESERVED);
}

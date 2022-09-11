/* A simple driver for the QMC5883L magnetometer chip from QST corporation */

#ifndef QMC5883L_H
#define QMC5883L_H

#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <stdbool.h>
#include <stdint.h>

/* This isnt quite exhaustive, but it contains everything we care about */
enum QMCRegister {
    QMC_XOUT_LSB = 0x00,
    QMC_XOUT_MSB = 0x01,
    QMC_YOUT_LSB = 0x02,
    QMC_YOUT_MSB = 0x03,
    QMC_ZOUT_LSB = 0x04,
    QMC_ZOUT_MSB = 0x05,
    QMC_STATUS   = 0x06,
    QMC_TEMP_LSB = 0x07,
    QMC_TEMP_MSB = 0x08,
    QMC_CONTROL1 = 0x09,
    QMC_CONTROL2 = 0x0A,
    QMC_SETRESET = 0x0B
};

/* Enums for the values that can be written into CONTROL1 */

enum QMCMode {             // The QMC can be put into standby if needed
    QMC_STANDBY    = 0x00,
    QMC_CONTINUOUS = 0x01
};

enum QMCODR {              // Output Data Rates
    QMC_ODR_10HZ   = 0x00,
    QMC_ODR_50HZ   = 0x01,
    QMC_ODR_100HZ  = 0x02,
    QMC_ODR_200HZ  = 0x03
};

enum QMCScale {            // Measurements are done on a scale from 0-x Gauss
    QMC_SCALE_2G   = 0x00,
    QMC_SCALE_8G   = 0x01
};

enum QMCOSR {              // OverSample Ratio; how many samples should be
    QMC_OSR_512    = 0x00, // averaged for a single data point?
    QMC_OSR_256    = 0x01,
    QMC_OSR_128    = 0x02,
    QMC_OSR_64     = 0x03
};

/* Shifts for CONTROL1 */
#define QMC_MODE_SHIFT  0
#define QMC_ODR_SHIFT   2
#define QMC_SCALE_SHIFT 4
#define QMC_OSR_SHIFT   6

/* CONTROL2 just takes boolean values, so just define the shifts */
#define QMC_SOFT_RST    7 // Restore registers to their default values
#define QMC_ROL_PNT     6 // Enable pointer rolling between 0x00-0x06
#define QMC_INT_ENB     0 // Enable the interrupt pin. Not wired in on Bob.

/* Likewise, STATUS only contains boolean values */
#define QMC_DSKIP       2 // Data skip? Not really sure what this does.
#define QMC_DOVL        1 // Overflow flag. Is high if any output goes out of range.
#define QMC_DRDY        0 // Data ready. Is high if new data is ready.

#define QMC_ADDR     0x0D
#define QMC_TIMEOUT  1000

struct qmc_cfg {          // Used for quick setting/analysis of QMC settings.
    enum QMCMode mode;
    enum QMCODR ODR;
    enum QMCScale scale;
    enum QMCOSR OSR;
    bool pointerRoll;
    bool enableInterrupt;
    uint8_t control1Raw;  // Only used by QMCGetCfg. Can be ignored otherwise.
    uint8_t control2Raw;
};

struct qmc_status {
    bool dataReady;
    bool dataOverflow;
    bool dataSkip;
};

struct magData {
    int16_t x;
    int16_t y;
    int16_t z;
};

typedef struct qmc_t {
    i2c_inst_t * i2c;
    struct qmc_cfg config;
} qmc_t;

/* Creates a QMC_T */
qmc_t QMCInit(i2c_inst_t * i2c);

/* The QMC5883L doesnt have any real self-test capability, but we can
 * at least make sure it is talking properly and if it is configured correctly.
 * Returns:
 * 0 if the QMC is talking and is configured to produce data.
 * 1 if the QMC is talking, has a valid configuration, but is on standby.
 * 2 if the QMC has an invalid configuration.
 * 3 if the QMC is not talking */
uint8_t QMCTest(qmc_t * sensor);

/* Reads and parses the QMC status register, places the result in status
 * Returns:
 * 0 if successful
 * 1 if the I2C read failed */
uint8_t QMCGetStatus(qmc_t * sensor, struct qmc_status * status);

/* A quick way to configure the QMC5883L, based on config.
 * N.B. the control 1 & 2 raw fields in config can be safely ignored
 * Returns:
 * 0 if successful
 * 1 if the I2C failed */
uint8_t QMCSetCfg(qmc_t * sensor, struct qmc_cfg config);

/* Reads and parses the config registers on the QMC5883L
 * Stores the result in sensor->config, alongside the raw registers.
 * Returns:
 * 0 if successful
 * 1 if the config is invalid
 * 2 if the I2C failed */
uint8_t QMCGetCfg(qmc_t * sensor);

/* Reads data from the magnetometer and stores it in data
 * Returns:
 * 0 if successful.
 * 1 if the i2c read failed. */
uint8_t QMCGetMag(qmc_t * sensor, struct magData * data);

/* Reads temperature from the magnetometer and stores it in result
 * Returns:
 * 0 if successful.
 * 1 if the i2c read failed.*/
 uint8_t QMCGetTemp(qmc_t * sensor, int16_t * result);
#endif

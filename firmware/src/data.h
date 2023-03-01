#ifndef SENSORS_H
#define SENSORS_H /*  A module for managing and polling the sensors as a unit
    Includes functions and structs for managing the data produced, as they're
    dependent on the configuration of the sensors. */

#include "qmc5883l.h"
#include "qmi8658c.h"

#include <stdint.h>
#include <stdlib.h>


/* Initialises the sensors and the associated i2c bus */
void configureSensors(void);


/*  Returns the magnitude of the vector passed to it.
    Basically just pythagoras. */
float magnitude(int16_t vector[3]);

/* Takes a raw accelerometer reading and returns a value in G */
float accelToG(int16_t accel);

/* Takes a value in G and calculates what the QMI would read */
int16_t gToAccel(float g);

/* Takes a raw reading from the gyro and returns a value in dps */
float gyroToDps(int16_t gyro);

/* Applies calibration coefficients to a magnetometer reading */
void compCalib(int16_t *reading[3], int16_t calib[3]);

/*  Finds the difference in pressure between the most recent
    data packet and the one that was t samples ago . */
int32_t deltaPres(size_t t);

class data
{
    private:

        uint32_t time;      // Time since boot in ms
        uint32_t count;    // Cycle count from boot

        uint32_t press;     // Pressure in pascals
        int32_t temp;       // Temperature in hundreths degree's C

        int16_t magn[3] = {0, 0, 0};    // Raw magentometer output [X, Y, Z]

        int16_t acce[3] = {0, 0, 0};    // Raw IMU output [X, Y, Z]
        int16_t gyro[3] = {0, 0, 0};    // Raw Gyro data [X, Y, z]

        enum states
        {
            TESTING,        // If system is connected over usb
            GROUNDED,         // If system is not connected, but not in flight
            POWERED_ASCENT, // System detects upward acceleration due to rocket motor
            COASTING,       // System coasting upwards on ascent
            POST_APOGEE,    // System descending
            DROGUE_OUT,     // Drogue pyro fired
            MAIN_OUT,       // Main pyro fired
            LANDING,        // Under 150m
            LANDED          // Stationary on the ground
        } flight_state;

        enum sensor_status
        {
            NO_FAULT = 0,           // No fault detected
            BARO_FAULT = 1 << 0,    // Barometer sensor disabled or faulty (etc)
            GYRO_FAULT = 1 << 1,    // Gyroscope failure
            ACCL_FAULT = 1 << 2,    // Accelerometer failure
            MAGN_FAULT = 1 << 3,    // Magnetometer failure
            ADXL_FAULT = 1 << 4,    // ADXL accel failure
            GPS_FAULT = 1 << 5,     // GPS failure
            RADIO_FAULT = 1 << 6,   // Radio failure
            FAULT_RECOVERED = 1 << 7   // Recovered from a fault
        } sensor_state;

        data();

    public:
        QMIGyroScale GYRO_SCALE = QMI_GYRO_256DPS;
        QMIAccelScale ACC_SCALE = QMI_ACC_16G;
        QMCScale COMP_SCALE = QMC_SCALE_2G;
        /*  Tests all the sensors and returns a bitfield showing which sensors
            are active.
            Bit | Sensor
            0  | HP203B   Barometer
            1  | QMI8658C Gyroscope
            2  | QMI8658C Accelerometer
            3  | QMC5883L Compass
            Bit set high indicates a faulty sensor. */
        uint8_t testSensors(void);

        void pollSensors();
        /*  Polls the sensors and returns a data struct
            Takes a bitfield showing the status of all sensors. */
        void pollSensors(uint8_t status);
};

#endif


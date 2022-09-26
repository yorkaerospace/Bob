#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"

#include "stateMachine.h"
#include "sensors.h"

// A 10 second buffer should be ~30 kB
#define BUF_SIZE 10 * 100
// Pressure threshold for a launch
#define PRES_L -100  // About 10m
// Acceleration threshold for a launch
#define ACCL_L (gToAccel(10))
// Pressure threshold to detect apogee
#define PRES_A 100  // About 10m
// Pressure threshold for landing
#define PRES_G 100

volatile uint8_t state = GROUNDED;

// Simple circular buffer.
volatile data_t dataBuf[BUF_SIZE];
volatile uint16_t tail = 0;
volatile uint16_t head = 0;

/* Finds the difference in pressure between the most recent
 * data packet and the one that was time ago. */
static int32_t deltaPres(size_t time) {
    uint32_t currPres = dataBuf[head].pres;
    uint32_t pastPres = dataBuf[(head - time) % BUF_SIZE].pres;

    return currPres - pastPres;
}

/* Polls stdin and interprets what it gets. */
static void pollUsb(void) {
    static const char helpText[] =
        "Bob Rev 3 running build: %s %s\n"
        "Press:\n"
        "b to enter bootsel mode\n"
        "c to clear this tty\n"
        "h to display this help text\n"
        "t to run self-test\n";

    static const char testText[] =
        "\033c"     // Clear the buffer
        "\033[?25l" // Hide the cursor
        "Bob Rev 3 running build: %s %s\n"
        "Timestamp: \n\n"
        "Accelerometer: X:        G   Y:        G   Z:        G     \n"
        "Gyroscope:     X:        dps Y:        dps Z:        dps   \n"
        "Compass:       X:            Y:            Z:              \n"
        "Barometer:     Pressure:         Pa     Temp:        °C    \n"
        "Press any key to exit.\n";

    switch(getchar_timeout_us(0)) {
    case PICO_ERROR_TIMEOUT:         // If there is no char, just break.
        break;
    case 'b':
        printf("Entering bootsel mode...\n");
        reset_usb_boot(0,0);
        break;
    case 'c':
        printf("\033c");
        break;
    case 'h':
        printf(helpText, __TIME__, __DATE__);
        break;
    case 't':
        printf(testText, __TIME__, __DATE__);
        state = TESTING;
        break;
    default:
        printf("?\n");
        break;
    }
}

static void testPrint(void) {
    // Rather than redrawing the entire readout we're just going to blindly
    // redraw bits. Usually you'd do some caching here, but thats for nerds.
    static const char testText[] =
        "\x1b[2;11f %-10d"
        "\x1b[4;19f%6.1f \x1b[7C%6.1f \x1b[7C%6.1f \x1b[5C %s"
        "\x1b[5;19f%6.1f \x1b[7C%6.1f \x1b[7C%6.1f \x1b[5C %s"
        "\x1b[6;19f%6d \x1b[7C%6d \x1b[7C%6d \x1b[5C %s"
        "\x1b[7;26f%7d \x1b[13C%6.2f \x1b[5C %s";

    // I am a sucker for pretty terminal colours. Leave me alone.
    static const char go[] = "\x1b[34;1m [  \x1b[32;1mGO  \x1b[34;1m]  \x1b[0;1m";
    static const char nogo[] = "\x1b[34;1m [ \x1b[31;1mNOGO \x1b[34;1m]  \x1b[0;1m";

    data_t latest = dataBuf[head - 1];
    float accel[3];
    float gyro[3];
    float temp;
    char * goStr[4];

    accel[0] = accelToG(latest.accel[0]);
    accel[1] = accelToG(latest.accel[1]);
    accel[2] = accelToG(latest.accel[2]);

    gyro[0] = gyroToDps(latest.gyro[0]);
    gyro[1] = gyroToDps(latest.gyro[1]);
    gyro[2] = gyroToDps(latest.gyro[2]);

    temp = (float)latest.temp / 100;

    goStr[0] = latest.status & NO_ACCL == 0 ? nogo : go;
    goStr[1] = latest.status & NO_GYRO == 0 ? nogo : go;
    goStr[2] = latest.status & NO_COMP == 0 ? nogo : go;
    goStr[3] = latest.status & NO_BARO == 0 ? nogo : go;

    printf(testText,
           latest.time,
           accel[0], accel[1], accel[2], goStr[0],
           gyro[0], gyro[1], gyro[2], goStr[1],
           latest.mag[0], latest.mag[1], latest.mag[2], goStr[2],
           latest.pres, temp, goStr[3]);

    sleep_ms(200);

    if (getchar_timeout_us(0) != PICO_ERROR_TIMEOUT) {
        printf("\033c \033[?25l");
        state = CONNECTED;
    }
}


void fifoIRQ(void) {
    data_t * data_p;
    int16_t accel_mag;

    // Copy everything out of the FIFO and into the buffer
    while(multicore_fifo_rvalid()) {
        data_p = multicore_fifo_pop_blocking();
        head++;
        if(head >= BUF_SIZE) {
            head = 0;
        }
        dataBuf[head] = *data_p;
        free(data_p);
    }

    // Figure out if state changes are required.
    switch (state) {
    case GROUNDED:
        if (deltaPres(100) < PRES_L ||
            magnitude(dataBuf[head].accel) > ACCL_L) {
            state = ASCENDING;
            dataBuf[head].status |= LAUNCH;
        }
        break;
    case ASCENDING:
        if (deltaPres(100) > PRES_A) {
            state = DESENDING;
            dataBuf[head].status |= APOGEE;
        }
        break;
    case DESENDING:
        if (abs(deltaPres(BUF_SIZE)) < PRES_G) {
            state = GROUNDED;
            dataBuf[head].status |= LANDING;
        }
    }

}

void stateLoop(void) {
    while (true) {
        switch (state) {
        case CONNECTED:
            if (!stdio_usb_connected()) {
                state = GROUNDED;
            } else {
                pollUsb();
            }
            break;
        case TESTING:
            if (!stdio_usb_connected()) {
                state = GROUNDED;

            } else {
                testPrint();
            }
            break;
        case GROUNDED:
            if (stdio_usb_connected()) {
                state = CONNECTED;
            }
            break;
        case ASCENDING:
            break;
        case DESENDING:
            break;
        }
    }
}

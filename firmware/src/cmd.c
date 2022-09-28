#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/bootrom.h"

#include "stateMachine.h"
#include "sensors.h"

// How fast should updatey bits be updated?
#define UPDATE_PERIOD 200

/* Clears the terminal */
static void clearTTY(void) {
    printf("\033c");
}

static void showCursor(bool visible) {
    static char DECTCEM[] = "\033[?25%c";
    char final = visible ? 'h' : 'l';
    printf(DECTCEM, final);
}

static void debugPrint(void) {

    static const char prompt[] =
        "\x1b[1;1f"
        "\x1b[22m"
        "Bob Rev 3 running build: %s %s\x1b[0K\n"
        "Timestamp: %d\x1b[0K\n"
        "\x1b[0K\n"
        "Accelerometer: X: %6.1f G   Y: %6.1f G   Z: %6.1f G"
        "\x1b[0K\x1b[999C\x1b[10D%s\n\x1b[22m"
        "Gyroscope:     X: %6.1f dps Y: %6.1f dps Z: %6.1f dps"
        "\x1b[0K\x1b[999C\x1b[10D%s\n\x1b[22m"
        "Compass:       X: %6d     Y: %6d     Z: %6d"
        "\x1b[0K\x1b[999C\x1b[10D%s\n\x1b[22m"
        "Barometer:     Pressure: %7d Pa     Temp: %6.2f °C"
        "\x1b[0K\x1b[999C\x1b[10D%s\n\x1b[22m"
        "\x1b[0K\n"
        "Press any key to exit.\x1b[0J\n";

    // I am a sucker for pretty terminal colours. Leave me alone.
    static const char go[] = "\x1b[34;1m [  \x1b[32;1mGO  \x1b[34;1m]\x1b[0;1m";
    static const char nogo[] = "\x1b[34;1m [ \x1b[31;1mNOGO \x1b[34;1m]\x1b[0;1m";

    data_t latest;
    float accel[3];
    float gyro[3];
    float temp;
    char * goStr[4];

    int redrawTimer = 0;

    clearTTY();
    showCursor(false);

    while (true) {

        latest = getLatest();

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

        printf(prompt, __TIME__, __DATE__,
               latest.time,
               accel[0], accel[1], accel[2], goStr[0],
               gyro[0], gyro[1], gyro[2], goStr[1],
               latest.mag[0], latest.mag[1], latest.mag[2], goStr[2],
               latest.pres, temp, goStr[3]);

        sleep_ms(UPDATE_PERIOD);
    }

    clearTTY();
    showCursor(true);
}

/* Polls stdin and interprets what it gets. */
void pollUsb(void) {
    static const char helpText[] =
        "Bob Rev 3 running build: %s %s\n"
        "Press:\n"
        "b to enter bootsel mode\n"
        "c to clear this tty\n"
        "d to show the debug prompt\n"
        "h to display this help text\n";

    switch(getchar_timeout_us(0)) {
    case PICO_ERROR_TIMEOUT:         // If there is no char, just break.
        break;
    case 'b':
        printf("Entering bootsel mode...\n");
        reset_usb_boot(0,0);
        break;
    case 'c':
        clearTTY();
        break;
    case 'd':
        debugPrint();
        break;
    case 'h':
        printf(helpText, __TIME__, __DATE__);
        break;
    default:
        printf("?\n");
        break;
    }
}

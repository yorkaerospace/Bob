#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/bootrom.h"

#include "dataBuf.h"
#include "ansi.h"

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
        MOV(1,1) NORM
        "Bob Rev 3 running build: %s %s" CLRLN
        "Timestamp: %d" CLRLN
        CLRLN
        "Accelerometer: X: %6.1f G   Y: %6.1f G   Z: %6.1f G" MOV_EC(10) "%s\n"
        NORM
        "Gyroscope:     X: %6.1f dps Y: %6.1f dps Z: %6.1f dps" MOV_EC(10) "%s\n"
        NORM
        "Compass:       X: %6d     Y: %6d     Z: %6d" MOV_EC(10) "%s\n"
        NORM // Alacritty *really* likes to bold stuff.
        "Barometer:     Pressure: %7d Pa     Temp: %6.2f °C" MOV_EC(10) "%s\n"
        CLRLN NORM
        "Press any key to exit.\x1b[0J\n";

    // I am a sucker for pretty terminal colours. Leave me alone.
    static const char go[] = BLUE "[" GREEN "  GO" BLUE "  ]" WHITE;
    static const char nogo[] = BLUE"[" RED " NOGO " BLUE"]" WHITE;

    data_t latest;
    float accel[3];
    float gyro[3];
    float temp;
    char * goStr[4];

    int redrawTimer = 0;

    clearTTY();
    showCursor(false);

    while (true) {

        dataHead(&latest);

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

        if (getchar_timeout_us(0) != PICO_ERROR_TIMEOUT) {
            break;
        }
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

/* shell.c
 *
 * Implements a simple shell that can run single char commands.
 *
 * Claire H, 2023 */

#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "stdio.h"
#include <hardware/sync.h>
#include <hardware/timer.h>

#include "ansi.h"
#include "taskList.h"
#include "types.h"

// Task list
extern taskList_t tl;

// Latest data points
extern baro_t baroData;
extern imu_t  imuData;
extern comp_t compData;
extern gps_t  gpsData;

// Timer
static repeating_timer_t shellTimer;

/* --------------------- PROTOTYPES -------------------- */

static bool shellIRQ(repeating_timer_t * rt);

/* ----------------------- TASKS ----------------------- */

static void ignore(void) {
    printf(MISSILE);
}

static void debugPlot(void) {
    static const char prompt[] =
        MOV(1,1) NORM
        "Bob Rev 3 running build: %s %s" CLRLN
        "Timestamp: %lu" CLRLN
        CLRLN
        "Accelerometer: X:    %7d  Y:    %7d  Z:    %7d" "\n"
        NORM
        "Gyroscope:     X:    %7d  Y:    %7d  Z:    %7d" "\n"
        NORM
        "Compass:       X:    %7d  Y:    %7d  Z:    %7d " "\n"
        NORM // Alacritty *really* likes to bold stuff.
        "GPS:           Lat:  %7d  Lng:  %7d  Sat:  %7d" "\n"
        NORM
        "Barometer:     Pressure:     %7u Pa      Temp: %7d" "\n"
        NORM
        "Task List:     %d / %d \n"
        NORM
        "\n"
        "Press any key to exit. \n"
        CLRLN NORM
        "\x1b[0J\n";

    printf(prompt, __TIME__, __DATE__, to_ms_since_boot(get_absolute_time()),
           imuData.accl[0], imuData.accl[1], imuData.accl[2],
           imuData.gyro[0], imuData.gyro[1], imuData.gyro[2],
           compData.compass[0], compData.compass[1], compData.compass[2],
           gpsData.lat, gpsData.lon, gpsData.sats,
           baroData.pres, baroData.temp,
           tlSize(&tl), TL_SIZE);
}


/* I had this really nice idea involving stdio IRQs and all that.
 * Would've been beautiful. Wonderful demonstration of event driven programming.
 *
 * Didnt work at all. Instead we have this.
 *
 * Effectively works as a state machine. Whatever char comes in sets the state.
 * If a command is finished, it sets the state to 0. */
static void shellTask(void * ptr) {
    static const char helpText[] =
        "Bob Rev 3 running build: %s %s\n"
        "Press:\n"
        "b to enter bootsel mode\n"
        "c to clear the contents of the flash\n"
        "d to show the debug prompt\n"
        "h to display this help text\n"
        "r to read files\n";

    static char shellState = 0;
    int in = getchar_timeout_us(0);
    if (in != PICO_ERROR_TIMEOUT) {
        shellState = in;
        printf(CLRSC);
    }

    switch(shellState) {
    case 'b':
        printf("Entering bootsel mode...\n");
        reset_usb_boot(0,0);
        break;
    case 'c':
        shellState = 0;
        break;
    case 'd':
        debugPlot();
        break;
    case 'h':
        printf(helpText, __TIME__, __DATE__);
        shellState = 0;
        break;
    case 'm':
        ignore();
        shellState = 0;
        break;
    }
}

/* ------------------------ ISRs ----------------------- */


static bool shellIRQ(repeating_timer_t * rt) {
    tlAdd(&tl, shellTask, NULL);
    return true;
}

/* ------------------------ INIT ----------------------- */

void shellInit(void) {
    add_repeating_timer_ms(500, shellIRQ, NULL, &shellTimer);
}

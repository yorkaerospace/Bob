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
#include "flash.h"

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

static void flashWipe(void) {
    printf(NORM
           "Are you sure you wish to clear the flash? "
           "["GREEN "Y" WHITE "/" RED "N" WHITE "]\n"
           NORM);
    switch(getchar_timeout_us(30000000)){
    case PICO_ERROR_TIMEOUT:
        printf("Timed out due to lack of response, please try again\n");
        break;
    case 'y':
        printf("Clearing flash. (This may take a while) \n");
        fErase();
        printf("Done!\n");
        break;
    }
}


/* Dumps n records from flash. Returns the number actually read. */
static int flashDump(int n) {
    int i, j;
    log_t l;
    for(i = 0; i < n; i++) {
        // Attempt to read from flash.
        if(fRead(&l)) {
            return 1;
        }
        switch (l.type) {
        case BARO:;
            baro_t b = l.data.baro;
            printf("BARO, %u, %u, %d, %d\n", b.time,
                   b.pres, b.temp, b.vVel);
            break;
        case IMU:;
            imu_t i = l.data.imu;
            printf("IMU, %u, %d, %d, %d, %d, %d, %d\n", i.time,
                   i.accl[0], i.accl[1], i.accl[2],
                   i.gyro[0], i.gyro[1], i.gyro[2]);
            break;
        case COMP:;
            comp_t c = l.data.comp;
            printf("COMP, %u, %d, %d, %d\n", c.time,
                   c.compass[0], c.compass[1], c.compass[2]);
            break;
        case GPS:;
            gps_t g = l.data.gps;
            printf("GPS, %u, %u, %u, %u, %d, %d, %u\n", g.time,
                   g.utc[0], g.utc[1], g.utc[2], g.lat, g.lon, g.sats);
            break;
        }
    }
    return 0;
}

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
        "Filter:        %7d \n "
        NORM
        "Task List:     %d / %d \n"
        NORM
        "Flash:         %d kiB \n"
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
           baroData.pres, baroData.temp, baroData.vVel,
           tlSize(&tl), TL_SIZE,
           fUsed());
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
        flashWipe();
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
    case 'r':
        if(flashDump(100) != 0) {
            shellState = 0;
            fRewind();
        }
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

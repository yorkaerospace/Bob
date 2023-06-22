#include "pico/stdlib.h"
#include "stdio.h"

#include "ansi.h"
#include "sampler.h"
#include "taskList.h"
#include "types.h"
#include "hat.h"
#include "flash.h"
#include "shell.h"

enum states state = FAULT_HANDLER;

// Latest data packets from the sensors.
// Made global for other tasks to have access to them
// Since context switching isn't a thing, we can just do this!
baro_t baroData = {0};
imu_t  imuData = {0};
comp_t compData = {0};
gps_t gpsData = {0};

// Task list
taskList_t tl;

int main() {
    stdio_init_all();

    tl = tlInit();

    configureSensors();
    hatInit();
    shellInit();
    fInit();

    while (true) {
        tlRun(&tl);
    }
}

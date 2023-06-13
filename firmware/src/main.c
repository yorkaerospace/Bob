#include "stdio.h"

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"

#include "ansi.h"
#include "sampler.h"
#include "taskList.h"

enum states {
    PLUGGED_IN,  // Connected to USB
    DATA_OUT,    // Printing flight data to USB
    DEBUG_PRINT, // Printing debug data to USB
    DEBUG_LOG,   // Logging data while plugged in
    LOG          // Logging data while unplugged
};

enum states state = PLUGGED_IN;
uint32_t readIndex = 0;

taskList_t tl;

int main() {
    stdio_init_all();
    sleep_ms(10000);
    printf("A\n");

    tl = tlInit();

    printf("B\n");
    configureSensors();

    printf("C\n");
    while (true) {
        tlRun(&tl);
    }
}

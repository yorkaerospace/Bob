#include "stdio.h"

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"

#include "ansi.h"
#include "sampler.h"
#include "taskList.h"
#include "types.h"

enum states state = BOOT;

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

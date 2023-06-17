#include "stdio.h"

#include "pico/stdlib.h"

#include "ansi.h"
#include "sampler.h"
#include "taskList.h"
#include "types.h"
#include "hat.h"

enum states state = BOOT;

conf_t cfg = {0};

taskList_t tl;

int main() {
    stdio_init_all();
    sleep_ms(10000);

    tl = tlInit();

    configureSensors();
    hatInit();

    while (true) {
        tlRun(&tl);
    }
}

#include "usbcmd.h"

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"

static const char helpText[] =
    "Bob Rev 3 running build: %s\n"
    "Press:\n"
    "b to enter bootsel mode\n"
    "c to clear this tty\n"
    "h to display this help text\n";

static uint8_t state;
static repeating_timer_t timer;

/* Polls stdin and interprets what it gets.
 * This is a biiig IRQ... */
static bool usbCmdPoller() {
    if (state != 2) {
        switch(getchar_timeout_us(0)) {
        case PICO_ERROR_TIMEOUT:         // If there is no char, just break.
            break;
        case 'b':
            printf("Entering bootsel mode...");
            reset_usb_boot(0,0);
            break;
        case 'c':
            printf("\033c");
            break;
        case 'h':
            printf(helpText, __DATE__);
            break;
        }
        return true;
    } else {          // If a kill has been scheduled, then return false.
        state = 0;    // This tells the alarm to stop executing the poller.
        return false;
    }
}

/* Initialises the USB command handler
 * Returns: 0 on success
 * 1 if USB command handler is already running
 * 2 on other failure */
uint8_t usbCmdInit() {
    bool timerSuccess;
    if(state != 1) {
        timerSuccess = add_repeating_timer_us(-1000000 / USB_POLLING_FREQ,
                                               usbCmdPoller, NULL, &timer);
        if(timerSuccess){
            state = 1;
            return 0;
        } else {
            return 2;
        }
    } else {
        return 1;
    }
}

/* Instructs the command handler to stop itself
 * Returns 0 on success
 * 1 if the command handler was not running */
uint8_t usbCmdKill() {
    if(state != 0 ) {
        state = 2;
        return 0;
    } else {
        return 1;
    }
}

/* Gets the status of the command handler
 * Returns 0 if not running
 * 1 if running
 * 2 if kill has been scheduled */
uint8_t usbCmdStatus() {
    return state;
}

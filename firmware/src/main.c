#include "stdio.h"

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"

#include "ansi.h"

enum states {
    PLUGGED_IN,  // Connected to USB
    DATA_OUT,    // Printing flight data to USB
    DEBUG_PRINT, // Printing debug data to USB
    DEBUG_LOG,   // Logging data while plugged in
    LOG          // Logging data while unplugged
};

enum states state = PLUGGED_IN;

int main() {
    stdio_init_all();
    configureSensors();

    while (true) {
        switch (state) {
        case PLUGGED_IN:
            // State transition logic
            state = stdio_usb_connected() ? PLUGGED_IN : LOG;

            // Interpret commands
            switch(getchar_timeout_us(0)) {
            case PICO_ERROR_TIMEOUT:         // If there is no char, just break.
                break;
            case 'd':
                state = DEBUG_PRINT;
                break;
            case 'l':
                state = DEBUG_LOG;
                break;
            case 'r':
                state = DATA_OUT;
                break;
            }
            break;
        case LOG:
            state = stdio_usb_connected() ? PLUGGED_IN : LOG;
            break;
        case DEBUG_PRINT:
            // If unplugged, go to LOG
            state = stdio_usb_connected() ? DEBUG_PRINT : LOG;
            // Return to PLUGGED_IN if the user presses a key
            state = getchar_timeout_us(0) == PICO_ERROR_TIMEOUT ? DEBUG_PRINT : PLUGGED_IN;
            break;
        case DEBUG_LOG:
            // If unplugged, go to LOG
            state = stdio_usb_connected() ? DEBUG_LOG : LOG;
            // Return to PLUGGED_IN if the user presses a key
            state = getchar_timeout_us(0) == PICO_ERROR_TIMEOUT ? DEBUG_LOG : PLUGGED_IN;
            break;
        case DATA_OUT:
            // If unplugged, go to LOG
            state = stdio_usb_connected() ? DEBUG_PRINT : LOG;
            break;
        }
    }
}

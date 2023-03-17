#include "stdio.h"

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"

#include "ansi.h"
#include "sampler.h"

enum states {
    PLUGGED_IN,  // Connected to USB
    DATA_OUT,    // Printing flight data to USB
    DEBUG_PRINT, // Printing debug data to USB
    DEBUG_LOG,   // Logging data while plugged in
    LOG          // Logging data while unplugged
};

enum states state = PLUGGED_IN;

int main() {
    sample_t sample;
    uint32_t readIndex = 0;
    absolute_time_t nextPoll;

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
                readIndex = 0;
                state = DATA_OUT;
                break;
            case 'c':
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
                    clearFlash();
                    printf("Done!\n");
                    break;
                }
                break;
            case 'b':
                printf("Entering bootsel mode...\n");
                reset_usb_boot(0,0);
                break;
            default:
                printf("?\n");
                break;
            }
            break;
        case LOG:
            state = stdio_usb_connected() ? PLUGGED_IN : LOG;

            nextPoll = make_timeout_time_ms(10);
            sample = getSample();
            logSample(sample);
            sleep_until(nextPoll);

            break;
        case DEBUG_PRINT:
            // If unplugged, go to LOG
            state = stdio_usb_connected() ? DEBUG_PRINT : LOG;
            // Return to PLUGGED_IN if the user presses a key
            state = getchar_timeout_us(0) == PICO_ERROR_TIMEOUT ? DEBUG_PRINT : PLUGGED_IN;

            nextPoll = make_timeout_time_ms(10);
            sample = getSample();
            prettyPrint(sample, "Press any key to exit");
            sleep_until(nextPoll);

            break;
        case DEBUG_LOG:
            // If unplugged, go to LOG
            state = stdio_usb_connected() ? DEBUG_LOG : LOG;
            // Return to PLUGGED_IN if the user presses a key
            state = getchar_timeout_us(0) == PICO_ERROR_TIMEOUT ? DEBUG_LOG : PLUGGED_IN;

            nextPoll = make_timeout_time_ms(10);
            sample = getSample();
            logSample(sample);
            prettyPrint(sample, "Press any key to stop logging");
            sleep_until(nextPoll);

            break;
        case DATA_OUT:
            // If unplugged, go to LOG
            state = stdio_usb_connected() ? DEBUG_PRINT : LOG;
            // If we're out of data, go to PLUGGED_IN
            state = readSample(readIndex, &sample) ? PLUGGED_IN : DATA_OUT;

            printf("%ld, %d, %ld, %ld, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
                   sample.time, sample.status, sample.pres, sample.temp,
                   sample.mag[0], sample.mag[1], sample.mag[2],
                   sample.accel[0], sample.accel[1], sample.accel[2],
                   sample.gyro[0], sample.gyro[1], sample.gyro[2]);

            readIndex++;
            break;
        }
    }
}

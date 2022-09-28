#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "sensors.h"

enum states {
    CONNECTED, // If system is connected over usb
    GROUNDED,  // If system is not connected, but not in flight
    ASCENDING,
    DESENDING
};

/* Copies the newest packet from the data buffer
 * Should make writing terminal stuff a bit safer as we dont
 * need to play with that data buffer. */
data_t getLatest(void);

void fifoIRQ(void);

void stateLoop(void);

#endif

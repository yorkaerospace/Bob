#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "sensors.h"

enum states {
    CONNECTED, // If system is connected over usb
    GROUNDED,  // If system is not connected, but not in flight
    ASCENDING,
    DESENDING
};

void stateLoop(void);

#endif

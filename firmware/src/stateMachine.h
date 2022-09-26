#ifndef STATEMACHINE_H
#define STATEMACHINE_H

enum states {
    CONNECTED, // If system is connected over usb
    TESTING,   // If system is displaying test data
    GROUNDED,  // If system is not connected, but not in flight
    ASCENDING,
    DESENDING
};

void fifoIRQ(void);

void stateLoop(void);

#endif

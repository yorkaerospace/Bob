#ifndef STATES_H
#define STATES_H

enum states {
    CONNECTED, // If system is connected over usb
    GROUNDED,  // If system is not connected, but not in flight
    ASCENDING,
    DESENDING
};

#endif

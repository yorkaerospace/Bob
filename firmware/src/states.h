#ifndef STATES_H
#define STATES_H

enum states
{
    TESTING, // If system is connected over usb
    GROUND,  // If system is not connected, but not in flight
    POWERED_ASCENT,
    COASTING,
    POST_APOGEE,
    DROGUE_OUT,
    MAIN_OUT,
    LANDING,
    LANDED
};

#endif

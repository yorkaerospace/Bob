#ifndef GPS_H
#define GPS_H

#include "stdint.h"

typedef struct GPS
{
    long Time;                   // Time as read from GPS, as an integer but 12:13:14 is 121314
    long SecondsInDay;           // Time in seconds since midnight.
    int Hours, Minutes, Seconds;
    int32_t Longitude, Latitude;
    long Altitude;
    unsigned int Satellites;
    unsigned int FixQuality;
    float HDOP;
    float GeoidSeparation;

} GPS;

void setup_gps();
void reset_gps();
void readGPSData(GPS *gps);

#endif

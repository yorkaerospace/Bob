#ifndef LOGGER_H
#define LOGGER_H

#include "sensors.h"
#include "spiffs/xip.h"

/* Opens a new log file, returns its file handle */
void newLog(void);

/* Moves all data from the buffer to the file. */
void logData(void);

/* Dumps all the logs to stdout */
void dumpLogs(void);

/* Deletes all the log files
 * Blatantly stolen from the SPIFFS examples. */
void clearLogs(void);

void closeLog(void);

#endif

#include "spiffs/xip.h"
#include <stdio.h>

#include "dataBuf.h"

extern spiffs fs;
spiffs_file f;

static int8_t countLogs(void) {
    int8_t logs = 0;
    char *prefix = "log_";
    spiffs_DIR d;
    struct spiffs_dirent e;
    struct spiffs_dirent *pe = &e;

    SPIFFS_opendir(&fs, "/", &d);
    while ((pe = SPIFFS_readdir(&d, pe))) {
        if (0 == strncmp(prefix, (char *)pe->name, strlen(prefix))) {
            logs++;
        }
    }
    SPIFFS_closedir(&d);
    return logs;
}

/* Opens a new log file */
void newLog(void) {
    char fileName[32];
    int8_t logs = countLogs();
    snprintf(fileName, 32, "log_%d", logs);
    f = SPIFFS_open(&fs, "my_file", SPIFFS_CREAT |
                                       SPIFFS_TRUNC |
                                       SPIFFS_RDWR, 0);
}

void closeLog(void) {
    SPIFFS_close(&fs, f);
}

/* Moves all data from the buffer to the file. */
void logData(void) {
    data_t data;
    while (dataPop(&data) > 0) {
        // Spiffs has caches built in, so no need to buffer anything here.
        if(SPIFFS_write(&fs, f, &data, sizeof(data_t)) < 0) {
            break; // If something breaks, come back later? Maybe its fixed.
        }
    }
}

/* Dumps all the logs to stdout */
void dumpLogs(void) {
    // remove all files starting with "tmp_"
    char *search_prefix = "log_";
    spiffs_DIR d;
    struct spiffs_dirent e;
    struct spiffs_dirent *pe = &e;
    int res;

    data_t cur;

    float accel[3];
    float gyro[3];
    float temp;

    spiffs_file fd = -1;

    SPIFFS_opendir(&fs, "/", &d);
    while ((pe = SPIFFS_readdir(&d, pe))) {
        if (0 == strncmp(search_prefix, (char *)pe->name, strlen(search_prefix))) {
            // found one
            fd = SPIFFS_open_by_dirent(&fs, pe, SPIFFS_RDWR, 0);
            if (fd < 0) {
                printf("errno %i\n", SPIFFS_errno(&fs));
                return;
            }
            printf("-- %s --\n", (char *)pe->name);
            printf("Time, Ax, Ay, Az, Gx, Gy, Gz, Mx, My, Mz, Pa, °C\n");

            while(SPIFFS_read(&fs, fd, &cur, sizeof(data_t)) > 0) {

                accel[0] = accelToG(cur.accel[0]);
                accel[1] = accelToG(cur.accel[1]);
                accel[2] = accelToG(cur.accel[2]);

                gyro[0] = gyroToDps(cur.gyro[0]);
                gyro[1] = gyroToDps(cur.gyro[1]);
                gyro[2] = gyroToDps(cur.gyro[2]);

                temp = (float)cur.temp/100;

                printf("%d, %f, %f, %f, %f, %f, %f, %d, %d, %d, %d, %f\n",
                       cur.time,
                       accel[0], accel[1], accel[2],
                       gyro[0], gyro[1], gyro[2],
                       cur.mag[0], cur.mag[1], cur.mag[2],
                       cur.pres, cur.temp
                    );
            }

            res = SPIFFS_close(&fs, fd);
            if (res < 0) {
                printf("errno %i\n", SPIFFS_errno(&fs));
                return;
            }
        }
    }
    SPIFFS_closedir(&d);
}

/* Deletes all the log files
 * Blatantly stolen from the SPIFFS examples. */
void clearLogs(void) {
    char *search_prefix = "log_";
    spiffs_DIR d;
    struct spiffs_dirent e;
    struct spiffs_dirent *pe = &e;
    int res;

    spiffs_file fd = -1;

    SPIFFS_opendir(&fs, "/", &d);
    while ((pe = SPIFFS_readdir(&d, pe))) {
        if (0 == strncmp(search_prefix, (char *)pe->name, strlen(search_prefix))) {
            // found one
            fd = SPIFFS_open_by_dirent(&fs, pe, SPIFFS_RDWR, 0);
            if (fd < 0) {
                printf("errno %i\n", SPIFFS_errno(&fs));
                return;
            }
            res = SPIFFS_fremove(&fs, fd);
            if (res < 0) {
                printf("errno %i\n", SPIFFS_errno(&fs));
                return;
            }
            res = SPIFFS_close(&fs, fd);
            if (res < 0) {
                printf("errno %i\n", SPIFFS_errno(&fs));
                return;
            }
        }
    }
    SPIFFS_closedir(&d);
}

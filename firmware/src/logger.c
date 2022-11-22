#include <stdio.h>
#include <stdlib.h>

#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <assert.h>

#include "sensors.h"
#include "dataBuf.h"

// Should be about 7, but may change if more stuff is added to data_t
// One byte is reserved for metadata.
#define STRUCTS_PER_PAGE 255/sizeof(data_t)

// How much space should be reserved for program code?
#define PROG_RESERVED (1024 * 1024)

// Unions are weird, they're like a struct, but everything is in the same place.
// This *does* give us a very nice way to pad a struct out to a certian size,
// as the union will reserve enough space for its biggest member.

typedef union
{
    struct contents
    {
        uint8_t metadata;
        data_t data[STRUCTS_PER_PAGE];
    } contents;
    uint8_t padding[256];
} page;

// Just to be safe, perhaps someone did something weird to data_t.
static_assert(sizeof(page) == 256, "Page struct isnt the size of a page!!!");

// A pointer to the start of the data, as an XIP address
static const page *dataStart = (void *) XIP_BASE + PROG_RESERVED;

// A blank page struct used as a buffer.
static page buf;

static uint32_t writeIndex = 0;

extern mutex_t flashMtx;


/*  Clears data from the flash.
    Has no error checking because apparently the SDK thinks we dont need that. */
void clearData(void)
{
    flash_range_erase(PROG_RESERVED, 7 * 1024 * 1024);
    writeIndex = 0;
}


/*  Flushes the page buffer into the flash proper.
    Returns:
    The number of data structs written if successful
    -1 if an error occured.

    Will also probably hardfault if an error occurs >:( */
uint8_t flushData(void)
{
    page *cur = dataStart + writeIndex;
    uint32_t ints;

    printf("Reading from flash\n");

    // Skip past all the written blocks
    while(cur->contents.metadata != 0xFF)
    {
        writeIndex++;
        cur++;
    }

    if((int) cur >= PICO_FLASH_SIZE_BYTES + XIP_BASE - 256)
        return -1;

    printf("Writing to %#X\n", cur);
    mutex_enter_blocking(&flashMtx);
    printf("Mutex claimed\n");
    ints = save_and_disable_interrupts();
    flash_range_program(PROG_RESERVED + 256 * writeIndex, &buf, 256);
    restore_interrupts(ints);
    mutex_exit(&flashMtx);
    buf.contents.metadata = 0;
    return cur->contents.metadata;
}

/*  Pushes data to the write buffer, then flushes it if it's full
    Returns:
    1 if data was written to flash
    0 if data was buffered
    -1 if an error occured during a write */
uint8_t writeData(data_t data)
{
    buf.contents.data[buf.contents.metadata] = data;
    buf.contents.metadata++;

    if(buf.contents.metadata >= STRUCTS_PER_PAGE)
    {
        if(flushData() != -1)
            return 1;

        else
        {
            buf.contents.metadata = 0;
            return -1;
        }
    }

    return 0;
}

/*  Writes all the data in databuf to the flash
    Returns the number of structs written. */
uint8_t writeAll(void)
{
    data_t cur;
    uint8_t i = 0;

    while(dataSize() != 0)
    {
        dataPop(&cur);
        writeData(cur);
        i++;
    }

    return i;
}

/* Writes the logs to STDOUT as a nice CSV */
void dumpLogs(void)
{
    uint32_t j = 0;

    data_t latest;
    float accel[3];
    float gyro[3];
    float temp;
    page *cur = dataStart;

    printf("Timestamp, Flags, AX, AY, AZ, GX, GY, GZ, CX, CY, CZ, P, T\n");

    while(cur->contents.metadata != 0xFF &&
            (int)cur >= PICO_FLASH_SIZE_BYTES + XIP_BASE)
    {
        for(j = 0; j < cur->contents.metadata; j++)
        {
            latest = cur->contents.data[j];

            accel[0] = accelToG(latest.accel[0]);
            accel[1] = accelToG(latest.accel[1]);
            accel[2] = accelToG(latest.accel[2]);

            gyro[0] = gyroToDps(latest.gyro[0]);
            gyro[1] = gyroToDps(latest.gyro[1]);
            gyro[2] = gyroToDps(latest.gyro[2]);

            temp = (float)latest.temp / 100;

            printf("%d, %X, %f, %f, %f, %f, %f, %f, %d, %d, %d, %d, %f\n",
                   latest.time, latest.status,
                   accel[0], accel[1], accel[2],
                   gyro[0], gyro[1], gyro[2],
                   latest.mag[0], latest.mag[1], latest.mag[2],
                   latest.pres, temp);
        }

        cur++;
    }
}

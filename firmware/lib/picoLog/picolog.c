#include <string.h> // Memset
#include "../include/w25log.h"

#define PAGE 256

// Increment with loop around
// This would normaly be a modulo, but ARM doesnt have that.
#define LOOPINC(i, max) i + 1 >= max ? 0 : i + 1;


struct w25_t {
    w25Conf_t conf;

    // Enable writing to flash
    bool writeEnable;

    // Buffer pages before they're actually written
    uint8_t pageBuf[PAGE * 3];
    size_t pageBufPtr;

    // Head and tail of circular buffer
    size_t head;
    size_t tail;

    // Used to track position in flash.
    size_t readPtr;
    size_t writePtr;
};

/* Initialises a w25 device */
int w25Init(w25_t * w25, w25Conf_t conf) {
    w25->conf = conf;
    w25->writeEnable = false;
    w25->head = 0;
    w25->tail = 1;
    w25->readPtr = 0;
    w25->writePtr = 0;

    w25Entry_t buf;

    if(w25Busy(w25)) {
        return FLASH_BUSY;
    }

    // Figure out where existing records end.
    while (w25Read(w25, &buf) != FLASH_EOF);

    // Set the write pointer to sit on a page boundary
    pl->pageBufPtr = pl->readPtr & 0xFF;
    pl->writePtr = pl->readPtr - pl->pageBufPtr;
    plRewind(pl);

    // Set the page buffer to 0xFF so we dont accidentally overwrite stuff
    memset(pl->pageBuf, 0xFF, sizeof(pl->pageBuf));

    return 0;
}

/* Checks if the flash is busy */
int plBusy(pl_t * pl) {
    uint8_t cmd = 0x05;
    uint8_t busy;

    pl->conf.spiCS(false);
    pl->conf.spiWrite(&cmd, 1);
    pl->conf.spiRead(&busy, 1);
    pl->conf.spiCS(true);
    return busy;
}

/* Reads the next entry from the flash and puts it into buf */
int plRead(pl_t * pl, plEntry_t * buf) {
    uint8_t cmd[4];

    if(plBusy(pl)) {
        // Check if flash is busy and dont proceed if it is.
        return FLASH_BUSY;
    } else if (pl->readPtr >= pl->conf.chip) {
        // Check if we've reached the end of the flash.
        return FLASH_EOF;
    } else {

        // Package the command
        cmd[0] = 0x03;
        cmd[1] = pl->readPtr >> 16;
        cmd[2] = pl->readPtr >> 8;
        cmd[3] = pl->readPtr;

        // SPI transaction (with small degree of pointer fuckery)
        pl->conf.spiCS(false);
        pl->conf.spiWrite(cmd, 4);
        pl->conf.spiRead(buf, sizeof(plEntry_t));
        pl->conf.spiCS(true);

        // Check if we've reached the end of written flash.
        if(buf->header.marker == 0xFF) {
            return FLASH_EOF;
        } else {
            // Increment read pointer.
            pl->readPtr += buf->header.size + sizeof(struct plHeader_s);
            return 0;
        }
    }
}

/* Erases all data in the flash. Returns the expected time in ms. */
int plErase(pl_t * pl) {
    uint8_t cmd = 0xC7;

    pl->conf.spiCS(false);
    pl->conf.spiWrite(&cmd, 1);
    pl->conf.spiCS(true);

    // Holy moly this is slow. Dont do this in flight.
    return 50000;
}

/* Creates a log from the data in buf and pushes it to the circular buffer
 * N.B. May overwrite data if the circular buffer is full. */
void plPush(pl_t * pl, uint8_t * buf, uint8_t size, uint8_t type) {
    plEntry_t e;

    // Set e to 0xFF to avoid writing nonsense.
    memset(&e, 0xFF, sizeof(plEntry_t));

    // Package up the rest of the entry
    e.header.marker = 0xAA;
    e.header.size = size;
    e.header.type = type;
    memcpy(buf, e.data, size);

    pl->conf.mtxClaimBlocking();
    pl->conf.circBuf[pl->writePtr] = e;

    // Increment read pointer
    pl->head = LOOPINC(pl->head, pl->conf.bufSize);
    // Increment write ptr if needed
    if (pl->head == pl->tail) {
        pl->tail = LOOPINC(pl->tail, pl->conf.bufSize);
    }

    pl->conf.mtxRelease();
}

/* Pops an entry from the circular buffer, stores it in buf.
 * Returns 0 on success or -1 on failure */
static int plPop(pl_t * pl, plEntry_t * buf) {

    pl->conf.mtxClaimBlocking();

    if (pl->head == pl->tail) {
        pl->conf.mtxRelease();
        return -1;
    }

    memcpy(buf, pl->conf.circBuf + pl->readPtr, sizeof(plEntry_t));
    pl->tail = LOOPINC(pl->tail, pl->conf.bufSize);

    pl->conf.mtxRelease();
    return 0;
}

/* Flushes the circular buffer into the page buffer when writeEnable
 * is true. Always flushes the page buffer to flash.
 * Returns the expected time of the flash operation in ms, or one of plERR */
int plTask(pl_t * pl) {

     if (pl->writePtr >= pl->conf.chip) {
         return FLASH_FULL;
     }

     plEntry_t buf;
     size_t size;
     uint8_t cmd[260];

     while (pl->pageBufPtr < 256 && plPop(pl, &buf) != -1) {
         size = sizeof(struct plHeader_s) + buf.header.size;
         memcpy(pl->pageBuf + pl->pageBufPtr, &buf, size);
         pl->pageBufPtr =+ size;
     }
     if (pl->pageBufPtr < 256) {
         memset(cmd, 0xFF, sizeof(cmd));

         cmd[0] = 0x02;
         cmd[1] = pl->writePtr >> 16;
         cmd[2] = pl->writePtr >>  8;
         cmd[3] = pl->writePtr;

         // Copy the page into the command
         memcpy(cmd + 4, pl->pageBuf, 256);
         // Replace the 256 bytes we just wrote
         memcpy(pl->pageBuf, pl->pageBuf+256, 512);

         pl->conf.spiCS(false);
         pl->conf.spiWrite(cmd, 260);
         pl->conf.spiCS(true);

         pl->writePtr += 256;

         return 5;
     }
     return 0;
}

/* Sets and unsets the write enable flag */
void plWriteEnable(pl_t * pl, bool we) {
    pl->writeEnable = we;
}

/* Resets the read pointer to the start of the flash */
void plRewind(pl_t * pl) {
    pl->readPtr = 0;
}

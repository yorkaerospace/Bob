/* This module defines a task for gathering data from a GPS module and
 * transmitting it over LoRa.
 *
 * Unfortunately some dependencies are C++ only, so this module is written
 * in C-like C++.
 *
 * Claire Hinton, 2023 */


#include "hardware/timer.h"
#include "hardware/spi.h"
#include <hardware/sync.h>
#include <pico/stdlib.h>
#include <string.h>
#include <stdio.h>

#include "LoRa-RP2040.h"
#include "minmea.h"

extern "C" {
#include "types.h"
#include "taskList.h"
}

// UART Constants
#define BAUD_RATE     9600
#define DATA_BITS     8
#define STOP_BITS     1
#define PARITY        UART_PARITY_NONE
#define UART_TX_PIN   0
#define UART_RX_PIN   1

// DMA Constants
#define DMA_CH        0
#define DMA_BUF_SIZE  512
#define DMA_BUF_LOG2  9

#define MINMEA_MAX_SENTENCE_LENGTH 80

extern "C" {
extern taskList_t tl;
extern baro_t baroData;
}

static char buf[MINMEA_MAX_SENTENCE_LENGTH];

// Again, provide gps data for... stuff to play with.
gps_t gpsData = {0};

/* ------------------------- STRUCTS ----------------------- */

#pragma pack(1)
struct packet {
    // Packet information
    uint32_t seq_no;
    uint32_t time_ms;  // ms since boot
    char     vid;      // Vehicle ID

    // GPS data
    uint32_t time_utc; // Seconds since midnight
    int32_t  lat, lng; // Position
    uint8_t  sat;      // Satellites

    // Baro data
    uint32_t pres;     // Pascals
    int16_t  temp;     // centidegrees
};
#pragma pack()

/* ------------------------- TASKS ------------------------ */

static void hatTask(void * data) {
    static unsigned int packetsSent = 0;
    struct packet p = {0};
    uint32_t time;

    printf("%s", buf);

    struct minmea_sentence_gga frame;
    if (minmea_parse_gga(&frame, buf)\) {
        printf("Good!\n");
        // Convert time to seconds
        time = frame.time.hours * 3600
             + frame.time.minutes * 60
             + frame.time.seconds;

        // Bundle everything into the packet.
        p.seq_no   = packetsSent;
        p.time_ms  = to_ms_since_boot(get_absolute_time());
        p.vid      = 'F';
        p.time_utc = time;
        p.lat      = minmea_rescale(&frame.latitude, 1000);
        p.lng      = minmea_rescale(&frame.longitude, 1000);
        p.sat      = frame.satellites_tracked;
        p.pres     = baroData.pres;
        p.temp     = baroData.temp;

        //printf("time = %d, lat = %d, lng = %d, sat = %d, baro = %d, temp = %d\n",
        //       p.time_utc, p.lat, p.lng, p.sat, p.pres, p.temp);

        LoRa.beginPacket();
        LoRa.write((const unsigned char * ) &p, sizeof(struct packet));
        LoRa.endPacket();

        packetsSent++;
    }
    // Reenable the IRQ
    uart_set_irq_enables(uart0, true, false);
}

/* ------------------------- IRQs ------------------------- */

/* RX interrupt handler
 * Gets data from UART and tries to filter for NMEA sentences.
 * DMA doesnt really work here as I need this filtering, and
 * NMEA sentences are of variable length. */
static void uartRX() {
    static bool copying = false; // Reject any gibberish on boot
    static uint8_t index = 0;

    if (uart_is_readable(uart0)) {
        uint8_t ch = uart_getc(uart0);
        if (ch == '$') { // Start of sentence. Reset index.
            copying = true;
            index = 0;
        }

        if (copying && index < MINMEA_MAX_SENTENCE_LENGTH) {
            buf[index] = ch;
            index++;
            // End of sentence, add task and disable this IRQ
            if(ch == '\n') {
                tlAdd(&tl, hatTask, NULL);
                uart_set_irq_enables(uart0, false, false);
            }
        }
    }
}


/* ------------------------ CONFIG ------------------------ */

extern "C" void hatInit() {
    const uint8_t GGA[] = "$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n";

    /* ----- Set up SPI & LoRa ----- */
    spi_init(spi0, 1000 * 1000);

    gpio_set_function(2, GPIO_FUNC_SPI);
    gpio_set_function(3, GPIO_FUNC_SPI);
    gpio_set_function(4, GPIO_FUNC_SPI);

    LoRa.setSPI(*spi0);
    LoRa.setPins(28, 29);

    if(LoRa.begin(868E6)) {
        LoRa.setTxPower(9);
        LoRa.setSpreadingFactor(12);
        LoRa.setSignalBandwidth(250E3);
    }

    printf("initing UART\n");

    /* ---- Set up UART & GPS ---- */
    uart_init(uart0, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(uart0, DATA_BITS, STOP_BITS, PARITY);
    uart_set_hw_flow(uart0, false, false);
    uart_set_fifo_enabled(uart0, false);

    irq_set_exclusive_handler(UART0_IRQ, uartRX);
    irq_set_enabled(UART0_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(uart0, true, false);

    // Set the GPS to only output GGA
    uart_write_blocking(uart0, GGA, sizeof(GGA));



}

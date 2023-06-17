#include "hardware/flash.h"
#include "hardware/timer.h"
#include "hardware/spi.h"
#include <hardware/sync.h>
#include "LoRa.h"
#include "gps.h"
#include "types.h"
#include "taskList.h"

#include <pico/stdlib.h>
#include <string.h>

// Externs
extern taskList_t tl;

static repeating_timer_t hatTimer;
static lora_t lr = {0};
static GPS gps;

// Again, provide gps data for... stuff to play with.
gps_t gpsData = {0};

struct Packet {
    uint32_t seq_no;
    uint32_t time_ms;
    double gps_lat, gps_lng, gps_alt;
    uint32_t gps_sat, gps_tme;
    double bmp_tmp, bmp_prs, bmp_alt;
};

static void hatTask(void * data) {

    readGPSData(&gps);

    struct Packet p;

    // Get the bits we actually want.
    p.gps_tme = gps.SecondsInDay;
    p.gps_lat = gps.Latitude;
    p.gps_lng = gps.Longitude;
    p.gps_sat = gps.Satellites;

    printf("t = %u, lat = %d, lon = %d, sats = %u \n",
           gpsData.time, gpsData.lat, gpsData.lon, gpsData.sats);

    lrBeginPacket(&lr, false);
    lrWrite(&lr, &gpsData, sizeof(gps_t));
    lrEndPacket(&lr, false);
}

static bool hatIRQ(repeating_timer_t *rt) {
    tlAdd(&tl, hatTask, NULL);
    return true;
}

void hatInit() {
    setup_gps();

    spi_init(spi0, 1000 * 1000);

    gpio_set_function(2, GPIO_FUNC_SPI);
    gpio_set_function(3, GPIO_FUNC_SPI);
    gpio_set_function(4, GPIO_FUNC_SPI);

    lr.spi = spi0;
    lr.ss = 11;
    lr.reset = 10; // 29
    lr.frequency = 868000000;

    lrInit(&lr);

    add_repeating_timer_ms(1000, hatIRQ, NULL, &hatTimer);
}

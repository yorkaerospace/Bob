#include "hardware/flash.h"
#include "hardware/timer.h"
#include "hardware/spi.h"
#include <hardware/sync.h>
#include "LoRa-RP2040.h"

#include <pico/stdlib.h>
#include <string.h>
#include <stdio.h>

extern "C" {
#include "gps.h"
#include "types.h"
#include "taskList.h"

// Externs
extern taskList_t tl;
}
static repeating_timer_t hatTimer;
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
    uint8_t buf[sizeof(Packet)];

    // Get the bits we actually want.
    p.gps_tme = gps.SecondsInDay;
    p.gps_lat = gps.Latitude;
    p.gps_lng = gps.Longitude;
    p.gps_sat = gps.Satellites;

    printf("t = %u, lat = %d, lon = %d, sats = %u \n",
           p.gps_tme,p.gps_lat,p.gps_lng, p.gps_sat);

    memcpy(buf, &p, sizeof(Packet));

    LoRa.beginPacket();
    LoRa.write(buf, sizeof(gps_t));
    LoRa.endPacket();
}

static bool hatIRQ(repeating_timer_t *rt) {
    tlAdd(&tl, hatTask, NULL);
    return true;
}

extern "C" void hatInit() {
    setup_gps();

    spi_init(spi0, 1000 * 1000);

    gpio_set_function(2, GPIO_FUNC_SPI);
    gpio_set_function(3, GPIO_FUNC_SPI);
    gpio_set_function(4, GPIO_FUNC_SPI);

    LoRa.setSPI(*spi0);
    LoRa.setPins(28, 29);

    reset_gps();
    if(LoRa.begin(868E6)) {
        add_repeating_timer_ms(1000, hatIRQ, NULL, &hatTimer);
        LoRa.setTxPower(8);
    } else {
        while (true) {
            printf("well fuk");
        }
    }
}

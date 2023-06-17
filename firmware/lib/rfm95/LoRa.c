// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Port by Claire Hinton for York Aerospace & Rocketry

#include <LoRa.h>
#include "hardware/gpio.h"

// Just define some arduino stuff cause im lazy
#define LOW  0
#define HIGH 1
#define bitWrite(x, n, b) x = b ? x | 1UL << n : x & 1UL << n


// registers
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_PA_CONFIG            0x09
#define REG_OCP                  0x0b
#define REG_LNA                  0x0c
#define REG_FIFO_ADDR_PTR        0x0d
#define REG_FIFO_TX_BASE_ADDR    0x0e
#define REG_FIFO_RX_BASE_ADDR    0x0f
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13
#define REG_PKT_SNR_VALUE        0x19
#define REG_PKT_RSSI_VALUE       0x1a
#define REG_RSSI_VALUE           0x1b
#define REG_MODEM_CONFIG_1       0x1d
#define REG_MODEM_CONFIG_2       0x1e
#define REG_PREAMBLE_MSB         0x20
#define REG_PREAMBLE_LSB         0x21
#define REG_PAYLOAD_LENGTH       0x22
#define REG_MODEM_CONFIG_3       0x26
#define REG_FREQ_ERROR_MSB       0x28
#define REG_FREQ_ERROR_MID       0x29
#define REG_FREQ_ERROR_LSB       0x2a
#define REG_RSSI_WIDEBAND        0x2c
#define REG_DETECTION_OPTIMIZE   0x31
#define REG_INVERTIQ             0x33
#define REG_DETECTION_THRESHOLD  0x37
#define REG_SYNC_WORD            0x39
#define REG_INVERTIQ2            0x3b
#define REG_DIO_MAPPING_1        0x40
#define REG_VERSION              0x42
#define REG_PA_DAC               0x4d

// modes
#define MODE_LONG_RANGE_MODE     0x80
#define MODE_LRSLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_TX                  0x03
#define MODE_RX_CONTINUOUS       0x05
#define MODE_RX_SINGLE           0x06
#define MODE_CAD                 0x07

// PA config
#define PA_BOOST                 0x80

// IRQ masks
#define IRQ_TX_DONE_MASK           0x08
#define IRQ_PAYLOAD_CRC_ERROR_MASK 0x20
#define IRQ_RX_DONE_MASK           0x40
#define IRQ_CAD_DONE_MASK          0x04
#define IRQ_CAD_DETECTED_MASK      0x01

#define RF_MID_BAND_THRESHOLD    525E6
#define RSSI_OFFSET_HF_PORT      157
#define RSSI_OFFSET_LF_PORT      164

#define MAX_PKT_LENGTH           255

#if (ESP8266 || ESP32)
#define ISR_PREFIX ICACHE_RAM_ATTR
#else
#define ISR_PREFIX
#endif

static uint8_t lrSingleTransfer(lora_t * lr,uint8_t address, uint8_t value)
{
    uint8_t response;

    gpio_put(lr->ss, LOW);

    spi_write_read_blocking (lr->spi, &address, &response, 1);
    spi_write_read_blocking (lr->spi, &value, &response, 1);

    gpio_put(lr->ss, HIGH);

    return response;
}

static uint8_t lrReadRegister(lora_t * lr,uint8_t address)
{
    return lrSingleTransfer(lr, address & 0x7f, 0x00);
}

static void lrWriteRegister(lora_t * lr,uint8_t address, uint8_t value)
{
    lrSingleTransfer(lr, address | 0x80, value);
}

static void lrExplicitHeaderMode(lora_t * lr)
{
    lr->implicitHeaderMode = 0;

    lrWriteRegister(lr, REG_MODEM_CONFIG_1, lrReadRegister(lr, REG_MODEM_CONFIG_1) & 0xfe);
}

static void lrImplicitHeaderMode(lora_t * lr)
{
    lr->implicitHeaderMode = 1;

    lrWriteRegister(lr, REG_MODEM_CONFIG_1, lrReadRegister(lr, REG_MODEM_CONFIG_1) | 0x01);
}

int lrInit(lora_t * lr)
{

    // setup pins
    gpio_init(lr->ss);
    gpio_set_dir(lr->ss, GPIO_OUT);
    // set SS high
    gpio_put(lr->ss, HIGH);

    if (lr->reset != -1) {
        gpio_init(lr->reset);
        gpio_set_dir(lr->reset, GPIO_OUT);

        // perform reset
        gpio_put(lr->reset, LOW);
        sleep_ms(10);
        gpio_put(lr->reset, HIGH);
        sleep_ms(10);
    }

    lrSetFrequency(lr,lr->frequency);

    // check version
    uint8_t version = lrReadRegister(lr, REG_VERSION);
    if (version != 0x12) {
        return 0;
    }

    // set base addresses
    lrWriteRegister(lr, REG_FIFO_TX_BASE_ADDR, 0);
    lrWriteRegister(lr, REG_FIFO_RX_BASE_ADDR, 0);

    // set LNA boost
    lrWriteRegister(lr, REG_LNA, lrReadRegister(lr, REG_LNA) | 0x03);

    // set auto AGC
    lrWriteRegister(lr, REG_MODEM_CONFIG_3, 0x04);

    // set output power to 17 dBm
    lrSetTxPower(lr, 17, 1);

    // put in standby mode
    lrIdle(lr);

    return 1;
}

int lrBeginPacket(lora_t * lr,int implicitHeader)
{
    if (lrIsTransmitting(lr)) {
        return 0;
    }

    // put in standby mode
    lrIdle(lr);

    if (implicitHeader) {
        lrImplicitHeaderMode(lr);
    } else {
        lrExplicitHeaderMode(lr);
    }

    // reset FIFO address and paload length
    lrWriteRegister(lr, REG_FIFO_ADDR_PTR, 0);
    lrWriteRegister(lr, REG_PAYLOAD_LENGTH, 0);

    return 1;
}

int lrEndPacket(lora_t * lr,bool async)
{

    if ((async)) {
        lrWriteRegister(lr, REG_DIO_MAPPING_1, 0x40); // DIO0 => TXDONE
    }
    // put in TX mode
    lrWriteRegister(lr, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);

    if (!async) {
        // wait for TX done
        while ((lrReadRegister(lr, REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0) {
            return 0;
        }
        // clear IRQ's
        lrWriteRegister(lr, REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
    }

    return 1;
}

bool lrIsTransmitting(lora_t * lr)
{
    if ((lrReadRegister(lr, REG_OP_MODE) & MODE_TX) == MODE_TX) {
        return true;
    }

    if (lrReadRegister(lr, REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) {
        // clear IRQ's
        lrWriteRegister(lr, REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
    }

    return false;
}

int lrParsePacket(lora_t * lr,int size)
{
    int packetLength = 0;
    int irqFlags = lrReadRegister(lr, REG_IRQ_FLAGS);

    if (size > 0) {
        lrImplicitHeaderMode(lr);

        lrWriteRegister(lr, REG_PAYLOAD_LENGTH, size & 0xff);
    } else {
        lrExplicitHeaderMode(lr);
    }

    // clear IRQ's
    lrWriteRegister(lr, REG_IRQ_FLAGS, irqFlags);

    if ((irqFlags & IRQ_RX_DONE_MASK) && (irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0) {
        // received a packet
        lr->packetIndex = 0;

        // read packet length
        if (lr->implicitHeaderMode) {
            packetLength = lrReadRegister(lr, REG_PAYLOAD_LENGTH);
        } else {
            packetLength = lrReadRegister(lr, REG_RX_NB_BYTES);
        }

        // set FIFO address to current RX address
        lrWriteRegister(lr, REG_FIFO_ADDR_PTR, lrReadRegister(lr, REG_FIFO_RX_CURRENT_ADDR));

        // put in standby mode
        lrIdle(lr);
    } else if (lrReadRegister(lr, REG_OP_MODE) != (MODE_LONG_RANGE_MODE | MODE_RX_SINGLE)) {
        // not currently in RX mode

        // reset FIFO address
        lrWriteRegister(lr, REG_FIFO_ADDR_PTR, 0);

        // put in single RX mode
        lrWriteRegister(lr, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_SINGLE);
    }

    return packetLength;
}

int lrPacketRssi(lora_t * lr)
{
    return (lrReadRegister(lr, REG_PKT_RSSI_VALUE) - (lr->frequency < RF_MID_BAND_THRESHOLD ? RSSI_OFFSET_LF_PORT : RSSI_OFFSET_HF_PORT));
}

float lrPacketSnr(lora_t * lr)
{
    return ((int8_t)lrReadRegister(lr, REG_PKT_SNR_VALUE)) * 0.25;
}

long lrPacketFrequencyError(lora_t * lr)
{

    /* int32_t freqError = 0; */
    /* freqError = (int32_t)(lrReadRegister(lr, REG_FREQ_ERROR_MSB) & 0b111); */
    /* freqError <<= 8L; */
    /* freqError +=  (int32_t)(lrReadRegister(lr, REG_FREQ_ERROR_MID)); */
    /* freqError <<= 8L; */
    /* freqError += (lrReadRegister(lr, REG_FREQ_ERROR_LSB)); */

    /* if (lrReadRegister(lr, REG_FREQ_ERROR_MSB) & 0b1000) { // Sign bit is on */
    /*     freqError -= 524288; // 0b1000'0000'0000'0000'0000 */
    /* } */

    /* const float fXtal = 32E6; // FXOSC: crystal oscillator (XTAL) frequency (2.5. Chip Specification, p. 14) */
    /* const float fError = ((((float) freqError) * (1L << 24)) / fXtal) * (lrGetSignalBandwidth(lr) / 500000.0f); // p. 37 */

    /* return (fError); */
    return 0;
}

int lrRssi(lora_t * lr)
{
    return (lrReadRegister(lr, REG_RSSI_VALUE) - (lr->frequency < RF_MID_BAND_THRESHOLD ? RSSI_OFFSET_LF_PORT : RSSI_OFFSET_HF_PORT));
}

size_t lrWrite(lora_t * lr,const uint8_t *buffer, size_t size)
{
    int currentLength = lrReadRegister(lr, REG_PAYLOAD_LENGTH);

    // check size
    if ((currentLength + size) > MAX_PKT_LENGTH) {
        size = MAX_PKT_LENGTH - currentLength;
    }

    // write data
    for (size_t i = 0; i < size; i++) {
        lrWriteRegister(lr, REG_FIFO, buffer[i]);
    }

    // update length
    lrWriteRegister(lr, REG_PAYLOAD_LENGTH, currentLength + size);

    return size;
}

int lrAvailable(lora_t * lr)
{
    return (lrReadRegister(lr, REG_RX_NB_BYTES) - lr->packetIndex);
}

int lrRead(lora_t * lr)
{
    if (!lrAvailable(lr)) {
        return -1;
    }

    lr->packetIndex++;

    return lrReadRegister(lr, REG_FIFO);
}

int lrPeek(lora_t * lr)
{
    if (!lrAvailable(lr)) {
        return -1;
    }

    // store current FIFO address
    int currentAddress = lrReadRegister(lr, REG_FIFO_ADDR_PTR);

    // read
    uint8_t b = lrReadRegister(lr, REG_FIFO);

    // restore FIFO address
    lrWriteRegister(lr, REG_FIFO_ADDR_PTR, currentAddress);

    return b;
}

void lrIdle(lora_t * lr)
{
    lrWriteRegister(lr, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
}

void lrSleep(lora_t * lr)
{
    lrWriteRegister(lr, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_LRSLEEP);
}

void lrSetTxPower(lora_t * lr,int level, int outputPin)
{
    if (PA_OUTPUT_RFO_PIN == outputPin) {
        // RFO
        if (level < 0) {
            level = 0;
        } else if (level > 14) {
            level = 14;
        }

        lrWriteRegister(lr, REG_PA_CONFIG, 0x70 | level);
    } else {
        // PA BOOST
        if (level > 17) {
            if (level > 20) {
                level = 20;
            }

            // subtract 3 from level, so 18 - 20 maps to 15 - 17
            level -= 3;

            // High Power +20 dBm Operation (Semtech SX1276/77/78/79 5.4.3.)
            lrWriteRegister(lr, REG_PA_DAC, 0x87);
            lrSetOCP(lr, 140);
        } else {
            if (level < 2) {
                level = 2;
            }
            //Default value PA_HF/LF or +17dBm
            lrWriteRegister(lr, REG_PA_DAC, 0x84);
            lrSetOCP(lr, 100);
        }

        lrWriteRegister(lr, REG_PA_CONFIG, PA_BOOST | (level - 2));
    }
}

void lrSetFrequency(lora_t * lr,long frequency)
{
    lr->frequency = frequency;

    uint64_t frf = ((uint64_t)frequency << 19) / 32000000;

    lrWriteRegister(lr, REG_FRF_MSB, (uint8_t)(frf >> 16));
    lrWriteRegister(lr, REG_FRF_MID, (uint8_t)(frf >> 8));
    lrWriteRegister(lr, REG_FRF_LSB, (uint8_t)(frf >> 0));
}

int lrGetSpreadingFactor(lora_t * lr)
{
    return lrReadRegister(lr, REG_MODEM_CONFIG_2) >> 4;
}

/* void lrSetSpreadingFactor(lora_t * lr,int sf) */
/* { */
/*     if (sf < 6) { */
/*         sf = 6; */
/*     } else if (sf > 12) { */
/*         sf = 12; */
/*     } */

/*     if (sf == 6) { */
/*         lrWriteRegister(lr, REG_DETECTION_OPTIMIZE, 0xc5); */
/*         lrWriteRegister(lr, REG_DETECTION_THRESHOLD, 0x0c); */
/*     } else { */
/*         lrWriteRegister(lr, REG_DETECTION_OPTIMIZE, 0xc3); */
/*         lrWriteRegister(lr, REG_DETECTION_THRESHOLD, 0x0a); */
/*     } */

/*     lrWriteRegister(lr, REG_MODEM_CONFIG_2, (lrReadRegister(lr, REG_MODEM_CONFIG_2) & 0x0f) | ((sf << 4) & 0xf0)); */
/*     lrSetLdoFlag(lr); */
/* } */

long lrGetSignalBandwidth(lora_t * lr)
{
    uint8_t bw = (lrReadRegister(lr, REG_MODEM_CONFIG_1) >> 4);

    switch (bw) {
    case 0: return 7.8E3;
    case 1: return 10.4E3;
    case 2: return 15.6E3;
    case 3: return 20.8E3;
    case 4: return 31.25E3;
    case 5: return 41.7E3;
    case 6: return 62.5E3;
    case 7: return 125E3;
    case 8: return 250E3;
    case 9: return 500E3;
    }

    return -1;
}

/* void lrSetSignalBandwidth(lora_t * lr,long sbw) */
/* { */
/*     int bw; */

/*     if (sbw <= 7.8E3) { */
/*         bw = 0; */
/*     } else if (sbw <= 10.4E3) { */
/*         bw = 1; */
/*     } else if (sbw <= 15.6E3) { */
/*         bw = 2; */
/*     } else if (sbw <= 20.8E3) { */
/*         bw = 3; */
/*     } else if (sbw <= 31.25E3) { */
/*         bw = 4; */
/*     } else if (sbw <= 41.7E3) { */
/*         bw = 5; */
/*     } else if (sbw <= 62.5E3) { */
/*         bw = 6; */
/*     } else if (sbw <= 125E3) { */
/*         bw = 7; */
/*     } else if (sbw <= 250E3) { */
/*         bw = 8; */
/*     } else /\*if (sbw <= 250E3)*\/ { */
/*         bw = 9; */
/*     } */

/*     lrWriteRegister(lr, REG_MODEM_CONFIG_1, (lrReadRegister(lr, REG_MODEM_CONFIG_1) & 0x0f) | (bw << 4)); */
/*     lrSetLdoFlag(lr); */
/* } */


void lrSetCodingRate4(lora_t * lr,int denominator)
{
    if (denominator < 5) {
        denominator = 5;
    } else if (denominator > 8) {
        denominator = 8;
    }

    int cr = denominator - 4;

    lrWriteRegister(lr, REG_MODEM_CONFIG_1, (lrReadRegister(lr, REG_MODEM_CONFIG_1) & 0xf1) | (cr << 1));
}

void lrSetPreambleLength(lora_t * lr,long length)
{
    lrWriteRegister(lr, REG_PREAMBLE_MSB, (uint8_t)(length >> 8));
    lrWriteRegister(lr, REG_PREAMBLE_LSB, (uint8_t)(length >> 0));
}

void lrSetSyncWord(lora_t * lr,int sw)
{
    lrWriteRegister(lr, REG_SYNC_WORD, sw);
}

void lrEnableCrc(lora_t * lr)
{
    lrWriteRegister(lr, REG_MODEM_CONFIG_2, lrReadRegister(lr, REG_MODEM_CONFIG_2) | 0x04);
}

void lrDisableCrc(lora_t * lr)
{
    lrWriteRegister(lr, REG_MODEM_CONFIG_2, lrReadRegister(lr, REG_MODEM_CONFIG_2) & 0xfb);
}

void lrEnableInvertIQ(lora_t * lr)
{
    lrWriteRegister(lr, REG_INVERTIQ,  0x66);
    lrWriteRegister(lr, REG_INVERTIQ2, 0x19);
}

void lrDisableInvertIQ(lora_t * lr)
{
    lrWriteRegister(lr, REG_INVERTIQ,  0x27);
    lrWriteRegister(lr, REG_INVERTIQ2, 0x1d);
}

void lrSetOCP(lora_t * lr,uint8_t mA)
{
    uint8_t ocpTrim = 27;

    if (mA <= 120) {
        ocpTrim = (mA - 45) / 5;
    } else if (mA <=240) {
        ocpTrim = (mA + 30) / 10;
    }

    lrWriteRegister(lr, REG_OCP, 0x20 | (0x1F & ocpTrim));
}

void lrSetGain(lora_t * lr,uint8_t gain)
{
    // check allowed range
    if (gain > 6) {
        gain = 6;
    }

    // set to standby
    lrIdle(lr);

    // set gain
    if (gain == 0) {
        // if gain = 0, enable AGC
        lrWriteRegister(lr, REG_MODEM_CONFIG_3, 0x04);
    } else {
        // disable AGC
        lrWriteRegister(lr, REG_MODEM_CONFIG_3, 0x00);

        // clear Gain and set LNA boost
        lrWriteRegister(lr, REG_LNA, 0x03);

        // set gain
        lrWriteRegister(lr, REG_LNA, lrReadRegister(lr, REG_LNA) | (gain << 5));
    }
}

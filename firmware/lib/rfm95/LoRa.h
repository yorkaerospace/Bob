// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef LORA_H
#define LORA_H

#include "hardware/spi.h"

#if defined(ARDUINO_SAMD_MKRWAN1300)
#define LORA_DEFAULT_SPI           SPI1
#define LORA_DEFAULT_SPI_FREQUENCY 200000
#define LORA_DEFAULT_SS_PIN        LORA_IRQ_DUMB
#define LORA_DEFAULT_RESET_PIN     -1
#define LORA_DEFAULT_DIO0_PIN      -1
#elif defined(ARDUINO_SAMD_MKRWAN1310)
#define LORA_DEFAULT_SPI           SPI1
#define LORA_DEFAULT_SPI_FREQUENCY 200000
#define LORA_DEFAULT_SS_PIN        LORA_IRQ_DUMB
#define LORA_DEFAULT_RESET_PIN     -1
#define LORA_DEFAULT_DIO0_PIN      LORA_IRQ
#else
#define LORA_DEFAULT_SPI           SPI
#define LORA_DEFAULT_SPI_FREQUENCY 8E6
#define LORA_DEFAULT_SS_PIN        10
#define LORA_DEFAULT_RESET_PIN     9
#define LORA_DEFAULT_DIO0_PIN      2
#endif

#define PA_OUTPUT_RFO_PIN          0
#define PA_OUTPUT_PA_BOOST_PIN     1

typedef struct {
    spi_inst_t * spi;
    int ss;
    int reset;
    int dio0;
    long frequency;
    int packetIndex;
    int implicitHeaderMode;
} lora_t;

int lrInit(lora_t * lr);

int lrBeginPacket(lora_t * lr, int implicitHeader);
int lrEndPacket(lora_t * lr, bool async);

int lrParsePacket(lora_t * lr, int size);
int lrPacketRssi(lora_t * lr);
float lrPacketSnr(lora_t * lr);
long lrPacketFrequencyError(lora_t * lr);

int lrRssi(lora_t * lr);

size_t lrWrite(lora_t * lr, const uint8_t *buffer, size_t size);

void lrIdle(lora_t * lr);
void lrSleep(lora_t * lr);

void lrSetTxPower(lora_t * lr, int level, int outputPin);
void lrSetFrequency(lora_t * lr, long frequency);
void lrSetSpreadingFactor(lora_t * lr, int sf);
void lrSetSignalBandwidth(lora_t * lr, long sbw);
void lrSetCodingRate4(lora_t * lr, int denominator);
void lrSetPreambleLength(lora_t * lr, long length);
void lrSetSyncWord(lora_t * lr, int sw);
void lrEnableCrc(lora_t * lr);
void lrDisableCrc(lora_t * lr);
void lrEnableInvertIQ(lora_t * lr);
void lrDisableInvertIQ(lora_t * lr);
bool lrIsTransmitting(lora_t * lr);

void lrSetOCP(lora_t * lr, uint8_t mA); // Over Current Protection control

void lrSetGain(lora_t * lr, uint8_t gain); // Set LNA gain

#endif

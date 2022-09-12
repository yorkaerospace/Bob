/* Header file containing bob-specific constants 
 * This overrides a lot of things that the SDK sets by default 
 * You dont have to include this in any code you write; the SDK
 * should include it by default. */
#ifndef BOB_H
#define BOB_H

// For board detection
#define YAR_BOB

// Internal I2C bus
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C 0
#endif

#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN 16
#endif

#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN 17
#endif

// Size of the flash storage in bytes
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (64 * 1024 * 1024)
#endif

// Buzzer
#define BOB_BUZZER 7

#endif

/* Header file containing Bob-specific constants.
 * By rights this should be in the SDK, to add board specific support, but that
 * requires forking the SDK and remembering to add flags into CMake and thats a
 * whole faff I don't want to deal with.
 *
 * If I wanted to be clever, I could add some preprocessor stuff here to be able
 * to build for multiple revs of board, but that's not really needed atm.
 */
#ifndef BOB_H
#define BOB_H

// I2C Stuff
#define BOB_DEFAULT_SDA 16
#define BOB_DEFAULT_SCL 17

// Buzzer
#define BOB_BUZZER 7

// Size of the flash storage in bytes
#define BOB_STORAGE (64*1024*1024)

#endif

/* A system for handling commands sent over USB.
 * Only capable of interpreting single char commands.
 * This isnt ideal, but users shouldnt have to interact with this.
 *
 * I cant seem to find a way to put an interrupt on STDIN, so we have to poll
 * To avoid making the IRQ too big this can only process 30 chars a second. */

#ifndef USBCMD_H
#define USBCMD_H

#include <stdint.h>

#define USB_POLLING_FREQ 30

/* Initialises the USB command handler
 * Returns: 0 on success
 * 1 if USB command handler is already running
 * 2 on other failure */
uint8_t usbCmdInit();

/* Kills the command handler
 * Returns 0 on success
 * 1 if the command handler was not running
 * 2 on other failure */
uint8_t usbCmdKill();

/* Gets the status of the command handler
 * Returns 0 if not running
 * 1 if running */
uint8_t usbCmdStatus();
#endif

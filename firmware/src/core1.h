#ifndef CORE1_H
#define CORE1_H

/* A simple loop that runs on core 1. Polls the sensors and passes
 * pointers to data into the FIFO buffer. */
void core1Entry(void);

#endif

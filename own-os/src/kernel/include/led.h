#ifndef LED_H
#define LED_H

#include "driver.h"

// number of FPGAIO leds
#define FPGAIO_LEDS 2

// number of FPGAIO buttons
#define FPGAIO_BUTTONS 2

// writes to board FGPA led
void led_write(void *ctx, int idx, int val);

// reads from board FPGA button
int button_read(void *ctx, int idx);

// FPGA device driver
extern bitwise_driver fpgaio_driver;

#endif

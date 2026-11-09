#ifndef GPIO_H
#define GPIO_H

#include "driver.h"

// number of GPIO_PINS
#define GPIO_PINS 16

// initializes board GPIO
void gpio_init(void *ctx);

// writes to board GPIO
void gpio_write(void *ctx, int idx, int val);

// reads from board GPIO
int gpio_read(void *ctx, int idx);

// sets board GPIO pin mode
void gpio_pin_mode(void *ctx, int idx, int mode);

// GPIO device drivers
extern bitwise_driver gpio0_driver;
extern bitwise_driver gpio1_driver;
extern bitwise_driver gpio2_driver;
extern bitwise_driver gpio3_driver;

#endif

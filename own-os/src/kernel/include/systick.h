#ifndef SYSTICK_H
#define SYSTICK_H

#include "driver.h"
#include <stdint.h>

// CPU clock frequency
#define CPU_CLOCK_FREQ 25000000UL // 25 MHz

// initializes SysTick
void systick_init(void* ctx, int period_ms);

// gets current uptime ms
unsigned int systick_tick(void* ctx);

// timer device driver
extern timer_driver systick_driver;

#endif

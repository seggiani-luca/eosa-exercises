#ifndef TIMER_H
#define TIMER_H

#include "driver.h"

// initialize the timer
void timer_init(void *ctx, int period_ms);

// gets current uptime ms
unsigned int timer_tick(void *ctx);

// timer device drivers
extern timer_driver timer0_driver;
extern timer_driver timer1_driver;

#endif

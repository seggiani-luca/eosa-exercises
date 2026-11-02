#ifndef TIMER_H
#define TIMER_H

#include "delay.h"
#include "driver.h"

// initialize the timer
void timer_init(void *ctx, int period_ms);

// gets current uptime ms
unsigned int timer_tick(void *ctx);

// timer device drivers
extern timer_driver timer0_driver;
extern timer_driver timer1_driver;

// timer device delay queue
extern timer_queue timer0_queue;
extern timer_queue timer1_queue;

#endif

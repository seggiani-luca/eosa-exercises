#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#define MAX_TICKS 10

void timer0_init(uint32_t period_ms);
void timer0_stop();

#endif

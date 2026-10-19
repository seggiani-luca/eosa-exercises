#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "cmsdk.h"

#define MAX_TICKS 10

void timer_init(CMSDK_TIMER_TypeDef* timer, int irq, int prio, uint32_t period_ms);

#endif

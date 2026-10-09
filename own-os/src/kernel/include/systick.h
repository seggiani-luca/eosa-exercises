#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

// CPU clock frequency
#define CPU_CLOCK_FREQ 25000000UL // 25 MHz

// SysTick period
#define TICK_TIME 10 // 10 ms

// initializes SysTick
void systick_init();

// gets current uptime ms
uint32_t systick_uptime_ms();

#endif

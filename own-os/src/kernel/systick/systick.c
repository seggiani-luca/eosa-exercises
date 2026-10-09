#include "../include/systick.h"
#include "../include/cmsdk.h"

// current uptime
static volatile uint32_t uptime_ms;

void systick_init() {
	uptime_ms = 0;

	SysTick->LOAD = CPU_CLOCK_FREQ / (1000 / TICK_TIME) - 1;

	SysTick->VAL = 0;
	SysTick->CTRL =
		SysTick_CTRL_ENABLE | SysTick_CTRL_TICKINT | SysTick_CTRL_CLKSOURCE;
}

uint32_t systick_uptime_ms() { return uptime_ms; }

void SysTick_Handler(void) { uptime_ms += TICK_TIME; }

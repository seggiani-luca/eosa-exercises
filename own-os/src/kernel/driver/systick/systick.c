#include "../../include/systick.h"
#include "../../include/cmsdk.h"

timer_driver systick_driver = {
	.init = systick_init,
	.tick = systick_tick,
};

// current ticks
static volatile unsigned int ticks;
static int period;

void systick_init(void *ctx, int period_ms) {
	ticks = 0;
	period = period_ms;

	SysTick->LOAD = CPU_CLOCK_FREQ / (1000 / period_ms) - 1;

	SysTick->VAL = 0;
	SysTick->CTRL =
		SysTick_CTRL_ENABLE | SysTick_CTRL_TICKINT | SysTick_CTRL_CLKSOURCE;
}

unsigned int systick_tick(void *ctx) { return ticks * period; }

void SysTick_Handler(void) { ticks++; }

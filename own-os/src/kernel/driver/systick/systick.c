#include "../../include/systick.h"
#include "../../include/cmsdk.h"
#include "../../include/pendsv.h"
#include "../../include/process.h"

timer_driver systick_driver = {
	.init = systick_init,
	.tick = systick_tick,
};

timer_queue systick_queue;

// current ticks
volatile unsigned int ticks = 0;
int period;

// quantum counter
int quantum = 0;

void systick_init(void *ctx, int period_ms) {
	ticks = 0;
	period = period_ms;

	SysTick->LOAD = CPU_CLOCK_FREQ / (1000 / period_ms) - 1;

	SysTick->VAL = 0;
	SysTick->CTRL =
		SysTick_CTRL_ENABLE | SysTick_CTRL_TICKINT | SysTick_CTRL_CLKSOURCE;
}

unsigned int systick_tick(void *ctx) { return ticks * period; }

void SysTick_Handler(void) {
	// advance timer
	ticks++;
	timer_queue_advance(&systick_queue, period);

	// advance round-robin quantum
	quantum += period;
	if (quantum >= QUANTUM) {
		quantum = 0;
		pendsv_request();
	}
}

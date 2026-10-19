#include "../../include/timer.h"
#include "../../include/cmsdk.h"
#include "../../include/watchdog.h"

// timer handling block
typedef struct {
	// timer
	CMSDK_TIMER_TypeDef *timer;

	// IRQ number
	int irq;

	// tick period
	int period;

	// current ticks
	int ticks;
} timer_block;

// timer blocks
static timer_block timer0 = {.timer = CMSDK_TIMER0, .irq = TIMER0_IRQn};
static timer_block timer1 = {.timer = CMSDK_TIMER1, .irq = TIMER1_IRQn};

// drivers
timer_driver timer0_driver = {
	.init = timer_init, .tick = timer_tick, .ctx = &timer0};
timer_driver timer1_driver = {
	.init = timer_init, .tick = timer_tick, .ctx = &timer1};

void timer_init(void *ctx, int period_ms) {
	timer_block *bl = ctx;

	bl->ticks = 0;
	bl->period = period_ms;

	// stop timer
	bl->timer->CTRL = 0;

	// enable NVIC interrupts
	NVIC->IPR[bl->irq] = NVIC_TIMER_PRIO;
	NVIC->ISER[bl->irq / 32] = 1UL << (bl->irq % 32);

	// load period
	uint32_t reload = 25000U * period_ms;
	bl->timer->RELOAD = reload;
	bl->timer->VALUE = reload;

	// start timer
	bl->timer->CTRL = TIMER_CTRL_EN | TIMER_CTRL_IRQEN;
}

unsigned int timer_tick(void *ctx) {
	timer_block *bl = ctx;

	return bl->ticks * bl->period;
}

void TIMER0_Handler(void) {
	CMSDK_TIMER0->INTSTATUS = TIMER_INT_PENDING;

	timer0.ticks++;
}

void TIMER1_Handler(void) {
	CMSDK_TIMER1->INTSTATUS = TIMER_INT_PENDING;

	timer1.ticks++;

	// feed watchdog
	watchdog_feed();
}

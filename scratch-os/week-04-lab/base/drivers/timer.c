#include "timer.h"
#include "cmsdk.h"
#include "kernel/kprintf.h"

void timer0_init(uint32_t period_ms) {
	CMSDK_TIMER0->CTRL = 0;
	
	NVIC->IPR[TIMER0_IRQn] = 0xC0;
    NVIC->ISER[TIMER0_IRQn / 32] = 1UL << ( TIMER0_IRQn % 32 );

    uint32_t reload = 25000U * period_ms;
    CMSDK_TIMER0->RELOAD = reload;
    CMSDK_TIMER0->VALUE = reload;
	CMSDK_TIMER0->CTRL = TIMER_CTRL_EN | TIMER_CTRL_IRQEN;
}

static int ticks = 0;

void TIMER0_Handler( void ) {
	CMSDK_TIMER0->INTSTATUS = TIMER_INT_PENDING;
	
	ticks++;
	if(ticks % MAX_TICKS == 0) {
		kprintf("timer: %d\n", ticks);
	}
}

#include "timer.h"
#include "kernel/kprintf.h"

void timer_init(CMSDK_TIMER_TypeDef* timer, int irq, int prio, uint32_t period_ms) {
	timer->CTRL = 0;
	
	NVIC->IPR[irq] = prio;
    NVIC->ISER[irq / 32] = 1UL << ( irq % 32 );

    uint32_t reload = 25000U * period_ms;
    timer->RELOAD = reload;
    timer->VALUE = reload;
	timer->CTRL = TIMER_CTRL_EN | TIMER_CTRL_IRQEN;
}

static int ticks = 0;
static volatile int in_slow = 0;

void TIMER0_Handler( void ) {
	CMSDK_TIMER0->INTSTATUS = TIMER_INT_PENDING;
	
	ticks++;
	if(ticks % MAX_TICKS == 0) {
		kprintf("T0 %s\n", in_slow ? "(in T1)" : "");
	}
}

void TIMER1_Handler( void ) {
	CMSDK_TIMER1->INTSTATUS = TIMER_INT_PENDING;
	
	in_slow = 1;
	kprintf("T1 start\n");

	for (volatile uint32_t i = 0; i < 100000; i++);

	in_slow = 0;
}

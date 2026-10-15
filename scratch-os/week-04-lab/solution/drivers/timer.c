/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * TIMER0 driver: see timer.h.
 */
#include "drivers/timer.h"
#include "cmsdk.h"
#include "kernel/kprintf.h"

/* The timer counts down at the processor clock: 25 000 cycles per ms. */
#define CYCLES_PER_MS 25000UL

/* Ticks since timer0_init. volatile: the handler changes it. */
static volatile uint32_t ticks;

void timer0_init( uint32_t period_ms )
{
    /* The value the counter reloads when it reaches 0. */
    CMSDK_TIMER0->RELOAD = CYCLES_PER_MS * period_ms;
    /* Count, and raise the interrupt at each reload. */
    CMSDK_TIMER0->CTRL = TIMER_CTRL_EN | TIMER_CTRL_IRQEN;
    /* Same priority as the UART, then the NVIC enable. */
    NVIC->IPR[TIMER0_IRQn] = 0xC0;
    NVIC->ISER[TIMER0_IRQn / 32] = 1UL << ( TIMER0_IRQn % 32 );
}

void TIMER0_Handler( void )
{
    /* Acknowledge at the device (write 1 to clear). */
    CMSDK_TIMER0->INTSTATUS = TIMER_INT_PENDING;

    ticks++;
    /* Every 10th tick, i.e. once per second with a 100 ms period. */
    if ( ticks % 10 == 0 )
    {
        kprintf( "timer0: %u\n", ticks );
    }
}

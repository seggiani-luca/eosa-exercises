/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * SysTick driver: see systick.h.
 */

#include "drivers/systick.h"
#include "cmsdk.h"

/* The speed of the processor clock, in cycles per second. */
#define CPU_CLOCK_HZ 25000000UL

/* Milliseconds since start. volatile: the handler changes it while
 * the main code reads it. Not static: src/task_b.c pokes it on purpose. */
volatile uint32_t uptime_ms;

void systick_init( void )
{
    uptime_ms = 0;
    /* LOAD + 1 cycles per tick: 250000 cycles for TICK_MS = 10. */
    SysTick->LOAD = CPU_CLOCK_HZ / ( 1000 / TICK_MS ) - 1;
    /* Writing any value to VAL sets the counter to 0. */
    SysTick->VAL = 0;
    /* Start counting, raise the exception at each wrap, use the core clock. */
    SysTick->CTRL = SysTick_CTRL_ENABLE | SysTick_CTRL_TICKINT | SysTick_CTRL_CLKSOURCE;
}

void SysTick_Handler( void )
{
    /* One more tick has passed. SysTick needs no acknowledge. */
    uptime_ms += TICK_MS;
}

uint32_t systick_uptime_ms( void )
{
    /* A single aligned 32-bit read cannot be torn by the interrupt. */
    return uptime_ms;
}

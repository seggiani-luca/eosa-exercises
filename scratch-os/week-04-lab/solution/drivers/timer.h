/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * TIMER0 driver: a periodic interrupt that counts its ticks.
 */
#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* Start TIMER0 so that its interrupt fires every period_ms milliseconds. */
void timer0_init( uint32_t period_ms );

/* The TIMER0 interrupt handler. It is named in the vector table. */
void TIMER0_Handler( void );

#endif /* TIMER_H */

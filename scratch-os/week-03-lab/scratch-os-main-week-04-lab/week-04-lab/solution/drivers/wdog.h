/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Watchdog driver: a timer that resets the board unless it is fed.
 */
#ifndef WDOG_H
#define WDOG_H

/* Start the watchdog with a 1 s period. Call it from privileged code. */
void wdog_init( void );

/* Restart the watchdog's count ("feed" it). */
void wdog_feed( void );

/* The watchdog interrupt arrives as NMI, which nothing can mask. */
void NMI_Handler( void );

#endif /* WDOG_H */

/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Watchdog driver: see wdog.h.
 */
#include "drivers/wdog.h"
#include "cmsdk.h"
#include "kernel/kprintf.h"

void wdog_init( void )
{
    /* The registers ignore writes while locked: unlock first. */
    CMSDK_WDOG->LOCK = WDOG_UNLOCK_KEY;
    CMSDK_WDOG->LOAD = 25000000UL;                  /* 1 s at 25 MHz */
    CMSDK_WDOG->CTRL = WDOG_CTRL_INTEN | WDOG_CTRL_RESEN;
    CMSDK_WDOG->LOCK = 0;                           /* any other value locks */
}

void wdog_feed( void )
{
    CMSDK_WDOG->LOCK = WDOG_UNLOCK_KEY;
    CMSDK_WDOG->INTCLR = 1;                         /* restart the count */
    CMSDK_WDOG->LOCK = 0;
}

void NMI_Handler( void )
{
    kprintf( "watchdog: NMI\n" );
}

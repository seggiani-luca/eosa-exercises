/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * GPIO driver: see gpio.h.
 */
#include "drivers/gpio.h"
#include "cmsdk.h"

void gpio_set_output( unsigned pin )
{
    /* A set register: a 1 changes its pin, a 0 leaves the others alone. */
    CMSDK_GPIO0->OUTENABLESET = 1UL << pin;
}

void gpio_write( unsigned pin, int level )
{
    /* DATAOUT is a plain register: read-modify-write. */
    if ( level )
    {
        CMSDK_GPIO0->DATAOUT |= 1UL << pin;
    }
    else
    {
        CMSDK_GPIO0->DATAOUT &= ~( 1UL << pin );
    }
}

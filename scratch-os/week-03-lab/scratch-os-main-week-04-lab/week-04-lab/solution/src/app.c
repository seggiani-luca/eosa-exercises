/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * The application. It runs unprivileged: it may not touch the drivers
 * or any hardware register. Everything it needs it asks the kernel for,
 * through the system calls (lib/syscalls.h) and the helpers built on
 * them (lib/printf.h).
 *
 * What it does, forever:
 *   - once per second, print the uptime;
 *   - if a key was pressed, print it;
 *   - once per second, alternate the board LEDs;
 *   - sleep 10 ms between checks.
 */

#include <stdint.h>
#include "src/app.h"
#include "lib/syscalls.h"
#include "lib/printf.h"

void app_main( void )
{
    printf( "app: hello from unprivileged code\n" );

    /* When the uptime was last printed. We compare against the clock
     * instead of counting loop turns: each turn takes 10 ms plus the
     * time spent printing, so counting turns would drift. */
    uint32_t last = 0;

    for ( ;; )
    {
        uint32_t now = sys_uptime();
        /* "Has one second passed since last?" Unsigned subtraction gives
         * the elapsed time even when the millisecond count wraps around
         * from 0xFFFFFFFF back to 0. */
        if ( now - last >= 1000 )
        {
            printf( "uptime: %u ms\n", now );
            last += 1000;       /* the second we just reported */
            sys_led( ( now / 1000 ) % 2 ? 1 : 2 );   /* alternate the LEDs */
        }

        /* Was a key pressed? sys_read never waits: -1 means no. */
        int c = sys_read();
        if ( c != -1 )
        {
            printf( "key: %c\n", c );
        }

        /* Let the CPU sleep until the next check. */
        sleep_ms( 10 );
    }
}

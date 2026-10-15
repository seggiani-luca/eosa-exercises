/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Task B. It runs unprivileged: week 3's application. Forever, once
 * per second it prints the uptime, and it echoes any key pressed.
 *
 * It can also misbehave on purpose, to show what each protection
 * layer does. Pick one at build time:
 *     make run DEMO=rogue | badptr | overflow
 */

#include <stdint.h>
#include "src/tasks.h"
#include "lib/syscalls.h"
#include "lib/printf.h"

/* How many seconds this task has reported. A global on purpose: from
 * Unit 2 on, the linker places it inside task B's partition. */
static uint32_t b_seconds = 0;

#if defined( DEMO_ROGUE ) || defined( DEMO_BADPTR )
/* The kernel's own uptime counter (drivers/systick.c). Task B should
 * never touch it: it belongs to the kernel. */
extern volatile uint32_t uptime_ms;
#endif

#if defined( DEMO_OVERFLOW )
/* Recurse 4 levels deep (depth 0 to 3). Each call's 1 KB local array
 * makes ~4 KB in all, more than task B's 1 KB stack, so the stack runs
 * past the bottom of the partition into the RAM below. */
#define EAT_DEPTH 3
static uint32_t eat_stack( uint32_t depth )
{
    volatile uint8_t pad[ 1024 ];           /* a big local array */
    pad[ 0 ] = (uint8_t) depth;             /* touch it so it is not optimised away */
    if ( depth >= EAT_DEPTH )
    {
        return pad[ 0 ];                    /* deep enough: stop */
    }
    return eat_stack( depth + 1 ) + pad[ 0 ];   /* go one level deeper */
}
#endif

void task_b( void )
{
    printf( "B: hello from task B\n" );

#if defined( DEMO_ROGUE )
    /* Rogue write: overwrite the kernel's uptime counter. */
    printf( "B: writing to the kernel's uptime counter\n" );
    uptime_ms = 0x12345678;                 /* a store into kernel memory */
#endif

#if defined( DEMO_BADPTR )
    /* Bad pointer: ask the kernel to print 16 bytes that are in kernel
     * memory. The kernel runs privileged, so it could do it. */
    int r = sys_write( (const char *) &uptime_ms, 16 );
    printf( "\nB: sys_write returned %d\n", r );
#endif

#if defined( DEMO_OVERFLOW )
    /* Stack overflow: recurse until the stack leaves the partition. */
    printf( "B: recursing\n" );
    printf( "B: result %u\n", eat_stack( 0 ) );
#endif

    /* When the uptime was last printed. Compare against the clock, not
     * loop turns: counting turns would drift. Start from now, not from 0. */
    uint32_t last = sys_uptime();

    for ( ;; )
    {
        uint32_t now = sys_uptime();
        /* "Has one second passed since last?" Unsigned subtraction gives
         * the elapsed time even when the millisecond count wraps around
         * from 0xFFFFFFFF back to 0. */
        if ( now - last >= 1000 )
        {
            b_seconds++;                    /* one more second reported */
            printf( "B: uptime %u ms (second %u)\n", now, b_seconds );
            last += 1000;                   /* the second we just reported */
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

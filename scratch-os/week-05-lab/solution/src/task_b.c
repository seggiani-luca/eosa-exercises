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
 *     make run DEMO=rogue | badptr | overflow | cross | share | wrap | exec
 */

#include <stdint.h>
#include "src/tasks.h"
#include "lib/syscalls.h"
#include "lib/printf.h"

/* How many seconds this task has reported. A global on purpose: the
 * linker script puts every variable of this file inside task B's
 * partition. */
static uint32_t b_seconds = 0;

#if defined( DEMO_ROGUE ) || defined( DEMO_BADPTR )
/* The kernel's own uptime counter (drivers/systick.c). Task B should
 * never touch it: it belongs to the kernel. */
extern volatile uint32_t uptime_ms;
#endif

#if defined( DEMO_CROSS )
/* The first variable of task A, in task A's partition. */
extern uint32_t _task_a_data_start[];
#endif

#if defined( DEMO_SHARE )
/* The shared window: the top 256 bytes of task A's partition. */
extern char _shared_start[];
#endif

#if defined( DEMO_WRAP )
/* The first address after this task's partition. */
extern uint32_t _task_b_part_end[];
#endif

#if defined( DEMO_OVERFLOW )
/* Recurse 4 levels deep (depth 0 to 3). Each call uses about 1 KB of
 * stack for its local array, so about 4 KB in all: 3 KB more than the
 * 1 KB stack has. The stack runs past the bottom of task B's partition
 * into the RAM below it. 4 levels are enough to show this, and few
 * enough to stay inside RAM. (The limit also lets the compiler see that
 * the recursion ends; without one, it warns about endless recursion.) */
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

#if defined( DEMO_CROSS )
    /* Cross-task store: write into task A's partition, then read it
     * back. Task A has already exited; its memory is still there. */
    printf( "B: writing into task A's partition\n" );
    volatile uint32_t *a_var = _task_a_data_start;
    *a_var = 0xDEADBEEF;                    /* a store into another task's memory */
    printf( "B: read back 0x%x\n", *a_var );
#endif

#if defined( DEMO_WRAP )
    /* Tail pointer: a buffer that starts 16 bytes before the end of this
     * partition, with a length so large that start + length wraps past
     * zero. The kernel must refuse it. */
    printf( "B: sys_write with a wrapping length\n" );
    int t = sys_write( (const char *) ( (uint32_t) _task_b_part_end - 16 ), 0xFFFFFFF0u );
    printf( "\nB: sys_write returned %d\n", t );
#endif

#if defined( DEMO_EXEC )
    /* Data run as code: put two Thumb instructions in a local array,
     * `movs r0, #42` and `bx lr`, and call the array as a function.
     * (The low bit of the address selects Thumb state.) */
    printf( "B: running code from the stack\n" );
    volatile uint16_t code[ 2 ] = { 0x202A, 0x4770 };
    int ( *injected )( void ) = ( int ( * )( void ) )( ( (uint32_t) code ) | 1u );
    printf( "B: injected code returned %d\n", injected() );
#endif

#if defined( DEMO_SHARE )
    /* The shared window: read it, ask the kernel to print from it,
     * then try to write it. */
    printf( "B: reading the window: %s\n", _shared_start );
    int w = sys_write( _shared_start, 5 );
    printf( "\nB: sys_write returned %d\n", w );
    printf( "B: writing the window\n" );
    _shared_start[ 0 ] = 'J';               /* a store into the window */
    printf( "B: window written\n" );
#endif

    /* When the uptime was last printed. We compare against the clock
     * instead of counting loop turns: each turn takes 10 ms plus the
     * time spent printing, so counting turns would drift. Task B
     * starts after task A, so we start from the time now, not from 0. */
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

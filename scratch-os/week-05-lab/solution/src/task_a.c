/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Task A. It runs unprivileged, so it only uses system calls
 * (lib/syscalls.h) and the helpers built on them (lib/printf.h).
 * It prints a greeting, then its uptime three times, one second
 * apart, and returns.
 */

#include <stdint.h>
#include "src/tasks.h"
#include "lib/syscalls.h"
#include "lib/printf.h"

#if defined( DEMO_SHARE )
/* The shared window: the top 256 bytes of this task's partition
 * (scripts/mps2_m3.ld). */
extern char _shared_start[];
#endif

/* How many lines this task has printed. A global on purpose: the linker
 * script puts every variable of this file inside task A's partition. */
static uint32_t a_lines = 0;

void task_a( void )
{
#if defined( DEMO_SHARE )
    /* Publish a message in the window, with its NUL. */
    const char *msg = "hello";
    for ( int k = 0; k < 6; k++ )
    {
        _shared_start[ k ] = msg[ k ];
    }
#endif

    printf( "A: hello from task A\n" );
    a_lines++;                              /* one line printed */

    for ( int i = 0; i < 3; i++ )
    {
        printf( "A: uptime %u ms (line %u)\n", sys_uptime(), a_lines );
        a_lines++;                          /* one more line printed */
        sleep_ms( 1000 );                   /* wait one second */
    }
}

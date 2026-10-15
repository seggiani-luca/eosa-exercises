/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * System calls, kernel side: run the service that `svc #n` asked for
 * and hand its result back to the caller. Runs privileged, inside the
 * SVCall exception.
 */

#include <stdint.h>
#include "kernel/syscall.h"
#include "lib/syscalls.h"
#include "drivers/uart.h"
#include "drivers/systick.h"
#include "kernel/task.h"

/* A service takes up to three arguments (the caller's r0, r1, r2) and
 * returns one result (it becomes the caller's r0). */
typedef uint32_t ( *syscall_fn )( uint32_t a0, uint32_t a1, uint32_t a2 );

/* Is [p, p + len) inside [start, end)? Used by the check in sys_write
 * (Unit 3). Written as `len <= end - p`, which cannot wrap around. */
static __attribute__(( unused )) int in_range( uint32_t p, uint32_t len, uint32_t start, uint32_t end )
{
    return p >= start && p <= end && len <= end - p;
}

/* SYS_WRITE: a0 = buf, a1 = len. Copy as many bytes as fit in the UART
 * send buffer and return the count (0..len). It never waits for room:
 * we are inside SVCall, and the UART interrupt that empties the buffer
 * cannot run until we return, so waiting would freeze the system. */
static uint32_t sys_write_impl( uint32_t a0, uint32_t a1, uint32_t a2 )
{
    (void) a2;                                  /* not used */
    const char *buf = ( const char * ) a0;      /* the number is really an address */
    unsigned len = a1;

    /* >>> UNIT 3: check that [buf, buf + len) is allowed (in_range). <<< */

    return uart_write( buf, len );              /* bytes accepted, 0..len */
}

/* SYS_READ: no arguments. Return one received byte, or -1 if none. */
static uint32_t sys_read_impl( uint32_t a0, uint32_t a1, uint32_t a2 )
{
    (void) a0;                                  /* not used */
    (void) a1;
    (void) a2;
    /* -1 becomes 0xFFFFFFFF here; the caller's int turns it back into -1. */
    return ( uint32_t ) uart_getc_polled();
}

/* SYS_UPTIME: no arguments. Return the milliseconds since start. */
static uint32_t sys_uptime_impl( uint32_t a0, uint32_t a1, uint32_t a2 )
{
    (void) a0;                                  /* not used */
    (void) a1;
    (void) a2;
    return systick_uptime_ms();
}

/* Dispatch table: table[n] is the function for service n. Entry 0 is
 * empty because it is not a service. */
static const syscall_fn table[] = {
    [SYS_WRITE]  = sys_write_impl,
    [SYS_READ]   = sys_read_impl,
    [SYS_UPTIME] = sys_uptime_impl,
};

/* Number of table entries (numbers 0..3). SYS_EXIT and SYS_START_TASKS
 * are not in the table: svc_dispatch handles them, since they do not
 * return to the caller. */
#define TABLE_SIZE ( sizeof( table ) / sizeof( table[0] ) )

/* EXC_RETURN bit 2: 1 = the caller was a task (PSP), 0 = main() (MSP). */
#define EXC_RETURN_PSP_BIT 0x4u

/* frame = the 8 registers the hardware pushed on `svc`: frame[0] = r0,
 * frame[6] = pc (the instruction after `svc`). exc_return = the EXC_RETURN
 * of this exception. Returns the EXC_RETURN that SVC_Handler must use. */
uint32_t svc_dispatch( uint32_t *frame, uint32_t exc_return )
{
    /* Read the service number n: it is the low byte of the `svc #n`
     * instruction, which sits 2 bytes before the saved pc (frame[6]). */
    uint8_t n = ( ( uint8_t * ) frame[6] )[-2];

    /* Did a task ask (process stack), or main() (main stack)? */
    int from_task = ( exc_return & EXC_RETURN_PSP_BIT ) != 0;

    /* SYS_EXIT: only a task may exit. Start the next task, or go back to main(). */
    if ( n == SYS_EXIT && from_task )
    {
        return task_exit_current();     /* next task, or back to main() */
    }

    /* SYS_START_TASKS: kernel only. A task calling it gets -1 from the table below. */
    if ( n == SYS_START_TASKS && !from_task && task_current() == 0 )
    {
        return task_start_first();      /* enter the first task */
    }

    /* Unknown number, 0, or empty table entry: answer -1. */
    if ( n >= TABLE_SIZE || table[n] == 0 )
    {
        frame[0] = ( uint32_t ) -1;
        return exc_return;              /* back to the caller */
    }

    /* Run the service with the caller's r0, r1, r2, and store its result
     * in the saved r0: the hardware pops it into the caller's r0. */
    frame[0] = table[n]( frame[0], frame[1], frame[2] );
    return exc_return;                  /* back to the caller, as it came */
}

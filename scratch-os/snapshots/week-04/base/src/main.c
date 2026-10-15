/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Kernel start: Reset_Handler (boot/startup.s) calls main(). main()
 * starts the devices, then asks the kernel (one `svc`) to run the tasks.
 */

#include "drivers/uart.h"
#include "drivers/systick.h"
#include "kernel/kprintf.h"
#include "kernel/mpu.h"

int main( void )
{
    /* 1. The console first, so everything after this can print. */
    uart_init();

    /* 2. Say we are alive. kprintf works without interrupts. */
    kprintf( "scratch-os: kernel started\n" );

    /* 3. Start the 10 ms clock tick (needed by sys_uptime and sleep_ms). */
    systick_init();

    /* 4. Set up the MPU (its body is still empty: the MPU stays off). */
    mpu_init();

    /* 5. Start the tasks: `svc` with the kernel-only SYS_START_TASKS (5).
     * The kernel runs task A, then B, and returns here after the last one.
     * C equivalent:
     *     svc(SYS_START_TASKS);   // runs all tasks, then comes back
     */
    __asm volatile (
        "svc #5             \n" /* svc(SYS_START_TASKS) */
        : : : "memory" );

    /* 6. Every task has exited. */
    kprintf( "all tasks finished\n" );

    /* 7. Nothing left to do: sleep until the next interrupt, forever. */
    for ( ;; )
    {
        __asm volatile ( "wfi" );   /* wait for interrupt */
    }
}

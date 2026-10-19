/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Kernel start. Reset_Handler (boot/startup.s) has already set up the
 * CPU and the memory, and calls main(). main() starts the devices,
 * gives up privilege, and hands the CPU to the application.
 */

#include "drivers/uart.h"
#include "drivers/timer.h"
#include "drivers/systick.h"
#include "kernel/kprintf.h"
#include "src/app.h"

int main( void )
{
    /* 1. The console first, so everything after this can print. */
    uart_init();

    /* 2. Say we are alive. kprintf works without interrupts. */
    kprintf( "scratch-os: kernel started\n" );

    /* 3. Start the 10 ms clock tick (needed by sys_uptime and sleep_ms). */
    systick_init();

	timer_init(CMSDK_TIMER0, TIMER0_IRQn, 0xE0, 100);
	timer_init(CMSDK_TIMER1, TIMER1_IRQn, 0x40, 1000);

    kprintf( "dropping to unprivileged mode\n" );

    /* 4. Drop to unprivileged mode by setting bit 0 (nPRIV) of the
     * CONTROL register. From now on this code can no longer touch the
     * hardware registers: it must ask the kernel with `svc`.
     * C equivalent (C has no way to name CONTROL directly):
     *     CONTROL = CONTROL | 1;   // nPRIV = 1: unprivileged from now on
     */
    __asm volatile (
        "mrs r0, control    \n" /* r0 = CONTROL (read) */
        "orr r0, r0, #1     \n" /* r0 |= 1: set nPRIV (modify) */
        "msr control, r0    \n" /* CONTROL = r0 (write): now unprivileged */
        "isb                \n" /* flush the pipeline: the CPU may have already
                                  * fetched later instructions under the old,
                                  * privileged rules; isb throws those away
                                  * and re-fetches under the new ones. */
        : : : "r0", "memory" );

    /* 5. Run the application. Exceptions (SysTick, UART, SVCall) still
     * run privileged: the hardware raises privilege on entry and drops
     * it again on return. */
    app_main();

    /* Not reached: app_main() never returns. If it ever did, stop here. */
    for ( ;; )
    {
    }
}

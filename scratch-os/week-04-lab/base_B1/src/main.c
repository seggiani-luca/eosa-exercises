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
#include "kernel/irq.h"

#include <stdint.h>

volatile uint32_t counter;
volatile uint32_t isr_hits;

int main( void )
{
    /* 1. The console first, so everything after this can print. */
    uart_init();

    /* 2. Say we are alive. kprintf works without interrupts. */
    kprintf( "scratch-os: kernel started\n" );

    /* 3. Start the 10 ms clock tick (needed by sys_uptime and sleep_ms). */
    systick_init();

	timer0_init(1);

	for(int i = 0; i < 100000; i++) {
		uint32_t saved = irq_save();

		uint32_t tmp = counter;
		for(volatile int j = 0; j < 1000; j++);
		counter = tmp + 1;

		irq_restore(saved);
	}

	timer0_stop();

	kprintf("counter: %d, isr_hits: %d\n", counter, isr_hits);
	kprintf("diff: %d\n", counter - 100000 - isr_hits);

    /* Not reached: app_main() never returns. If it ever did, stop here. */
    for ( ;; )
    {
    }
}

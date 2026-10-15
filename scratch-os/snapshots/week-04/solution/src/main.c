/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Kernel start. Reset_Handler (boot/startup.s) has already set up the
 * CPU and the memory, and calls main(). main() starts the devices,
 * then asks the kernel (with one `svc`) to run the tasks. Each task
 * runs unprivileged, on its own stack, in its own partition
 * (kernel/task.c). When the last task has finished, main() continues.
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

    /* 5. Start the tasks. Only an exception return can enter a task
     * (it is the only way to switch to the process stack and drop
     * privilege in one step), so main() raises an exception: `svc`
     * with the kernel-only number SYS_START_TASKS (5, lib/syscalls.h).
     * The kernel starts task A, then B when A exits, and so on. After
     * the last task it returns here, just after the `svc`, privileged
     * again and on the main stack.
     * C equivalent (pseudo-C):
     *     svc(SYS_START_TASKS);   // runs all tasks, then comes back
     * r4-r11 hold the same values when we come back: each task is a C
     * function, which gives them back unchanged before it returns. */
    __asm volatile (
        "svc #5             \n" /* svc(SYS_START_TASKS) */
        : : : "memory" );

    /* 6. Every task has exited. */
    kprintf( "all tasks finished\n" );

    /* 7. Nothing left to do: sleep until the next interrupt, forever.
     * SysTick and the UART interrupts keep working. */
    for ( ;; )
    {
        __asm volatile ( "wfi" );   /* wait for interrupt */
    }
}

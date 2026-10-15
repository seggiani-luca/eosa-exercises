/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Tasks, kernel side (see kernel/task.h). Runs inside SVCall, privileged,
 * on the main stack. Tasks run one after the other: A, then B when A
 * calls exit. There is no switching back and forth between tasks.
 */

#include <stdint.h>
#include "kernel/task.h"
#include "kernel/mpu.h"
#include "kernel/kprintf.h"
#include "src/tasks.h"
#include "lib/syscalls.h"

/* Addresses from the linker script. Declared as arrays: only their
 * address matters, there is no variable behind the name. */
extern uint32_t _task_a_part_start[], _task_a_stack_top[], _task_a_part_end[];
extern uint32_t _task_a_data_start[], _task_a_data_end[];
extern const uint32_t _task_a_data_load[];
extern uint32_t _task_b_part_start[], _task_b_stack_top[], _task_b_part_end[];
extern uint32_t _task_b_data_start[], _task_b_data_end[];
extern const uint32_t _task_b_data_load[];

/* Task table, in run order. */
static const struct task tasks[] = {
    {
        .name       = "A",
        .entry      = task_a,
        .stack_top  = _task_a_stack_top,
        .part_start = ( uint32_t ) _task_a_part_start,
        .part_end   = ( uint32_t ) _task_a_part_end,
        .data_start = _task_a_data_start,
        .data_end   = _task_a_data_end,
        .data_load  = _task_a_data_load,
    },
    {
        .name       = "B",
        .entry      = task_b,
        .stack_top  = _task_b_stack_top,
        .part_start = ( uint32_t ) _task_b_part_start,
        .part_end   = ( uint32_t ) _task_b_part_end,
        .data_start = _task_b_data_start,
        .data_end   = _task_b_data_end,
        .data_load  = _task_b_data_load,
    },
};

/* Number of tasks (here 2). */
#define NUM_TASKS ( sizeof( tasks ) / sizeof( tasks[0] ) )

/* Index of the running task; -1 while the kernel runs alone. */
static int current = -1;

/* EXC_RETURN values: where the exception return goes back to. */
#define EXC_RETURN_THREAD_PSP 0xFFFFFFFDu   /* thread mode, process stack (a task) */
#define EXC_RETURN_THREAD_MSP 0xFFFFFFF9u   /* thread mode, main stack (main) */

/* xPSR with only the T bit (Thumb state) set: the CPU faults without it. */
#define XPSR_THUMB 0x01000000u

/* Copy the task's initial variable values from FLASH into its RAM partition. */
void partition_init( const struct task *t )
{
    const uint32_t *src = t->data_load;     /* initial values, in FLASH */
    uint32_t *dst = t->data_start;          /* the variables, in RAM */

    /* The linker stores .data and .bss together in FLASH (.bss as zeros),
     * so this one loop sets every variable, zeros included. */
    while ( dst < t->data_end )
    {
        *dst++ = *src++;                    /* copy one word, move both on */
    }
}

/* The running task, or 0 while the kernel runs alone. */
const struct task *task_current( void )
{
    if ( current < 0 )
    {
        return 0;                           /* the kernel runs alone */
    }
    return &tasks[current];
}

/* Start task i inside SVCall. The task is not jumped to directly: we
 * prepare its stack and MPU region, and return the EXC_RETURN that
 * enters it. */
static uint32_t task_launch( int i )
{
    const struct task *t = &tasks[i];

    current = i;                            /* this task now runs */
    kprintf( "starting task %s\n", t->name );

    /* MPU region 1 = this task's partition (empty body until Unit 3). */
    mpu_set_task_region( t->part_start, t->part_end - t->part_start );

    /* First run: give its variables their initial values. */
    partition_init( t );

    /* Build the 8-word frame the hardware pops on exception return, at the
     * top of the task's stack. The stack grows down, so it starts 8 words below. */
    uint32_t *frame = t->stack_top - 8;
    frame[0] = 0;                           /* r0 */
    frame[1] = 0;                           /* r1 */
    frame[2] = 0;                           /* r2 */
    frame[3] = 0;                           /* r3 */
    frame[4] = 0;                           /* r12 */
    /* lr: where the task goes when its function returns: task_exit, i.e. exit. */
    frame[5] = ( uint32_t ) task_exit;
    /* pc: the task's entry point, with bit 0 cleared (a real instruction address). */
    frame[6] = ( uint32_t ) t->entry & ~1u;
    frame[7] = XPSR_THUMB;                  /* xPSR: only the Thumb bit */

    /* Set PSP to the frame, then set nPRIV (CONTROL bit 0) so thread mode
     * runs unprivileged. Handler mode is always privileged, so nPRIV takes
     * effect only when the exception returns to the task.
     * C equivalent (C cannot name PSP or CONTROL):
     *     PSP = frame;
     *     CONTROL = CONTROL | 1;   // nPRIV = 1
     */
    __asm volatile (
        "msr psp, %0        \n" /* PSP = frame: the task's stack pointer */
        "mrs r1, control    \n" /* r1 = CONTROL (read) */
        "orr r1, r1, #1     \n" /* r1 |= 1: set nPRIV (modify) */
        "msr control, r1    \n" /* CONTROL = r1 (write) */
        "isb                \n" /* make the new CONTROL take effect now */
        : : "r" ( frame ) : "r1", "memory" );

    /* Return to thread mode on PSP: the hardware pops the frame and the
     * task starts at frame[6], with nPRIV set. */
    return EXC_RETURN_THREAD_PSP;
}

uint32_t task_start_first( void )
{
    return task_launch( 0 );                /* the first task in the table */
}

/* Print the exit message, then start the next task if there is one. The
 * exited task's frame is left behind on its stack. */
uint32_t task_exit_current( void )
{
    kprintf( "task %s exited\n", tasks[current].name );

    /* Is there another task after this one? Then start it. */
    if ( current + 1 < ( int ) NUM_TASKS )
    {
        return task_launch( current + 1 );
    }

    /* No more tasks: go back to main(), which waits just after its own
     * `svc` on the main stack. Clear nPRIV first so main() runs privileged.
     * C equivalent:
     *     CONTROL = CONTROL & ~1;   // nPRIV = 0
     */
    current = -1;                           /* no task runs any more */
    __asm volatile (
        "mrs r1, control    \n" /* r1 = CONTROL (read) */
        "bic r1, r1, #1     \n" /* r1 &= ~1: clear nPRIV (modify) */
        "msr control, r1    \n" /* CONTROL = r1 (write) */
        "isb                \n" /* make the new CONTROL take effect now */
        : : : "r1", "memory" );

    /* Return to thread mode on MSP: the hardware pops main()'s frame and
     * main() continues after its `svc`. */
    return EXC_RETURN_THREAD_MSP;
}

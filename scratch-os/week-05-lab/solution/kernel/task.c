/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Tasks, kernel side: see kernel/task.h.
 *
 * Everything here runs inside the SVCall exception (privileged, on the
 * main stack). The tasks run one after the other: A first, then B when
 * A calls exit. There is no switching back and forth between tasks.
 */

#include <stdint.h>
#include "kernel/task.h"
#include "kernel/mpu.h"
#include "kernel/kprintf.h"
#include "src/tasks.h"
#include "lib/syscalls.h"

/* Addresses defined by the linker script. Declared as arrays: only
 * their address matters, there is no variable behind the name. */
extern uint32_t _task_a_part_start[], _task_a_stack_top[], _task_a_part_end[];
extern uint32_t _task_a_data_start[], _task_a_data_end[];
extern const uint32_t _task_a_data_load[];
extern uint32_t _task_b_part_start[], _task_b_stack_top[], _task_b_part_end[];
extern uint32_t _task_b_data_start[], _task_b_data_end[];
extern const uint32_t _task_b_data_load[];

/* The task table: the tasks, in the order they run. */
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

/* How many tasks the table has (here 2). */
#define NUM_TASKS ( sizeof( tasks ) / sizeof( tasks[0] ) )

/* Index of the task that runs now; -1 while no task runs. */
static int current = -1;

/* EXC_RETURN values (week 3): the value that ends an exception also
 * says where to go back to. */
#define EXC_RETURN_THREAD_PSP 0xFFFFFFFDu   /* thread mode, process stack (a task) */
#define EXC_RETURN_THREAD_MSP 0xFFFFFFF9u   /* thread mode, main stack (main) */

/* xPSR with only bit 24 (T, Thumb state) set. The CPU can only run
 * Thumb code: popping an xPSR without this bit would fault. */
#define XPSR_THUMB 0x01000000u

void partition_init( const struct task *t )
{
    const uint32_t *src = t->data_load;     /* initial values, in FLASH */
    uint32_t *dst = t->data_start;          /* the variables, in RAM */

    /* Copy word by word. The linker script stores .data and .bss
     * together in FLASH (the .bss part as zeros), so this one loop
     * sets every variable of the task, zeros included. */
    while ( dst < t->data_end )
    {
        *dst++ = *src++;                    /* copy one word, move both on */
    }
}

const struct task *task_current( void )
{
    if ( current < 0 )
    {
        return 0;                           /* the kernel runs alone */
    }
    return &tasks[current];
}

/* Start task i. Called inside the SVCall exception. It does not jump to
 * the task: it prepares everything so that the exception return lands
 * in the task, and returns the EXC_RETURN value that does it. */
static uint32_t task_launch( int i )
{
    const struct task *t = &tasks[i];

    current = i;                            /* from now on, this task runs */
    kprintf( "starting task %s\n", t->name );

    /* MPU region 1 = this task's partition, so the task may use it.
     * (The body of mpu_set_task_region is still empty: MPU off.) */
    /* >>> MPU: program the task region here <<< */
    mpu_set_task_region( t->part_start, t->part_end - t->part_start );

    /* Each task is started only once, so this is its first run:
     * give its variables their initial values. */
    partition_init( t );

    /* Build, at the top of the task's stack, the 8 words the hardware
     * pops on exception return (the same layout it pushes on entry).
     * The stack grows down, so the frame starts 8 words below the top. */
    uint32_t *frame = t->stack_top - 8;
    frame[0] = 0;                           /* r0 */
    frame[1] = 0;                           /* r1 */
    frame[2] = 0;                           /* r2 */
    frame[3] = 0;                           /* r3 */
    frame[4] = 0;                           /* r12 */
    /* lr: where the task goes when its function returns. task_exit
     * (lib/syscalls.s) calls sys_exit, so returning means exit. */
    frame[5] = ( uint32_t ) task_exit;
    /* pc: where the task starts. A C function address has bit 0 set
     * (it means "Thumb code"); the saved pc must be a real instruction
     * address, so bit 0 is cleared. */
    frame[6] = ( uint32_t ) t->entry & ~1u;
    frame[7] = XPSR_THUMB;                  /* xPSR: only the Thumb bit */

    /* PSP = frame, then set nPRIV (bit 0 of CONTROL). We are in handler
     * mode, which is always privileged, so nPRIV only takes effect when
     * the exception returns to thread mode, that is, in the task.
     * C equivalent (C cannot name PSP or CONTROL):
     *     PSP = frame;
     *     CONTROL = CONTROL | 1;   // nPRIV = 1: thread mode is unprivileged
     */
    __asm volatile (
        "msr psp, %0        \n" /* PSP = frame: the task's stack pointer */
        "mrs r1, control    \n" /* r1 = CONTROL (read) */
        "orr r1, r1, #1     \n" /* r1 |= 1: set nPRIV (modify) */
        "msr control, r1    \n" /* CONTROL = r1 (write) */
        "isb                \n" /* make the new CONTROL take effect now */
        : : "r" ( frame ) : "r1", "memory" );

    /* Return to thread mode on the process stack: the hardware pops our
     * frame from PSP, and the task starts at frame[6] with CONTROL = 3
     * (nPRIV = 1, and SPSEL = 1 set by this EXC_RETURN). */
    return EXC_RETURN_THREAD_PSP;
}

uint32_t task_start_first( void )
{
    return task_launch( 0 );                /* the first task in the table */
}

uint32_t task_exit_current( void )
{
    kprintf( "task %s exited\n", tasks[current].name );

    /* Is there another task after this one? Then start it. The frame of
     * the task that exited is simply left behind on its stack. */
    if ( current + 1 < ( int ) NUM_TASKS )
    {
        return task_launch( current + 1 );
    }

    /* That was the last task. Go back to main(), which is still waiting
     * just after its own `svc`, with its frame on the main stack.
     * First clear nPRIV, so main() runs privileged again.
     * C equivalent:
     *     CONTROL = CONTROL & ~1;   // nPRIV = 0: thread mode is privileged
     */
    current = -1;                           /* no task runs any more */
    __asm volatile (
        "mrs r1, control    \n" /* r1 = CONTROL (read) */
        "bic r1, r1, #1     \n" /* r1 &= ~1: clear nPRIV (modify) */
        "msr control, r1    \n" /* CONTROL = r1 (write) */
        "isb                \n" /* make the new CONTROL take effect now */
        : : : "r1", "memory" );

    /* Return to thread mode on the main stack: the hardware pops main()'s
     * frame from MSP, and main() continues after its `svc`. */
    return EXC_RETURN_THREAD_MSP;
}

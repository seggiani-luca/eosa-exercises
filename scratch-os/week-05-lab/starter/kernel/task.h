/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Tasks, kernel side: the task table, and the code that starts a task
 * and ends it when it calls exit.
 *
 * A task always runs unprivileged, in thread mode, on its own stack
 * (the process stack, PSP), inside its own RAM partition. The only way
 * into a task is an exception return: the kernel builds the frame the
 * hardware would pop, and returns from the SVCall exception into it.
 */
#ifndef TASK_H
#define TASK_H

#include <stdint.h>

/* Everything the kernel knows about one task. All addresses come from
 * the linker script (scripts/mps2_m3.ld). */
struct task {
    const char *name;           /* short name for messages, e.g. "A" */
    void ( *entry )( void );    /* the function the task runs */
    uint32_t *stack_top;        /* first address above the task's stack */
    uint32_t part_start;        /* lowest address of the partition */
    uint32_t part_end;          /* first address after the partition */
    uint32_t *data_start;       /* the task's variables in RAM: start... */
    uint32_t *data_end;         /* ...and end (exclusive) */
    const uint32_t *data_load;  /* their initial values, stored in FLASH */
};

/* Copy the initial values of t's variables from FLASH into its
 * partition. Done once, before the task runs for the first time. */
void partition_init( const struct task *t );

/* The task that runs now, or 0 (no task) while the kernel runs alone. */
const struct task *task_current( void );

/* Kernel-only service: start the first task. Called from svc_dispatch
 * when main() asks with `svc`. Returns the EXC_RETURN value that enters
 * the task (0xFFFFFFFD). */
uint32_t task_start_first( void );

/* The current task has called exit. Print a message and start the next
 * task. Returns the EXC_RETURN value to use: 0xFFFFFFFD to enter the
 * next task, or 0xFFFFFFF9 to go back to main() after the last one. */
uint32_t task_exit_current( void );

#endif /* TASK_H */

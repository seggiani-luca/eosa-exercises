/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Tasks, kernel side: the task table, and the code that starts a task
 * and ends it when it calls exit. Tasks run unprivileged, on their own
 * stack (PSP), inside their own RAM partition. The only way in is an
 * exception return.
 */
#ifndef TASK_H
#define TASK_H

#include <stdint.h>

/* What the kernel knows about one task. Addresses come from the linker script. */
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

/* Copy the initial values of t's variables from FLASH into its partition.
 * Done once, before the task runs for the first time. */
void partition_init( const struct task *t );

/* The running task, or 0 while the kernel runs alone. */
const struct task *task_current( void );

/* Kernel-only: start the first task (called from svc_dispatch when main()
 * asks with `svc`). Returns the EXC_RETURN that enters the task. */
uint32_t task_start_first( void );

/* The current task called exit: print a message and start the next task.
 * Returns the EXC_RETURN to use: enter the next task, or go back to main()
 * after the last one. */
uint32_t task_exit_current( void );

#endif /* TASK_H */

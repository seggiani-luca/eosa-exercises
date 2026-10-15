/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * The two tasks: the code that runs unprivileged, after the kernel
 * has started.
 */
#ifndef TASKS_H
#define TASKS_H

/* Task A: prints a greeting and its uptime three times, then returns. */
void task_a( void );

/* Task B: prints the uptime every second and echoes keys. Never returns. */
void task_b( void );

#endif /* TASKS_H */

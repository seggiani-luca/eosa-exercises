/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * User side of the system calls: the functions declared in
 * lib/syscalls.h. Each is `svc #n` followed by `bx lr`. The arguments
 * are already in r0-r2, and the kernel leaves the result in the saved
 * r0, so `bx lr` returns it to the C caller unchanged.
 *
 * The svc numbers must match lib/syscalls.h (SYS_WRITE = 1, SYS_READ = 2,
 * SYS_UPTIME = 3, SYS_EXIT = 4).
 */

/* Same assembler settings as boot/startup.s. */
    .syntax unified             /* modern ARM assembly syntax */
    .cpu cortex-m3              /* only instructions this CPU really has */
    .thumb                      /* Thumb code (the only kind Cortex-M runs) */

    .section .text,"ax",%progbits   /* code goes into the normal code section */

/* ------------------------------------------------------------------ */
/* int sys_write(const char *buf, unsigned len)                        */
/* ------------------------------------------------------------------ */
/* buf arrives in r0, len in r1. The accepted count comes back in r0.
 * C equivalent (pseudo-C, since C cannot write `svc`):
 *     int sys_write(const char *buf, unsigned len) {
 *         return svc(SYS_WRITE, buf, len);   // the kernel does the work
 *     }
 */
    .global sys_write           /* visible to C files */
    .type sys_write, %function  /* it is code */
    .thumb_func                 /* it is Thumb code (bit 0 set in its address) */
sys_write:
    svc #1                      /* svc(SYS_WRITE, ...): buf, len already in r0, r1 */
    bx lr                       /* return the result, which the kernel put in r0 */
    .size sys_write, . - sys_write /* record its size in bytes */

/* ------------------------------------------------------------------ */
/* int sys_read(void)                                                  */
/* ------------------------------------------------------------------ */
/* No arguments. The byte (or -1) comes back in r0.
 * C equivalent (pseudo-C):
 *     int sys_read(void) {
 *         return svc(SYS_READ);              // the kernel does the work
 *     }
 */
    .global sys_read            /* visible to C files */
    .type sys_read, %function   /* it is code */
    .thumb_func                 /* it is Thumb code (bit 0 set in its address) */
sys_read:
    svc #2                      /* svc(SYS_READ) */
    bx lr                       /* return the result, which the kernel put in r0 */
    .size sys_read, . - sys_read /* record its size in bytes */

/* ------------------------------------------------------------------ */
/* uint32_t sys_uptime(void)                                           */
/* ------------------------------------------------------------------ */
/* No arguments. The milliseconds come back in r0.
 * C equivalent (pseudo-C):
 *     uint32_t sys_uptime(void) {
 *         return svc(SYS_UPTIME);            // the kernel does the work
 *     }
 */
    .global sys_uptime          /* visible to C files */
    .type sys_uptime, %function /* it is code */
    .thumb_func                 /* it is Thumb code (bit 0 set in its address) */
sys_uptime:
    svc #3                      /* svc(SYS_UPTIME) */
    bx lr                       /* return the result, which the kernel put in r0 */
    .size sys_uptime, . - sys_uptime /* record its size in bytes */

/* ------------------------------------------------------------------ */
/* void sys_exit(void)                                                 */
/* ------------------------------------------------------------------ */
/* No arguments, no result: the kernel never comes back here. It starts
 * the next task instead, on that task's own stack.
 * C equivalent (pseudo-C):
 *     void sys_exit(void) {
 *         svc(SYS_EXIT);                     // the kernel ends this task
 *         for (;;) { }                       // never reached
 *     }
 */
    .global sys_exit            /* visible to C files */
    .type sys_exit, %function   /* it is code */
    .thumb_func                 /* it is Thumb code (bit 0 set in its address) */
sys_exit:
    svc #4                      /* svc(SYS_EXIT) */
    b sys_exit                  /* never reached; if it were, ask again */
    .size sys_exit, . - sys_exit /* record its size in bytes */

/* ------------------------------------------------------------------ */
/* void task_exit(void)                                                */
/* ------------------------------------------------------------------ */
/* Where a task goes when its function returns. The kernel puts the
 * address of task_exit in the lr of the frame it builds to start a
 * task (kernel/task.c), so the task's final `bx lr` jumps here.
 * C equivalent:
 *     void task_exit(void) {
 *         sys_exit();                        // the task is finished
 *     }
 */
    .global task_exit           /* visible to C files (the kernel takes its address) */
    .type task_exit, %function  /* it is code */
    .thumb_func                 /* it is Thumb code (bit 0 set in its address) */
task_exit:
    bl sys_exit                 /* sys_exit(); it does not come back */
    b task_exit                 /* never reached; if it were, try again */
    .size task_exit, . - task_exit /* record its size in bytes */

/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * SVC_Handler: entry point of every system call (`svc #n`). It passes
 * the registers the hardware saved to svc_dispatch() (kernel/syscall.c),
 * then ends the exception with the EXC_RETURN value that svc_dispatch returns.
 */

/* Same assembler settings as boot/startup.s. */
    .syntax unified             /* modern ARM assembly syntax */
    .cpu cortex-m3              /* only instructions this CPU really has */
    .thumb                      /* Thumb code (the only kind Cortex-M runs) */

    .section .text,"ax",%progbits   /* code goes into the normal code section */

    .global SVC_Handler         /* visible to the linker: the vector table uses it */
    .type SVC_Handler, %function /* it is code */
    .thumb_func                 /* it is Thumb code (bit 0 set in its address) */
SVC_Handler:
    /* Pick the stack that holds the frame, from bit 2 of EXC_RETURN (set = task). */
    tst lr, #4                  /* flags = lr & 4: is bit 2 set? */
    /* `ite eq`: the next instruction runs if Z = 1, the one after if Z = 0. */
    ite eq                      /* if ((lr & 4) == 0) ... else ... */
    mrseq r0, msp               /*     frame = MSP;  // came from main() */
    mrsne r0, psp               /* else frame = PSP;  // came from a task */
    /* r0 is the first argument of a C function (AAPCS), r1 the second. */
    mov r1, lr                  /* exc_return = lr: second argument */
    /* bl overwrites lr, but EXC_RETURN is already saved in r1. */
    bl svc_dispatch             /* r0 = svc_dispatch(frame, exc_return) */
    /* Jumping to the returned EXC_RETURN ends the exception: the hardware
     * pops 8 registers from the stack it names, and resumes. */
    bx r0                       /* return_from_exception(r0) */
    .size SVC_Handler, . - SVC_Handler /* record its size in bytes */

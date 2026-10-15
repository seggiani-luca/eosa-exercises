/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * SVC_Handler: the entry point of every system call (`svc #n`).
 * It does only one thing: hand the registers saved by the hardware to
 * the C function svc_dispatch() in kernel/syscall.c.
 *
 * What the hardware does when `svc` runs (exception entry):
 *   1. it pushes 8 registers onto the stack, in this order of
 *      increasing address:
 *          frame[0] = r0     frame[4] = r12
 *          frame[1] = r1     frame[5] = lr   (the caller's lr)
 *          frame[2] = r2     frame[6] = pc   (where to resume afterwards)
 *          frame[3] = r3     frame[7] = xPSR (status flags)
 *      These are exactly the registers the AAPCS lets a function
 *      destroy (r0-r3, r12) plus what is needed to resume (lr, pc,
 *      xPSR). The stack used is the main stack, the only one this
 *      kernel uses, whose pointer is MSP (main stack pointer). After
 *      the push, MSP points at frame[0]. (The CPU also has a second
 *      stack pointer, PSP; this kernel never uses it.)
 *   2. it puts a special value in lr, called EXC_RETURN. Here it is
 *      0xFFFFFFF9, which means "return to thread mode (normal code),
 *      using the main stack". Jumping to this value with `bx lr` is
 *      what ends the exception: the hardware pops the 8 registers back
 *      and resumes at frame[6].
 *   3. it jumps to SVC_Handler (word 11 of the vector table).
 *
 * C equivalent (pseudo-C, since C cannot name MSP):
 *     void SVC_Handler(void) {
 *         svc_dispatch((uint32_t *)MSP);   // frame = what the hardware pushed
 *     }
 *
 * Why not write SVC_Handler directly in C? A C function may start by
 * pushing registers (for example lr, or r4-r7 it wants to use) onto the
 * stack. That moves SP away from the hardware frame before our code
 * can read it, and how far it moves depends on the compiler and on the
 * optimization level. Two instructions of assembly, run before
 * anything else touches the stack, read MSP while it still points
 * exactly at frame[0].
 *
 * This definition is strong: it replaces the weak SVC_Handler alias in
 * boot/startup.s.
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
    /* r0 is the first argument of a C function (AAPCS), so putting
     * the frame address in r0 makes it the `frame` argument. */
    mrs r0, msp                 /* frame = MSP: address of the 8 saved registers */
    /* `b` (branch) instead of `bl` (branch with link): `bl` would
     * overwrite lr with a return address here, losing EXC_RETURN.
     * With `b`, lr still holds EXC_RETURN, so when svc_dispatch ends
     * with its normal `bx lr` it does not come back here: the CPU sees
     * EXC_RETURN and returns from the exception. */
    b svc_dispatch              /* svc_dispatch(frame); its return ends the exception */
    .size SVC_Handler, . - SVC_Handler /* record its size in bytes */

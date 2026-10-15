/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * irq_save / irq_restore: mask interrupts around a short piece of code
 * and put the mask back exactly as it was. Declared for C in kernel/irq.h.
 * They use PRIMASK, which C cannot name, so they are written in assembly.
 */

/* Same assembler settings as boot/startup.s. */
    .syntax unified             /* modern ARM assembly syntax */
    .cpu cortex-m3              /* only instructions this CPU really has */
    .thumb                      /* Thumb code (the only kind Cortex-M runs) */

    .section .text,"ax",%progbits   /* code goes into the normal code section */

/* ------------------------------------------------------------------ */
/* uint32_t irq_save(void)                                             */
/* ------------------------------------------------------------------ */
/* Return the old PRIMASK in r0, then mask interrupts.
 * C equivalent:
 *     uint32_t irq_save(void) {
 *         uint32_t old = PRIMASK;  // remember the current state
 *         PRIMASK = 1;             // mask interrupts
 *         return old;
 *     }
 */
    .global irq_save            /* visible to C files */
    .type irq_save, %function   /* it is code */
    .thumb_func                 /* it is Thumb code (bit 0 set in its address) */
irq_save:
    mrs r0, primask             /* old = PRIMASK; r0 is also the return value */
    cpsid i                     /* PRIMASK = 1: interrupts masked */
    bx lr                       /* return old (already in r0) to the caller */
    .size irq_save, . - irq_save /* record its size in bytes */

/* ------------------------------------------------------------------ */
/* void irq_restore(uint32_t old)                                      */
/* ------------------------------------------------------------------ */
/* Write `old` (arriving in r0) back into PRIMASK.
 * C equivalent:
 *     void irq_restore(uint32_t old) {
 *         PRIMASK = old;           // masked again only if it was before
 *     }
 */
    .global irq_restore         /* visible to C files */
    .type irq_restore, %function /* it is code */
    .thumb_func                 /* it is Thumb code (bit 0 set in its address) */
irq_restore:
    msr primask, r0             /* PRIMASK = old (old arrived in r0) */
    bx lr                       /* return to the caller */
    .size irq_restore, . - irq_restore /* record its size in bytes */

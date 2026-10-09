/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * WEEK 2 LAB SOLUTION - not a checkpoint. Nothing later in the course
 * builds on this folder.
 *
 * lab A3: the hosted half of the swap exercise. This program targets
 * the same Cortex-M3, under the same QEMU command, as the firmware
 * image. It is linked as a HOSTED program (ISO C section 4), so the C
 * standard library is present and printf is available. Exercise A4
 * moves the assembly routine developed here into the FREESTANDING
 * firmware image, where it is not.
 */

#include <stdio.h>

/* Supplied by newlib's semihosting library. It establishes the file
 * descriptors that stdio writes through; without it, printf produces no
 * output whatever. Declared here because it appears in no public
 * header. Exercise A4 depends on understanding this function: the
 * firmware image cannot call it, which is precisely why printf is
 * unavailable there. */
extern void initialise_monitor_handles( void );

/* lab A3, step 1: the reference implementation, in C. Its purpose is to
 * establish the expected behaviour before any assembly is written. */
void swap_c( int *a, int *b )
{
    int tmp = *a;   /* retain a's value before it is overwritten */
    *a = *b;        /* a receives b's value                      */
    *b = tmp;       /* b receives a's original value             */
}

/* lab A3, step 2: the same operation in ARM assembly.
 *
 * `naked` suppresses the compiler-generated prologue and epilogue, so
 * the function body consists of these instructions and nothing else.
 * Nothing returns on our behalf: the closing `bx lr` is ours to write.
 *
 * By the AAPCS calling convention the first argument arrives in r0 and
 * the second in r1; this is relied upon rather than declared. Registers
 * r2 and r3 are call-clobbered and may be used without preservation.
 *
 * This provokes two -Wunused-parameter diagnostics. They are correct:
 * the parameter names are never referenced in C, the body reaching the
 * values through the calling convention, which the compiler cannot
 * observe. They are left in place deliberately. */
__attribute__( ( naked ) ) void swap_asm( int *a, int *b )
{
    __asm__ volatile (
        "ldr r2, [r0]\n"   /* r2 = *a */
        "ldr r3, [r1]\n"   /* r3 = *b */
        "str r3, [r0]\n"   /* *a = r3 */
        "str r2, [r1]\n"   /* *b = r2 */
        "bx  lr\n"         /* return; no epilogue is generated */
    );
}

int main( void )
{
    int x, y;

    /* Without this call the program runs to completion and prints
     * nothing. It performs the C run-time initialisation that newlib's
     * semihosting stdio requires. */
    initialise_monitor_handles();

    x = 1; y = 2;
    printf( "C   before: x=%d y=%d\n", x, y );
    swap_c( &x, &y );
    printf( "C   after:  x=%d y=%d\n", x, y );

    x = 1; y = 2;
    printf( "asm before: x=%d y=%d\n", x, y );
    swap_asm( &x, &y );
    printf( "asm after:  x=%d y=%d\n", x, y );

    return 0;
}

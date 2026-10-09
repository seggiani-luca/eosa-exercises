/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * WEEK 2 LAB STARTER - not a checkpoint.
 *
 * Exercise A3. This file builds and runs as supplied; the three pieces
 * marked TODO are yours to complete, in the order the lab gives them.
 * Build and run it before changing anything, and note what it does.
 */

#include <stdio.h>

/* Supplied by newlib's semihosting library, and declared here because
 * it appears in no public header. It establishes the file descriptors
 * that stdio writes through. */
extern void initialise_monitor_handles( void );

/* TODO (A3, step 2): exchange the values *a and *b, in C. */
void swap_c( int *a, int *b )
{
    (void) a;
    (void) b;
}

/* TODO (A3, step 3): exchange the values *a and *b, in ARM assembly.
 *
 * `naked` suppresses the compiler-generated prologue and epilogue, so
 * the body is exactly the instructions below and nothing else - which
 * is why the closing `bx lr` is already present and must remain last.
 * Add your instructions before it.
 *
 * The first argument arrives in r0 and the second in r1; r2 and r3 are
 * scratch registers you may use freely. */
__attribute__( ( naked ) ) void swap_asm( int *a, int *b )
{
    __asm__ volatile (
        "bx  lr\n"      /* return; your instructions belong above this */
    );
}

int main( void )
{
    int x, y;

    /* TODO (A3, step 1): this program prints nothing as supplied.
     * The lab states which single call is missing here. Add it. */

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

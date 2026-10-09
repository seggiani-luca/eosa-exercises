/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * WEEK 2 LAB SOLUTION - not a checkpoint.
 *
 * The finished state of the week-2 lab, whose starting point is
 * starter/ (itself derived from week-01/ minus its Makefile). Exercise
 * anchors appear as `lab A4:` / `lab B3:` comments below.
 *
 * Nothing later in the course builds on this folder: week-02/ is the
 * checkpoint that continues the course's own line of development.
 *
 * Nothing later in the course builds on this folder or on solution/.
 *
 * Week 1 checkpoint: proves the toolchain pipeline (cross-compile, link,
 * boot under QEMU) works end to end. No OS behavior yet - that starts
 * in week 2. Output goes over ARM semihosting, not real UART (week 3's
 * topic).
 *
 * This writes directly via a semihosting call instead of printf():
 * routing through printf fails at LINK time, not at run time. Depending
 * on whether the link searches a C library at all, it fails either on
 * printf itself (with -nostdlib) or on the syscall stubs newlib's stdio
 * needs - _write, _read, _sbrk - which this image does not provide.
 * Even had it linked, newlib's semihosting-backed stdio (rdimon) expects
 * C-runtime init our hand-rolled Reset_Handler never performs, so
 * supplying the library alone would not be enough. Direct semihosting
 * sidesteps libc entirely - appropriate for week 1, before any C-runtime
 * topic has been covered anyway.
 */

/* Only uint32_t is needed here; no other header is pulled in. */
#include <stdint.h>

/* Emit a NUL-terminated string over the ARM semihosting debug channel.
 * Semihosting is a convention where the target traps to the debugger
 * (here QEMU) to ask it to perform host I/O on the target's behalf. */
static void semihost_write0( const char *message )
{
    /* Semihosting call number 0x04 (SYS_WRITE0) must be in r0. The
     * `register ... __asm__("r0")` binding forces the value into that
     * exact register, which the calling convention does not otherwise
     * guarantee. */
    register uint32_t r0 __asm__( "r0" ) = 0x04;
    /* The pointer to the string must be in r1 for SYS_WRITE0. */
    register const char *r1 __asm__( "r1" ) = message;
    /* `bkpt 0xab` is the Thumb semihosting trap: QEMU sees it, reads r0
     * and r1, writes the string to its console, and resumes. "memory"
     * tells the compiler this may read arbitrary memory (the string),
     * so it must not reorder earlier writes past this point. */
    __asm__ volatile ( "bkpt 0xab" : : "r"( r0 ), "r"( r1 ) : "memory" );
}

/* lab A4: swap_asm(), transferred unchanged from hosted/swap.c, where
 * exercise A3 developed and verified it with printf. This image is
 * FREESTANDING (ISO C section 4): it is linked with -nostartfiles and
 * no C library, so printf is not merely inconvenient here but absent.
 * Verification therefore reports a fixed string through
 * semihost_write0 instead of printing the values.
 *
 * `naked` tells the compiler to emit NO prologue and NO epilogue: the
 * function body is exactly these instructions and nothing else. That
 * means nothing returns on our behalf, so the final `bx lr` is ours to
 * write. (Week 2's lecture uses the same attribute on Reset_Handler,
 * for a stricter reason: it runs before a valid stack exists, so a
 * compiler-generated prologue pushing registers would write through a
 * stack pointer that does not yet describe usable memory.)
 *
 * Per the AAPCS calling convention the first argument arrives in r0 and
 * the second in r1 - we never declare that, we rely on it. r2 and r3
 * are call-clobbered scratch, free to use without saving.
 *
 * This provokes two -Wunused-parameter warnings, and they are correct:
 * the parameter names are never referenced in C. They exist only to
 * document the interface to human readers and to make the compiler set
 * up the call site properly; the body reaches the values through the
 * calling convention, which the compiler cannot see into. Left in place
 * rather than silenced - a student's version warns identically, and the
 * warning is worth understanding. */
__attribute__( ( naked ) ) void swap_asm( int *a, int *b )
{
    __asm__ volatile (
        "ldr r2, [r0]\n"   /* r2 = *a  - load through the first pointer  */
        "ldr r3, [r1]\n"   /* r3 = *b  - load through the second pointer */
        "str r3, [r0]\n"   /* *a = r3  - store b's old value into a      */
        "str r2, [r1]\n"   /* *b = r2  - store a's old value into b      */
        "bx  lr\n"         /* return: `naked` means nobody does this for us */
    );
}

/* lab B3: a third implementation, for comparison in B2/B3. This one is
 * a normal (non-naked) function using extended inline asm, so the
 * compiler still writes the prologue and epilogue and still emits the
 * return - we must NOT write `bx lr` here.
 *
 * The operands are named positionally: %0 is the first input, %1 the
 * second. The clobber list declares what the block damages behind the
 * compiler's back: r2 and r3, and "memory" because we write through
 * pointers the compiler cannot see us following. Omit "memory" and the
 * compiler is entitled to assume *a and *b are unchanged, and may keep
 * stale copies in registers across this block. */
static void swap_ext( int *a, int *b )
{
    __asm__ volatile (
        "ldr r2, [%0]\n"
        "ldr r3, [%1]\n"
        "str r3, [%0]\n"
        "str r2, [%1]\n"
        :                                  /* no outputs */
        : "r"( a ), "r"( b )               /* inputs: any general register */
        : "r2", "r3", "memory" );          /* clobbers */
}

/* Program entry point, called from Reset_Handler once RAM is set up. */
int main( void )
{
    /* The whole point of week 1: if this line's text appears in the
     * QEMU console, the compile -> link -> boot pipeline works. */
    semihost_write0( "toolchain OK\n" );

    /* lab A4: verify the exchange in C and report a fixed string. The
     * values cannot be printed: semihost_write0 accepts a NUL-terminated
     * string, and printf does not link in this image at all (see this
     * file's header comment). This follows the same "<thing> OK"
     * convention as the line above. */
    {
        int x = 1, y = 2;
        swap_asm( &x, &y );
        semihost_write0( ( x == 2 && y == 1 ) ? "swap OK\n"
                                              : "swap FAILED\n" );

        /* lab B3: the extended-asm version must behave identically. */
        swap_ext( &x, &y );
        semihost_write0( ( x == 1 && y == 2 ) ? "swap_ext OK\n"
                                              : "swap_ext FAILED\n" );
    }
    /* Return to Reset_Handler, which then halts in its trailing spin
     * loop (a bare-metal main() has nowhere to "exit" to). */
    return 0;
}

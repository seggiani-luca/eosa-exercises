/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * WEEK 2 LAB STARTER - not a checkpoint.
 *
 * Derived from week-01/ (boot/, src/, scripts/ copied verbatim). What
 * was deliberately removed: the Makefile. Writing it is exercise A2;
 * lecture-notes/week-01.md section 6 is the reference. Exercise A4 then
 * transfers the swap_asm() routine developed in hosted/ into this file,
 * where printf is unavailable because the image is freestanding.
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

/* Program entry point, called from Reset_Handler once RAM is set up. */
int main( void )
{
    /* The whole point of week 1: if this line's text appears in the
     * QEMU console, the compile -> link -> boot pipeline works. */
    semihost_write0( "toolchain OK\n" );
    /* Return to Reset_Handler, which then halts in its trailing spin
     * loop (a bare-metal main() has nowhere to "exit" to). */
    return 0;
}

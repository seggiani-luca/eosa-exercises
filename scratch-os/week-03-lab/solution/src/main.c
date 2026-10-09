/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * WEEK 3 LAB SOLUTION - not a checkpoint. Nothing later in the course
 * builds on this folder.
 *
 * Derived from week-02/solution/src/main.c, whose contents it matches:
 * the lab's starter/ is this file with the exception side removed, and
 * exercises A3-A5 write it back. Exercise anchors appear as `lab A3:`
 * comments below.
 *
 * This program shows how the CPU reacts to faults and system calls:
 * it prints messages from privileged code, then drops itself to
 * unprivileged mode, asks to go back through a system call (`svc`),
 * and finally pokes a privileged-only register to trigger a fault.
 *
 * It writes directly via a semihosting call instead of printf():
 * routing through printf fails at LINK time, not at run time. Depending
 * on whether the link searches a C library at all, it fails either on
 * printf itself (with -nostdlib) or on the syscall stubs newlib's stdio
 * needs - _write, _read, _sbrk - which this image does not provide.
 * Even had it linked, newlib's semihosting-backed stdio (rdimon) expects
 * C-runtime init our hand-rolled Reset_Handler never performs, so
 * supplying the library alone would not be enough. Direct semihosting
 * sidesteps libc entirely, which this freestanding build does not
 * link against anyway.
 */

/* Only uint32_t is needed here; no other header is pulled in. */
#include <stdint.h>

/* lab A3: set this to 1 to make main() divide by zero instead of running the
 * privilege demo. CCR.DIV_0_TRP was turned on in Reset_Handler, so the
 * division traps into UsageFault_Handler instead of silently giving 0. */
#define DEMO_DIVIDE_BY_ZERO 0

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

/* A global array with an initial value: unlike a string literal (which
 * lives in FLASH, in .rodata), this one is writable data, so it must
 * live in RAM. The linker script stores its initial value in FLASH,
 * and Reset_Handler's .data copy loop moves that value into RAM before
 * main() runs - which is why, unlike in the base starting code, this
 * line now prints correctly. */
char data_message[] = "main: this text lives in .data\n";

/* Every handler below overrides the weak fallback defined in
 * boot/startup.s just by having the same name: the linker prefers a
 * strong (non-weak) symbol over a weak one, so the vector table now
 * points here instead of at Default_Handler.
 *
 * Printing from inside a fault handler works even after the CPU drops
 * to unprivileged mode elsewhere in this program: every exception
 * handler, no matter what mode the code that caused it was running
 * in, always runs privileged. Entering a handler is one of the two
 * ways to become privileged again (the other being reset). */

/* lab A3: the four fault handlers. UsageFault_Handler first, provoked
 * by DEMO_DIVIDE_BY_ZERO; the other three in the same form. */

/* HardFault: the CPU's fault of last resort, for example when a fault
 * happens that has been configured to have no handler of its own. */
void HardFault_Handler( void )
{
    semihost_write0( "HardFault_Handler: a hard fault happened\n" );
    /* Stop here forever: an unexpected fault is a bug, and looping at
     * a known place makes that visible to a debugger. */
    for ( ;; )
    {
    }
}

/* MemManage: a memory protection violation (needs an MPU; this board's
 * setup does not configure one, so this handler is not expected to
 * fire in this program, but it is enabled in Reset_Handler anyway). */
void MemManage_Handler( void )
{
    semihost_write0( "MemManage_Handler: a memory management fault happened\n" );
    for ( ;; )
    {
    }
}

/* BusFault: a bad memory access on the bus, for example accessing a
 * privileged-only system register while the CPU is unprivileged. */
void BusFault_Handler( void )
{
    semihost_write0( "BusFault_Handler: a bus fault happened\n" );
    for ( ;; )
    {
    }
}

/* UsageFault: a bad instruction, or (with CCR.DIV_0_TRP set) an integer
 * division by zero. */
void UsageFault_Handler( void )
{
    semihost_write0( "UsageFault_Handler: a usage fault happened\n" );
    for ( ;; )
    {
    }
}

/* lab A4: SVCall: raised by the `svc` instruction. This is how unprivileged
 * code asks privileged code to do something on its behalf: the `svc`
 * instruction itself works from unprivileged mode (unlike a direct
 * register write), and the CPU always runs the handler privileged.
 * Unlike the fault handlers above, this one is expected and returns
 * normally: `svc` is a deliberate request, not an error. */
void SVC_Handler( void )
{
    semihost_write0( "SVC_Handler: privileged code printing on behalf of unprivileged code\n" );
    /* Falling off the end of the function returns from the exception,
     * back to the instruction right after the `svc #0` that got here. */
}

/* Program entry point, called from Reset_Handler once RAM is set up. */
int main( void )
{
#if DEMO_DIVIDE_BY_ZERO
    /* `volatile` stops the compiler from computing the division at
     * compile time and from replacing it with something equivalent
     * that avoids an actual division (for example, the compiler knows
     * "1 / x" can only be 0 or 1, and could turn it into a comparison
     * instead of a real division instruction). Reading two volatile
     * variables at run time forces a real divide instruction - `udiv`,
     * the operands being unsigned - which
     * CCR.DIV_0_TRP turns into a UsageFault when the divisor is 0. */
    volatile uint32_t numerator = 42;
    volatile uint32_t zero = 0;
    volatile uint32_t result = numerator / zero;
    /* This line only runs if the fault did not stop the program (it
     * will not be reached: UsageFault_Handler loops forever). */
    ( void ) result;
    return 0;
#else
    /* main() starts running privileged: reset always begins in
     * privileged, Thread mode. Printing here works with no extra
     * steps. */
    semihost_write0( "main: privileged hello\n" );

    /* This string lives in RAM (.data), and Reset_Handler already
     * copied its initial value there before calling main(), so it
     * prints normally - unlike in the base starting code. */
    semihost_write0( data_message );

    /* lab A4: drop this code to unprivileged mode by setting bit 0 (nPRIV) of
     * the CONTROL register.
     * C equivalent (C has no way to name CONTROL directly):
     *     CONTROL = CONTROL | 1;   // nPRIV = 1: unprivileged from now on
     */
    __asm__ volatile (
        "mrs r0, control    \n" /* r0 = CONTROL (read) */
        "orr r0, r0, #1     \n" /* r0 |= 1: set nPRIV (modify) */
        "msr control, r0    \n" /* CONTROL = r0 (write): now unprivileged */
        "isb                \n" /* flush the pipeline: the CPU may have already
                                  * fetched later instructions under the old,
                                  * privileged rules; isb throws those away
                                  * and re-fetches under the new ones. */
        : : : "r0", "memory" );
    /* There is no instruction that raises privilege back up by itself:
     * once nPRIV is 1, plain code can never set it back to 0 by
     * writing CONTROL again (that write is itself only honored from
     * privileged code). Getting privileged again requires an
     * exception, which is exactly what the next step uses.
     * Also, from here on this code can no longer print directly:
     * semihosting's `bkpt` traps to the debugger the same way a fault
     * would, and QEMU treats it as a privileged-only operation, so
     * calling semihost_write0() now would itself fault. That is why
     * the request below goes through `svc` instead: `svc` is one of
     * the few things unprivileged code is still allowed to execute,
     * and the handler that runs because of it is privileged and can
     * print normally. */

    /* Ask privileged code to print, via a system call.
     * C equivalent: there is no plain C for "raise an exception"; the
     * closest description is "call a function that only privileged
     * code is allowed to run, through the one door unprivileged code
     * is still allowed to use":
     *     svc(0);   // traps into SVC_Handler, which prints and returns
     */
    __asm__ volatile ( "svc #0" : : : "memory" );

    /* lab A5: now try to write directly to SysTick's control register
     * (0xE000E010), a system register that only privileged code may
     * access. This code is unprivileged (nPRIV was set above and
     * nothing has raised it back), so this write is not allowed and
     * is expected to fault.
     * C equivalent:
     *     *(volatile uint32_t *)0xE000E010 = 0;
     */
    volatile uint32_t *systick_ctrl = ( volatile uint32_t * ) 0xE000E010;
    *systick_ctrl = 0;

    /* Not reached: the line above faults before getting here. */
    return 0;
#endif
}

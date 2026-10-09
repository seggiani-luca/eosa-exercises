/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * WEEK 2 LAB STARTER - not a checkpoint. Copied verbatim from week-01/;
 * the week-2 lab does not ask you to change the boot code.
 *
 * Minimal ARM Cortex-M3 boot code: vector table + reset handler.
 *
 * WEEK 1: this file is given as-is. We build it ourselves, by hand,
 * in week 2 - see course-material's slide-sources/lecture-notes/week-02.md.
 * Only enough is here to prove the toolchain pipeline works end to end.
 *
 * WEEK 3 will extend the vector table with the SysTick and UART/GPIO
 * interrupt entries this file deliberately leaves at 0 for now.
 */

/* Fixed-width integer types (uint32_t); this file needs no other libc. */
#include <stdint.h>

/* Symbols the linker script defines - addresses, not variables. Declared
 * extern here so C can take their address; their value comes from
 * mps2_m3.ld at link time. */
extern uint32_t _estack;                 /* top of stack, from mps2_m3.ld */
extern uint32_t _sidata, _sdata, _edata; /* .data load addr, then run start/end */
extern uint32_t _sbss, _ebss;            /* .bss run-address start/end */

/* The C entry point, defined in src/main.c; called at the end of reset. */
extern int main( void );

/* Every unhandled exception/interrupt lands here instead of jumping to
 * garbage. A real handler for a given vector replaces this entry in the
 * table below once that topic is covered (SysTick and UART in week 3). */
void Default_Handler( void )
{
    /* Spin forever: an unexpected exception is a bug; freezing here
     * keeps the fault visible in a debugger rather than letting the CPU
     * run off into undefined behaviour. */
    for( ;; )
    {
    }
}

/* First code to run after reset. On Cortex-M the hardware has already
 * loaded SP from isr_vector[0], so plain C is safe here in week 1. */
void Reset_Handler( void )
{
    /* Copy .data's initial values from their load address in FLASH to
     * their run address in RAM, one word at a time. */
    uint32_t *src = &_sidata;  /* source cursor: .data image in FLASH */
    uint32_t *dest = &_sdata;   /* destination cursor: .data in RAM */
    while( dest < &_edata )     /* stop once the whole .data span is copied */
    {
        *dest++ = *src++;      /* copy one word, advance both cursors */
    }

    /* Zero-initialize .bss, as the C standard requires for
     * static-storage variables with no explicit initializer. */
    dest = &_sbss;             /* reuse the cursor: start of .bss in RAM */
    while( dest < &_ebss )     /* stop at the end of .bss */
    {
        *dest++ = 0;          /* clear one word, advance the cursor */
    }

    main();                    /* hand control to the C program */

    /* main() should not return in a bare-metal image; if it does,
     * stop cleanly rather than run off into undefined memory. */
    for( ;; )
    {
    }
}

/* The vector table: the CPU reads entry 0 as the initial SP and entry 1
 * as the reset address, then uses the rest to dispatch exceptions.
 * ARMv7-M requires at least these 16 entries. Peripheral interrupt
 * entries are added in week 3 once there's a handler to point them at.
 * `section(".isr_vector")` places it where mps2_m3.ld puts it first in
 * FLASH; `const` keeps it in read-only memory. */
__attribute__( ( section( ".isr_vector" ) ) )
void ( * const isr_vector[] )( void ) =
{
    (void (*)(void)) &_estack,  /* 0: initial stack pointer value */
    Reset_Handler,              /* 1: reset - first instruction after power-on */
    Default_Handler,            /* 2: NMI */
    Default_Handler,            /* 3: HardFault */
    Default_Handler,            /* 4: MemManage */
    Default_Handler,            /* 5: BusFault */
    Default_Handler,            /* 6: UsageFault */
    0, 0, 0, 0,                 /* 7-10: reserved by the architecture */
    Default_Handler,            /* 11: SVCall */
    Default_Handler,            /* 12: DebugMonitor */
    0,                          /* 13: reserved */
    Default_Handler,            /* 14: PendSV */
    Default_Handler,            /* 15: SysTick (real handler arrives in week 3) */
};

/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * WEEK 2 LAB SOLUTION - not a checkpoint. Nothing later in the course
 * builds on this folder.
 *
 * lab B5: this file is UNCHANGED from the starter. B5's two experiments
 * are deliberately recorded here as comments rather than applied, so
 * that this folder still links. Both messages below are real output
 * from this toolchain.
 *
 *   Experiment 1 - grow the reserved stack until it meets the heap:
 *       _Min_Stack_Size = 0x400000;   (instead of 0x1000)
 *   produces, at LINK time:
 *       ld: region RAM overflowed: heap collides with the stack
 *   That text is our own, from the ASSERT at the bottom of this file.
 *
 *   Experiment 2 - shrink FLASH below what .text needs:
 *       FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 128
 *   produces:
 *       ld: section `.text' will not fit in region `FLASH'
 *       ld: region `FLASH' overflowed by 308 bytes
 *   That text is the linker's own, from the MEMORY region sizes.
 *
 * The difference is the point of B5: the first is a check we wrote and
 * could have forgotten to write; the second the linker performs for any
 * region, always. Both fail the BUILD rather than corrupting memory at
 * run time.
 *
 * Linker script for QEMU's mps2-an385 machine (ARM Cortex-M3, MPS2
 * platform). Memory layout matches the board's actual FLASH/RAM
 * addresses and sizes.
 *
 * Deliberately minimal for week 1: one stack, no per-task partitions
 * (that's week 4's topic), no heap (nothing here allocates dynamically -
 * see three-pillar-curriculum's static-memory scratch-OS scope).
 */

/* Physical memory regions of the board, with their access attributes.
 * The linker uses these to place sections and to catch overflow. */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 4M  /* code + read-only data, execute-in-place */
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 4M  /* data, bss, stack */
}

/* The image's entry point - the symbol whose address the debugger and
 * QEMU use as the start of execution (also entry 1 of the vector table). */
ENTRY(Reset_Handler)

_estack = ORIGIN(RAM) + LENGTH(RAM);   /* initial SP = top of RAM; the stack grows downward from here */
_Min_Stack_Size = 0x1000;              /* 4 KB reserved for the stack - checked against the heap below */
_Min_Heap_Size = 0x1000;               /* 4 KB reserved for the heap - only newlib's stdio buffering would touch it */

SECTIONS
{
    /* Vector table first, at address 0, so the CPU finds the initial SP
     * and reset vector where the architecture requires them. */
    .isr_vector :
    {
        . = ALIGN(4);            /* vector table must be word-aligned */
        KEEP(*(.isr_vector))     /* KEEP: never garbage-collect it, even though nothing "calls" it */
        . = ALIGN(4);            /* keep the location counter aligned for what follows */
    } > FLASH                    /* the vector table lives at the base of FLASH */

    /* Executable code and read-only data. */
    .text :
    {
        . = ALIGN(4);           /* word-align the start of this section */
        *(.text*)               /* all code from every input object */
        *(.rodata*)             /* string literals, const tables */
        . = ALIGN(4);           /* word-align the end so _etext is aligned */
        _etext = .;             /* end-of-text marker (unused here, handy when debugging) */
    } > FLASH                   /* place this section in the FLASH region */

    _sidata = LOADADDR(.data);  /* FLASH address where .data's initial image is stored */

    /* Initialized variables: they live (run) in RAM but their initial
     * contents are stored in FLASH and copied over by Reset_Handler. */
    .data :
    {
        . = ALIGN(4);           /* word-align the start of .data */
        _sdata = .;             /* start of .data in RAM - Reset_Handler copies from here */
        *(.data*)               /* every initialized-variable input section */
        . = ALIGN(4);           /* word-align the end */
        _edata = .;             /* end of .data in RAM - copy loop stops here */
    } > RAM AT > FLASH          /* run address in RAM, load address in FLASH */

    /* Zero-initialized variables: reserve RAM, no stored image. */
    .bss :
    {
        . = ALIGN(4);           /* word-align the start of .bss */
        _sbss = .;              /* start of .bss - Reset_Handler zeroes from here */
        *(.bss*)                /* every zero-initialized input section */
        *(COMMON)               /* legacy tentative-definition symbols */
        . = ALIGN(4);           /* word-align the end */
        _ebss = .;              /* end of .bss - zero loop stops here */
    } > RAM

    /* Heap region: reserved so the overflow check below is meaningful,
     * even though week 1's code never calls malloc. */
    .heap :
    {
        . = ALIGN(8);          /* malloc alignment on this ABI is 8 */
        PROVIDE( end = . );     /* `end`/`_end`: the symbol newlib's sbrk uses as the heap base */
        PROVIDE( _end = . );    /* same address under the alternate spelling */
        _heap_bottom = .;      /* record the heap's low address */
        . = . + _Min_Heap_Size; /* advance the location counter to reserve the heap span */
        _heap_top = .;         /* record the heap's high address (checked below) */
    } > RAM                    /* the heap lives in RAM */

    __StackLimit = _estack - _Min_Stack_Size;  /* lowest address the stack may grow to */
    /* Link-time guard: fail the build if the reserved heap and stack
     * would overlap, rather than corrupting memory at runtime. */
    ASSERT(__StackLimit >= _heap_top, "region RAM overflowed: heap collides with the stack")
}

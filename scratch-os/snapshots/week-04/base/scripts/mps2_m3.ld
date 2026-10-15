/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Linker script for QEMU's mps2-an385 (Cortex-M3). Flat layout: each task
 * has a stack, but no partition yet. No heap.
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
_Min_Stack_Size = 0x1000;              /* 4 KB reserved for the kernel stack - checked against .bss below */
TASK_STACK_SIZE = 0x400;               /* 1 KB stack for each task */

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

    /* >>> UNIT 2: paste the task partitions here, before .data (see the slides). <<< */

    _sidata = LOADADDR(.data);  /* FLASH address where .data's initial image is stored */

    /* Initialized variables: they live (run) in RAM but their initial
     * contents are stored in FLASH and copied over by Reset_Handler. */
    .data :
    {
        . = ALIGN(4);           /* word-align the start of .data */
        _sdata = .;             /* start of .data in RAM - Reset_Handler copies from here */
        *(.data*)               /* every initialized variable: kernel and tasks alike */
        . = ALIGN(4);           /* word-align the end */
        _edata = .;             /* end of .data in RAM - copy loop stops here */
    } > RAM AT > FLASH          /* run address in RAM, load address in FLASH */

    /* Zero-initialized variables: reserve RAM, no stored image. */
    .bss :
    {
        . = ALIGN(4);           /* word-align the start of .bss */
        _sbss = .;              /* start of .bss - Reset_Handler zeroes from here */
        *(.bss*)                /* every zero-initialized variable: kernel and tasks alike */
        *(COMMON)               /* legacy tentative-definition symbols */
        . = ALIGN(4);           /* word-align the end */
        _ebss = .;              /* end of .bss - zero loop stops here */
    } > RAM

    /* >>> UNIT 2: delete from this comment down to _task_b_data_load. <<<
     * For now each task only gets a plain stack, and the symbols the
     * kernel needs are defined with empty data: _task_X_part_start and
     * _task_X_part_end are the stack bounds, _task_X_data_start equals
     * _task_X_data_end, so the partition copy loop does nothing. */

    /* Task stacks. NOLOAD: reserve RAM, store nothing in FLASH. */
    .task_stacks (NOLOAD) :
    {
        . = ALIGN(8);                      /* stacks are 8-byte aligned */
        _task_a_part_start = .;            /* lowest address of A's stack */
        . = . + TASK_STACK_SIZE;
        _task_a_stack_top = .;             /* A's stack grows down from here */
        _task_a_part_end = .;
        _task_b_part_start = .;            /* lowest address of B's stack */
        . = . + TASK_STACK_SIZE;
        _task_b_stack_top = .;             /* B's stack grows down from here */
        _task_b_part_end = .;
    } > RAM

    /* No task data of their own yet: empty ranges (start = end). */
    _task_a_data_start = .;
    _task_a_data_end   = .;
    _task_a_data_load  = LOADADDR(.data);
    _task_b_data_start = .;
    _task_b_data_end   = .;
    _task_b_data_load  = LOADADDR(.data);

    __StackLimit = _estack - _Min_Stack_Size;  /* lowest address the kernel stack may grow to */
    /* Link-time guard: fail the build if .bss and the reserved kernel
     * stack would overlap, rather than corrupting memory at runtime. */
    ASSERT(__StackLimit >= _ebss, "region RAM overflowed: .bss collides with the stack")
}

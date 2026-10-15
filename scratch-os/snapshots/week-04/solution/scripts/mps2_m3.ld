/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Linker script for QEMU's mps2-an385 machine (ARM Cortex-M3, MPS2
 * platform). Memory layout matches the board's actual FLASH/RAM
 * addresses and sizes.
 *
 * Deliberately minimal: one kernel stack, two 4 KB task partitions
 * (one per task, each with its own stack and variables), no heap
 * (nothing here allocates dynamically).
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
PART_SIZE       = 0x1000;              /* size of one task partition: 4 KB (one MPU region needs a power of two) */
TASK_STACK_SIZE = 0x400;               /* 1 KB of each partition is the task's stack, at its bottom */

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

    /* Task partitions. Each one is 4 KB, aligned to 4 KB, laid out low to high:
     *   [ stack (1 KB) | the task's .data and .bss | unused padding ]
     * The stack is at the bottom so an overflow leaves the partition
     * instead of silently damaging the task's own variables.
     * These statements come BEFORE the generic .data/.bss below: the
     * linker gives an input section to the first statement that matches it. */

    /* Task A: its stack. NOLOAD: reserve RAM, store nothing in FLASH.
     * ALIGN(ORIGIN(RAM), 4096): "." is still a FLASH address after .text,
     * so we align a RAM address instead. */
    .task_a_stack ALIGN(ORIGIN(RAM), 4096) (NOLOAD) :
    {
        _task_a_part_start = .;            /* partition start = lowest stack address */
        . = . + TASK_STACK_SIZE;           /* reserve the stack */
        _task_a_stack_top = .;             /* the stack grows down from here */
    } > RAM

    /* Task A: its variables, taken from task_a.o only (no attributes
     * needed in the C file). They run in RAM, initial values sit in FLASH. */
    .task_a_data :
    {
        . = ALIGN(4);                      /* word-align the start */
        _task_a_data_start = .;            /* start in RAM - the partition init copies to here */
        *task_a.o(.data .data.* .bss .bss.* COMMON)   /* every variable of task_a.o */
        . = ALIGN(4);                      /* word-align the end */
        _task_a_data_end = .;              /* end in RAM - the copy stops here */
    } > RAM AT > FLASH                     /* run address in RAM, load address in FLASH */
    _task_a_data_load = LOADADDR(.task_a_data);   /* FLASH address of the initial image */

    /* Task A: padding up to the end of its 4 KB. NOLOAD: nothing stored. */
    .task_a_pad (NOLOAD) :
    {
        . = _task_a_part_start + PART_SIZE;   /* jump to the partition end */
        _task_a_part_end = .;              /* partition end (exclusive) */
    } > RAM

    /* Task B: the same three sections, in the next 4 KB. */
    .task_b_stack ALIGN(4096) (NOLOAD) :
    {
        _task_b_part_start = .;            /* partition start = lowest stack address */
        . = . + TASK_STACK_SIZE;           /* reserve the stack */
        _task_b_stack_top = .;             /* the stack grows down from here */
    } > RAM

    .task_b_data :
    {
        . = ALIGN(4);                      /* word-align the start */
        _task_b_data_start = .;            /* start in RAM */
        *task_b.o(.data .data.* .bss .bss.* COMMON)   /* every variable of task_b.o */
        . = ALIGN(4);                      /* word-align the end */
        _task_b_data_end = .;              /* end in RAM */
    } > RAM AT > FLASH                     /* run address in RAM, load address in FLASH */
    _task_b_data_load = LOADADDR(.task_b_data);   /* FLASH address of the initial image */

    .task_b_pad (NOLOAD) :
    {
        . = _task_b_part_start + PART_SIZE;   /* jump to the partition end */
        _task_b_part_end = .;              /* partition end (exclusive) */
    } > RAM

    /* Link-time guards: the variables must fit above the stack. */
    ASSERT(_task_a_data_end <= _task_a_part_start + PART_SIZE, "task A data does not fit in its partition")
    ASSERT(_task_b_data_end <= _task_b_part_start + PART_SIZE, "task B data does not fit in its partition")

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

    __StackLimit = _estack - _Min_Stack_Size;  /* lowest address the kernel stack may grow to */
    /* Link-time guard: fail the build if .bss and the reserved kernel
     * stack would overlap, rather than corrupting memory at runtime. */
    ASSERT(__StackLimit >= _ebss, "region RAM overflowed: .bss collides with the stack")
}

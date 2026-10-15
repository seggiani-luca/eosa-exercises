MEMORY {
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 4M /* 4 MiB */
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 4M /* 4 MiB */
}

ENTRY(Reset_Handler)

_estack = ORIGIN(RAM) + LENGTH(RAM);
_stack_size = 0x400; /* 1 KiB */
_stack_limit = _estack - _stack_size;

_part_size = 0x1000;

SECTIONS {
	.isr_vector : {
		. = ALIGN(4);
		KEEP(*(.isr_vector))
		. = ALIGN(4);
	} > FLASH

	.text : {
		. = ALIGN(4);
		*(.text*)
		*(.rodata*)
		. = ALIGN(4);
		_etext = .;
	} > FLASH

	.data : {
		. = ALIGN(4);
		_sdata = .;
		*(.data*)
		. = ALIGN(4);
		_edata = .;
	} > RAM AT > FLASH
	
	_sidata = LOADADDR(.data);

	.bss : {
		. = ALIGN(4);
		_sbss = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} > RAM
}

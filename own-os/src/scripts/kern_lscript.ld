MEMORY {
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 4M /* 4 MiB */
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 4M /* 4 MiB */
}

ENTRY(Reset_Handler)

_estack = ORIGIN(RAM) + LENGTH(RAM);

/* process partitions */
_stack_size = 0x400;   /* 1 KiB */
_part_size = 0x1000;   /* 4 KiB */
_heap_align = 0x1000;  /* 1 KiB */
_share_align = 0x1000; /* 1 KiB */

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
		*(EXCLUDE_FILE (*process*.o *idle*.o) .data*)
		. = ALIGN(4);
		_edata = .;
	} > RAM AT > FLASH
	
	_sidata = LOADADDR(.data);

	.bss : {
		. = ALIGN(4);
		_sbss = .;
		*(EXCLUDE_FILE (*process*.o *idle*.o) .bss*)
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} > RAM

	.heap : {
		. = ALIGN(_heap_align);
		_sheap = .;
		*(.heap*)
		. = ALIGN(_heap_align);
		_eheap = .;
	} > RAM
	
	.share : {
		. = ALIGN(_share_align);
		_sshare = .;
		*(.share*)
		. = ALIGN(_share_align);
		_eshare = .;
	} > RAM
}

INCLUDE src/scripts/kern_lscript.ld

SECTIONS {
    _process_start = ALIGN(., _part_size);

	.idle_stack ALIGN(_process_start, _part_size) (NOLOAD) : {
        __idle_spart = .;
		. = . + _stack_size;
        __idle_estack = .;
	} > RAM

    .idle_data : {
		. = ALIGN(4);
        __idle_sdata = .;
		*idle.o(.data .data.*)
		. = ALIGN(4);
        __idle_edata = .;
	} > RAM AT > FLASH

    __idle_sidata = LOADADDR(.idle_data);
	
	.idle_bss : {
		. = ALIGN(4);
        __idle_sbss = .;
		*idle.o(.bss .bss.*)
		. = ALIGN(4);
        __idle_ebss = .;
	} > RAM

	.idle_pad (NOLOAD) : {
		. = __idle_spart + _part_size;
        __idle_epart = .;
	} > RAM

    _process_start = ALIGN(., _part_size);

	.process_a_stack ALIGN(_process_start, _part_size) (NOLOAD) : {
        __process_a_spart = .;
		. = . + _stack_size;
        __process_a_estack = .;
	} > RAM

    .process_a_data : {
		. = ALIGN(4);
        __process_a_sdata = .;
		*process_a.o(.data .data.*)
		. = ALIGN(4);
        __process_a_edata = .;
	} > RAM AT > FLASH

    __process_a_sidata = LOADADDR(.process_a_data);
	
	.process_a_bss : {
		. = ALIGN(4);
        __process_a_sbss = .;
		*process_a.o(.bss .bss.*)
		. = ALIGN(4);
        __process_a_ebss = .;
	} > RAM

	.process_a_pad (NOLOAD) : {
		. = __process_a_spart + _part_size;
        __process_a_epart = .;
	} > RAM

}

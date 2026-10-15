INCLUDE src/scripts/kern_lscript.ld

SECTIONS {
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

    _process_start = ALIGN(., _part_size);

	.process_b_stack ALIGN(_process_start, _part_size) (NOLOAD) : {
        __process_b_spart = .;
		. = . + _stack_size;
        __process_b_estack = .;
	} > RAM

	.process_b_data : {
		. = ALIGN(4);
        __process_b_sdata = .;
		*process_b.o(.data .data.*)
		. = ALIGN(4);
        __process_b_edata = .;
	} > RAM AT > FLASH

    __process_b_sidata = LOADADDR(.process_b_data);
	
	.process_b_bss : {
		. = ALIGN(4);
        __process_b_sbss = .;
		*process_b.o(.bss .bss.*)
		. = ALIGN(4);
        __process_b_ebss = .;
	} > RAM

	.process_b_pad (NOLOAD) : {
		. = __process_b_spart + _part_size;
        __process_b_epart = .;
	} > RAM

}

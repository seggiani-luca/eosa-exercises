.syntax unified

.extern _estack
.extern _sidata, _sdata, _edata
.extern _sbss, _ebss

.extern main

.section .isr_vector, "a", %progbits
	.word _estack         // initial SP 
	.word Reset_Handler   // reset
	.word Default_Handler // NMI
	.word Default_Handler // hard fault
	.word Default_Handler // memory management 
	.word Default_Handler // bus fault
	.word Default_Handler // usage fault
	.word 0               // reserved
	.word 0
	.word 0
	.word 0
	.word Default_Handler // supervisor call
	.word Default_Handler // debug monitor
	.word 0               // reserved
	.word Default_Handler // pending supervisor
	.word Default_Handler // system tick 

.section .text	

.thumb_func
.global Reset_Handler
Reset_Handler:

Data_Init:
	ldr r0, =_sdata
	ldr r1, =_edata
	ldr r2, =_sidata

Data_Init_Loop:
	cmp r0, r1
	beq Bss_Init 

	ldr r3, [r2], #4
	str r3, [r0], #4

	b Data_Init_Loop

Bss_Init:
	ldr r0, =_sbss
	ldr r1, =_ebss
	mov r2, #0

Bss_Init_Loop:
	cmp r0, r1 
	beq Main_Init

	str r2, [r0], #4

	b Bss_Init_Loop

Main_Init:
	bl main
	b .

.thumb_func
.global Default_Handler
Default_Handler:
	b .

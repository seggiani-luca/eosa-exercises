#include "../../syscall.h"

.syntax unified

.section .text

.extern priv_down
.extern priv_up

.global start_process
.thumb_func
start_process:
	svc #SYS_START_PROC
	bx lr

.global exit_process
.thumb_func
exit_process:
	svc #SYS_EXIT_PROC
	bx lr

.global start_process_asm
.thumb_func
start_process_asm:
	sub r0, r0, #32
	mov r3, #0
	mov r2, r0
	
	// frame[0] : r0
	str r3, [r2], #4
	
	// frame[1] : r1
	str r3, [r2], #4
	
	// frame[2] : r2
	str r3, [r2], #4
	
	// frame[3] : r3
	str r3, [r2], #4

	// frame[4] : r12
	str r3, [r2], #4
	
	// frame[5] : lr
	ldr r3, =exit_process
	str r3, [r2], #4
	
	// frame[6] : pc
	str r1, [r2], #4
	
	// frame[7] : xPSR
	ldr r3, =0x01000000
	str r3, [r2], #4

	msr psp, r0
	push {lr}
	bl priv_down
	pop {lr}
	isb

	bx  lr

.global exit_process_asm
.thumb_func
exit_process_asm:
	push {lr}
	bl priv_up
	pop {lr}

	bx  lr

#include "../../syscall.h"

.syntax unified

.section .text

.extern dispatch_syscall
.extern priv_down 
.extern context_in 
.extern context_out
.extern exit_process_post

.global SVC_Handler 
.thumb_func
SVC_Handler:
	// from handler or thread?
	tst lr, #4
	ite eq
	mrseq r0, msp    // get handler pointer
	blne context_out // get thread pointer, save context

	// get svc syscall number
	ldr r1, [r0, #24]
	subs r1, #2
	ldrh r1, [r1]
	uxtb r1, r1

	// handle syscall
	bl dispatch_syscall

	// restore context
	bl context_in

	// return
	bx r0

#include "../../syscall.h"

.syntax unified

.section .text

.extern dispatch_syscall
.extern start_process_post
.extern priv_down 
.extern context_in 

.global SVC_Handler 
.thumb_func
SVC_Handler:
	// from handler or thread?
	tst lr, #4
	ite eq
	mrseq r0, msp // get handler pointer
	mrsne r0, psp // get stack pointer

	// get svc syscall number
	ldr r1, [r0, #24]
	subs r1, #2
	ldrh r1, [r1]
	uxtb r1, r1

	// have to start process?
    cmp  r1, #SYS_START_PROC
    bne  SVC_Handler_Dispatch
	
	bl priv_down
	bl context_in

    bx   r0

SVC_Handler_Dispatch:

	// get exc_return
	mov r2, lr

	bl dispatch_syscall

	// return
	bx r0

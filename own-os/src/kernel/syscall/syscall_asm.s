.syntax unified

.section .text

.extern dispatch_syscall

.global SVC_Handler
.thumb_func
SVC_Handler:
	// from handler or thread?
	tst lr, #4
	ite eq
	mrseq r0, msp // get handler pointer
	mrsne r0, psp // get stack pointer

	// get SVC syscall number
	ldr r1, [r0, #24]
	subs r1, #2
	ldrh r1, [r1]
	uxtb r1, r1

	// get exc_return
	mov r2, lr

	bl dispatch_syscall

	// return
	bx r0

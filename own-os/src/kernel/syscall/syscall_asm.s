.syntax unified

.section .text

.extern dispatch_syscall

.global SVC_Handler
.thumb_func
SVC_Handler:
	tst lr, #4
	ite eq
	mrseq r0, msp
	mrsne r0, psp
	ldr r1, [r0, #24]

	subs r1, #2
	ldrh r1, [r1]
	uxtb r1, r1

	mov r2, lr

	bl dispatch_syscall
	bx r0

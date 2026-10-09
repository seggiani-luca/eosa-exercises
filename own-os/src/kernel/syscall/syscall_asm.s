.syntax unified

.section .text

.extern irq_save
.extern irq_restore
.extern dispatch_syscall

.thumb_func
.global SVC_Handler
SVC_Handler:
	tst lr, #4
	ite eq
	mrseq r0, msp
	mrsne r0, psp
	ldr r1, [r0, #24]

	subs r1, #2
	ldrh r1, [r1]
	uxtb r1, r1

	b dispatch_syscall

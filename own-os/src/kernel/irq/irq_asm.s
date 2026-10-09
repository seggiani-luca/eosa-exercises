.syntax unified

	.global irq_save
	.thumb_func
irq_save:
	mrs r0, primask
	cpsid i // mask
	bx lr

	.global irq_restore
	.thumb_func
irq_restore:
	msr primask, r0
	bx lr

.syntax unified

	.global priv_down 
	.thumb_func
priv_down:
	mrs r0, control 
	orr r0, r0, #1
	msr control, r0
	isb
	bx lr

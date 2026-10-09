.syntax unified

.thumb_func
.global shst_call
shst_call:
	bkpt #0xab
	bx lr

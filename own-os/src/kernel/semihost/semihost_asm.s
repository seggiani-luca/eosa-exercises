.syntax unified

.global shst_call
.thumb_func
shst_call:
	bkpt #0xab
	bx lr

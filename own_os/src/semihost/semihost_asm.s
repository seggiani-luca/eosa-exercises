.syntax unified

.thumb_func
.global sh_call
sh_call:
	bkpt #0xab
	bx lr

.syntax unified

.global wfi
.thumb_func
wfi:
	wfi
	bx lr

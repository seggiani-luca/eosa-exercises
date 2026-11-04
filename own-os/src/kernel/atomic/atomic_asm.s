.syntax unified

.global ldrex
.thumb_func
ldrex:
	ldrex r1, [r0]
	mov r0, r1
	bx lr

.global strex
.thumb_func
strex:
	strex r2, r1, [r0]
	mov r0, r2
	bx lr

.global dmb
.thumb_func
dmb:
	dmb
	bx lr

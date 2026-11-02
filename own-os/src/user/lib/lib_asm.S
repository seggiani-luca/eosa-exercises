#include "../../syscall.h"

.syntax unified

.section .text

.global fwrite 
.thumb_func
fwrite:
    svc #SYS_WRITE
    bx lr

.global fread 
.thumb_func
fread:
    svc #SYS_READ
    bx lr

.global tick
.thumb_func
tick:
    svc #SYS_TICK
    bx lr

.global delay
.thumb_func
delay:
    svc #SYS_DELAY
    bx lr

.global yield
.thumb_func
yield:
    svc #SYS_YIELD
    bx lr

.global terminate
.thumb_func
terminate:
	svc #SYS_EXIT_PROC
	bx lr

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

.global digital_write 
.thumb_func
digital_write:
    svc #SYS_DIGITAL_WRITE
    bx lr

.global digital_read 
.thumb_func
digital_read:
    svc #SYS_DIGITAL_READ
    bx lr

.global pin_mode 
.thumb_func
pin_mode:
    svc #SYS_PIN_MODE
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

.global until
.thumb_func
until:
    svc #SYS_UNTIL
    bx lr

.global yield
.thumb_func
yield:
    svc #SYS_YIELD
    bx lr

.global sem_new
.thumb_func
sem_new:
    svc #SYS_SEM_NEW
    bx lr

.global sem_lock
.thumb_func
sem_lock:
    svc #SYS_SEM_LOCK
    bx lr

.global sem_unlock
.thumb_func
sem_unlock:
    svc #SYS_SEM_UNLOCK
    bx lr

.global sem_free
.thumb_func
sem_free:
    svc #SYS_SEM_FREE
    bx lr

.global mutex_new
.thumb_func
mutex_new:
    svc #SYS_MUTEX_NEW
    bx lr

.global mutex_lock
.thumb_func
mutex_lock:
    svc #SYS_MUTEX_LOCK
    bx lr

.global mutex_unlock
.thumb_func
mutex_unlock:
    svc #SYS_MUTEX_UNLOCK
    bx lr

.global mutex_free
.thumb_func
mutex_free:
    svc #SYS_MUTEX_FREE
    bx lr

.global terminate
.thumb_func
terminate:
	svc #SYS_EXIT_PROC
	bx lr

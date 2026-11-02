#include "../../syscall.h"

.syntax unified

.section .text

.extern process_table
.extern current 

.equ PROCESS_SIZE, 48
.equ EXC_RETURN_THREAD_PSP, 0xFFFFFFFD

.global context_out
.thumb_func
context_out:
    mrs   r0, psp
    stmdb r0!, {r4-r11}

    ldr   r1, =current
	ldr   r1, [r1] 
    str   r0, [r1]

    adds  r0, #32

	bx    lr

.global context_in
.thumb_func
context_in:
    ldr   r0, =current
	ldr   r0, [r0] 
    ldr   r0, [r0]             

    ldmia r0!, {r4-r11}
    msr   psp, r0

    ldr   r0, =EXC_RETURN_THREAD_PSP
    bx    lr

.global start_process 
.thumb_func
start_process:
	svc #SYS_START_PROC
	bx r0

.global exit_process 
.thumb_func
exit_process:
	svc #SYS_EXIT_PROC
	bx r0

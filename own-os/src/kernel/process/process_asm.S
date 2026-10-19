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

    ldr   r2, =process_table
    add   r2, r2, r1, lsl #4
    add   r2, r2, r1, lsl #5   
    str   r0, [r2]              

    bx    lr


.global context_in
.thumb_func
context_in:
    ldr   r0, =current
    ldr   r0, [r0]             

    ldr   r1, =process_table
    add   r1, r1, r0, lsl #4
    add   r1, r1, r0, lsl #5   
    ldr   r0, [r1]             

    ldmia r0!, {r4-r11}
    msr   psp, r0

    ldr   lr, =EXC_RETURN_THREAD_PSP
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

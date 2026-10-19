.syntax unified

.extern ticks
.extern scheduler
.extern context_in

.global SysTick_Handler
.thumb_func
SysTick_Handler:
    ldr r0, =ticks
    ldr r1, [r0]
    adds r1, r1, #1
    str r1, [r0]

	bl context_out

    bl scheduler

	bl context_in
	bx r0

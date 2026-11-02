.syntax unified

.extern ticks
.extern scheduler
.extern context_in

.extern SysTick_Handler_C // TODO

.global SysTick_Handler
.thumb_func
SysTick_Handler:
	bl SysTick_Handler_C

	# context switch
	bl context_out
    bl scheduler
	bl context_in

    bx r0

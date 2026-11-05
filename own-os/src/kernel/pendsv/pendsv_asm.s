.syntax unified

.extern context_out
.extern scheduler
.extern context_in
.extern irq_save
.extern irq_restore

.global PendSV_Handler 
.thumb_func
PendSV_Handler:
	# context switch
	bl context_out
    bl scheduler
	bl context_in

    bx r0

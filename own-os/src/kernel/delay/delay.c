#include "../include/delay.h"

// push to timer queue
void timer_queue_push(timer_queue *queue, process *proc, unsigned int ms) {
	// block
	proc->state = PROC_BLOCKED;

	while (*queue && ms >= (*queue)->block.delta) {
		ms -= (*queue)->block.delta;
		queue = &((*queue)->next);
	}

	proc->block.delta = ms;
	proc->next = *queue;

	if (*queue)
		(*queue)->block.delta -= ms;

	*queue = proc;
}

// advance time in timer queue
void timer_queue_advance(timer_queue *queue, unsigned int ms) {
	while (*queue && ms >= (*queue)->block.delta) {
		ms -= (*queue)->block.delta;
		process *ready = (*queue);
		*queue = (*queue)->next;

		// ready it
		ready->state = PROC_READY;
		process_queue_push(queue_from_prio(ready->prio), ready);
	}

	if (*queue)
		(*queue)->block.delta -= ms;
}

#include "../../user/process/process.h"
#include "../include/cmsdk.h"
#include "../include/irq.h"
#include "../include/log.h"
#include "../include/memory.h"
#include "../include/process.h"
#include <stdint.h>
#include <strings.h>

// diagnostic helper for process data
void process_print(process* proc) {
	kprintf("Process %s\n", proc->name);
	kprintf("State: %d; Priority: %d\n", proc->state, proc->prio);
	kprintf("spart: %p, epart: %p, estack: %p\n"
			"sdata: %p, edata: %p\n"
			"sbss:  %p, ebss:  %p\n\n",
			proc->name, proc->spart, proc->epart, proc->estack,
			proc->sdata, proc->edata, proc->sbss, proc->ebss);
}

/* process queue */

// push to process queue
void process_queue_push(process_queue *queue, process *proc) {
	while (*queue)
		queue = &(*queue)->next;

	proc->next = 0;
	*queue = proc;
}

// pop from process queue
process *process_queue_pop(process_queue *queue) {
	if (!*queue)
		return 0;

	process *tmp = *queue;
	*queue = tmp->next;
	tmp->next = 0;

	return tmp;
}

process_queue high_prio_process_queue = 0;
process_queue low_prio_process_queue = 0;
process_queue idle_process_queue = 0;

process_queue* queue_from_prio(process_priority prio) {
	switch(prio) {
		case PRIO_HIGH: return &high_prio_process_queue;
		case PRIO_LOW: return &low_prio_process_queue;
		case PRIO_IDLE: return &idle_process_queue;
	}

	return 0;
}

process *current = 0;

/* process table */

// actual system process table
#define MAX_PROCESSES 10
process process_table[MAX_PROCESSES];

// asm hooks
extern void init_process_asm(uintptr_t estack, int (*entry)(void));

// initalization routines, in startup code
extern void Data_Init(uintptr_t start, uintptr_t end, const uintptr_t source);
extern void Bss_Init(uintptr_t start, uintptr_t end);

void init_stack(process *proc) {
	uint32_t *frame = (uint32_t *)proc->estack - 8;
	frame[0] = 0;						// r0
	frame[1] = 0;						// r1
	frame[2] = 0;						// r2
	frame[3] = 0;						// r3
	frame[4] = 0;						// r12
	frame[5] = (uintptr_t)exit_process; // lr
	frame[6] = (uintptr_t)proc->entry;	// pc (thumb)
	frame[7] = XPSR_THUMB;				// xpsr

	// r4 - r11
	uint32_t *regs = frame - 8;
	for (int i = 0; i < 8; i++) {
		regs[i] = 0;
	}

	proc->sp = (uintptr_t)regs;
}

void init_partition(process *proc) {
	// enable partition
	mem_region_set(1,						  // region 1 (process)
				   (uint32_t)proc->spart,	  // base of process
				   proc->epart - proc->spart, // size of process
				   MPU_AP_RW_RW,			  // full read/write
				   0,						  // no executable
				   0						  // no srd
	);
}

void init_process(process *proc) {
	kprintf("Initializing: ");
	process_print(proc);

	// init data regions
	Data_Init(proc->sdata, proc->edata, proc->sidata);
	Bss_Init(proc->sbss, proc->ebss);

	// set process state
	proc->state = PROC_READY;

	// init process stack
	init_stack(proc);
}

process *scheduler_next(void);
void init_processes() {
	// initialize process table
	init_process_table(process_table);

	// initialize processes
	for (unsigned int i = 0; i < NUM_PROCESSES; i++) {
		process *proc = &process_table[i];
		init_process(proc);

		process_queue* queue = queue_from_prio(proc->prio);
		if(!queue) continue;
		process_queue_push(queue, proc);
	}

	// prepare first process to execute
    current = scheduler_next();
	if(!current) {
		kprintf("No processes to schedule at startup!\n");
		kpanic();
	}

	init_partition(current);
	current->state = PROC_RUNNING;
}

/* exit */

void cleanup_process() {
	kprintf("\nTerminating: ");
	process_print(current);

	current->state = PROC_TERMINATED;
}

/* scheduler */

// selects next process to schedule
process *scheduler_next(void) {
    if (high_prio_process_queue)
        return process_queue_pop(&high_prio_process_queue);

    if (low_prio_process_queue)
        return process_queue_pop(&low_prio_process_queue);

    if (idle_process_queue)
        return process_queue_pop(&idle_process_queue);

    return 0;
}

void scheduler() {
	uint32_t saved = irq_save();

	// push current process to queue if running
	if (current->state == PROC_RUNNING) {
		current->state = PROC_READY;
		process_queue_push(queue_from_prio(current->prio), current);
	}

	// schedule next process
    current = scheduler_next();
	if(!current) {
		kprintf("No processes to schedule at scheduler!\n");
		kpanic();
	}

	// prepare process to execute
	init_partition(current);
	current->state = PROC_RUNNING;

	irq_restore(saved);
}

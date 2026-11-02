#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

/* process table */

// process states
typedef enum {
	PROC_RUNNING,
	PROC_READY,
	PROC_BLOCKED,
	PROC_TERMINATED
} process_state;

// process descriptor
typedef struct process {
	// stack pointer
	uint32_t sp;

	// short name
	const char *name;

	// process state
	process_state state;

	// processor block conditino
	union {
		unsigned int delta; // delay time delta
		char* rx_buf;       // buffer for serial receiver (first char)
	} block;

	// entry point
	int (*entry)(void);

	// partition bounds
	uintptr_t spart;
	uintptr_t epart;

	// stack bounds
	uintptr_t estack;

	// data bounds
	uintptr_t sidata;
	uintptr_t sdata;
	uintptr_t edata;

	// bss bounds
	uintptr_t sbss;
	uintptr_t ebss;

	// next process in queue
	struct process *next;
} process;

// initializes all processes
void init_processes();

// jump to first process
extern void start_process();

// exit process
extern void exit_process();

// post hook for process cleanup
void cleanup_process();

// schedules next process and jumps to it
void scheduler();

// stores current process registers into process stack;
// updates stack pointer and returns it
extern uint32_t context_out();

// restores state from current process stack, jumps to it
extern void context_in();

/* process queue */

// process queue definition
typedef process *process_queue;

// scheduler process queue
extern process_queue scheduler_process_queue;

// current process
extern process *current;

// push to process queue
void process_queue_push(process_queue *queue, process *proc);

// pop from process queue
process *process_queue_pop(process_queue *queue);

#endif

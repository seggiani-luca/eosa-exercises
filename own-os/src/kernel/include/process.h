#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

// process descriptor
typedef struct {
	// short name
	const char *name;

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
} process;

// initializes process memory
void init_process(process *proc);

// starts a process, from thread mode
void start_process(process *proc);

// starts a process, from handler mode
void do_start_process(process *proc);

// exits from a process, from thread mode
void exit_process();

// exits from a process, from handler mode
void do_exit_process();

#endif

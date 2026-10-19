#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

// process descriptor
typedef struct {
	// stack pointer
	uint32_t sp;

	// short name
	const char *name;

	// entry point
	int (*entry)(void);

	// state
	int running;

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

// initializes all processes
void init_processes();

// jump to first process
void start_process();

// exit process 
void exit_process();

// schedules next process and jumps to it
void scheduler();

// context switching
extern uint32_t context_out();
extern void context_in(uint32_t sp);

#endif

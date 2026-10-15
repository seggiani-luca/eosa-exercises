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
	uint32_t *spart;
	uint32_t *epart;

	// stack bounds
	uint32_t *estack;

	// data bounds
	uint32_t *sidata;
	uint32_t *sdata;
	uint32_t *edata;

	// bss bounds
	uint32_t *sbss;
	uint32_t *ebss;
} process;

// starts a process, from thread mode
void start_process(process *proc);

// starts a process, from handler mode
void do_start_process(process *proc);

// exits from a process, from thread mode
void exit_process();

// exits from a process, from handler mode
void do_exit_process();

#endif

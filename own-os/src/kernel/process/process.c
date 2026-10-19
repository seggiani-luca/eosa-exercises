#include "../../user/process/process.h"
#include "../include/process.h"
#include "../include/cmsdk.h"
#include "../include/irq.h"
#include "../include/memory.h"
#include "../include/control.h"
#include <stdint.h>
#include <strings.h>

/* process table */

// actual system process table
#define MAX_PROCESSES 10
process process_table[MAX_PROCESSES];

// current process index
unsigned int current = 0;

// asm hooks
extern void init_process_asm(uintptr_t estack, int (*entry)(void));

// initalization routines, in startup code
extern void Data_Init(uintptr_t start, uintptr_t end, const uintptr_t source);
extern void Bss_Init(uintptr_t start, uintptr_t end);

void init_stack(process *proc) {
    uint32_t *frame = (uint32_t*)proc->estack - 8;
    frame[0] = 0; // r0
    frame[1] = 0; // r1
    frame[2] = 0; // r2
    frame[3] = 0; // r3
    frame[4] = 0; // r12
    frame[5] = (uintptr_t)exit_process; // lr
    frame[6] = (uintptr_t) proc->entry; // pc (thumb)
    frame[7] = XPSR_THUMB; // xpsr 

	// r4 - r11
    uint32_t *regs = frame - 8;
    for ( int i = 0; i < 8; i++ )
    {
        regs[i] = 0;
    }

    proc->sp = (uintptr_t)regs;   
}

void init_process(process *proc) {
	// init data regions
	Data_Init(proc->sdata, proc->edata, proc->sidata);
	Bss_Init(proc->sbss, proc->ebss);

	// set process state
	proc->running = 1;

	// init process stack
	init_stack(proc);
}

void init_processes() {
	// initialize process table
	init_process_table(process_table);

	// initialize processes
	for (unsigned int i = 0; i < NUM_PROCESSES; i++) {
		process *proc = &process_table[i];
		init_process(proc);
	}

	// initialize first memory region
	process* proc = &process_table[current];
	mem_region_set(1,						  // region 1 (process)
				   (uint32_t)proc->spart,	  // base of process
				   proc->epart - proc->spart, // size of process
				   MPU_AP_RW_RW,			  // full read/write
				   0,						  // no executable
				   0						  // no srd
	);
}

/* exiting */

void exit_process_post() {
	// TODO
}

/* scheduler */

void scheduler() {
	uint32_t saved = irq_save();

	// schedule (round robin)
	do {
		current = (current + 1) % NUM_PROCESSES;
	} while (!process_table[current].running);

	// enable partition
	process* proc = &process_table[current];
	mem_region_set(1,						  // region 1 (process)
				   (uint32_t)proc->spart,	  // base of process
				   proc->epart - proc->spart, // size of process
				   MPU_AP_RW_RW,			  // full read/write
				   0,						  // no executable
				   0						  // no srd
	);

	irq_restore(saved);
}

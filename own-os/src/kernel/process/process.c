#include "../include/process.h"
#include "../include/cmsdk.h"
#include "../include/log.h"
#include "../include/memory.h"

// current process
static process *current = 0;

// asm hooks
extern void start_process_asm(uintptr_t estack, int (*entry)(void));
extern void exit_process_asm();

// initalization routines, in startup code
extern void Data_Init(uintptr_t start, uintptr_t end, const uintptr_t source);
extern void Bss_Init(uintptr_t start, uintptr_t end);

void init_process(process *proc) {
	Data_Init(proc->sdata, proc->edata, proc->sidata);
	Bss_Init(proc->sbss, proc->ebss);
}

void do_start_process(process *proc) {
	kprintf("Starting process %s...\n", proc->name);
	current = proc;

	// setup MPU
	mem_region_set(1,						  // region 1 (process)
				   (uint32_t)proc->spart,	  // base of process
				   proc->epart - proc->spart, // size of process
				   MPU_AP_RW_RW,			  // full read only
				   0,						  // no executable
				   0						  // no srd
	);

	start_process_asm(proc->estack, proc->entry);
}

void do_exit_process() {
	kprintf("Exiting process %s...\n", current->name);
	current = 0;

	// reset MPU
	mem_region_reset(1);

	exit_process_asm();
}

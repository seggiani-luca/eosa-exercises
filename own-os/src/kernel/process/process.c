#include "../include/process.h"
#include "../include/log.h"

// current process
static process *current = 0;

// asm hooks
extern void start_process_asm(uint32_t *estack, int (*entry)(void));
extern void exit_process_asm();

void do_start_process(process *proc) {
	kprintf("Starting process %s...\n", proc->name);
	current = proc;

	start_process_asm(proc->estack, proc->entry);
}

void do_exit_process() {
	kprintf("Exiting process %s...\n", current->name);
	current = 0;

	exit_process_asm();
}

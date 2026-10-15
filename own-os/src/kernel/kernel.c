#include "../user/process/process.h"
#include "include/serial.h"
#include "include/systick.h"
#include "include/wfi.h"

// initalization routines, in startup
extern void Data_Init(uint32_t *start, uint32_t *end, const uint32_t *source);
extern void Bss_Init(uint32_t *start, uint32_t *end);

void init_process(process *proc) {
	Data_Init(proc->sdata, proc->edata, proc->sidata);
	Bss_Init(proc->sbss, proc->ebss);
}

int kernel_main(void) {
	// allow devices over SVC
	SCB->SHPR[7] = 0xE0;

	// init SysTick
	systick_init();

	// init serial I/O
	uart_init();

	// greet the user
	kprintf("\nBooting into own-os v.0.0\n");
	kprintf("Luca Seggiani - 2026\n\n");

	// initialize process table
	init_processes();

	// initialize processes
	for (unsigned int i = 0; i < NUM_PROCESSES; i++) {
		process *proc = &process_table[i];

		// initialize and start
		init_process(proc);
		start_process(proc);
	}

	for(;;) wfi();
}

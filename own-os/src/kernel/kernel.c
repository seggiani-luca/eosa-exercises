#include "../user/process/process.h"
#include "include/cmsdk.h"
#include "include/log.h"
#include "include/memory.h"
#include "include/serial.h"
#include "include/systick.h"
#include "include/timer.h"
#include "include/watchdog.h"
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

	// init SysTick timer
	systick_init(0, 10);

	// init other timers
	timer_init(timer0_driver.ctx, 10);
	timer_init(timer1_driver.ctx, 2000);

	// init serial interfaces
	uart_init(uart0_driver.ctx);
	uart_init(uart1_driver.ctx);
	uart_init(uart2_driver.ctx);

	// init watchdog
	watchdog_init(10000);

	// setup MPU
	mem_init();
	mem_region_set(0,			 // region 0 (FLASH)
				   0x00000000,	 // base is 0x00000000
				   0x00400000,	 // 4 MiB
				   MPU_AP_RO_RO, // full read only
				   1,			 // executable
				   0			 // no srd
	);

	// greet the user
	kprintf("\nBooting into own-os v.0.0\n");
	kprintf("Luca Seggiani - 2026\n\n");

	// initialize process table
	init_process_table();

	// initialize processes
	for (unsigned int i = 0; i < NUM_PROCESSES; i++) {
		process *proc = &process_table[i];

		// initialize and start
		init_process(proc);
		start_process(proc);
	}

	for (;;)
		wfi();
}

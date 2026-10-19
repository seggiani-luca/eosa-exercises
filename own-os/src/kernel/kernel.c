#include "include/cmsdk.h"
#include "include/handler.h"
#include "include/log.h"
#include "include/memory.h"
#include "include/process.h"
#include "include/serial.h"
#include "include/systick.h"
#include "include/timer.h"
#include "include/watchdog.h"
#include "include/wfi.h"

int kernel_main(void) {
	// initialize fault and SVC handlers
	handler_init();

	// init terminal serial interface
	uart_init(uart0_driver.ctx);

	// greet the user
	kprintf("\nBooting into own-os v.0.0\n");
	kprintf("Luca Seggiani - 2026\n\n");

	// init all timers
	timer_init(timer0_driver.ctx, 10);
	timer_init(timer1_driver.ctx, 2000);

	// init all serial interfaces
	uart_init(uart1_driver.ctx);
	uart_init(uart2_driver.ctx);

	// init watchdog
	watchdog_init(10000);

	// setup main MPU region
	mem_init();
	mem_region_set(0,			 // region 0 (FLASH)
				   0x00000000,	 // base is 0x00000000
				   0x00400000,	 // 4 MiB
				   MPU_AP_RO_RO, // full read only
				   1,			 // executable
				   0			 // no srd
	);

	// initialize processes
	init_processes();

	// init SysTick timer now, as it starts round robin
	systick_init(0, 100);
	start_process();

	for (;;)
		wfi();
}

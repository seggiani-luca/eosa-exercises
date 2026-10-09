#include "include/control.h"
#include "include/serial.h"
#include "include/systick.h"

extern int main(void);

int kernel_main(void) {
	// init SysTick
	systick_init();

	// init serial I/O
	uart_init();

	// greet the user
	kprintf("\nBooting into own-os v.0.0\n");
	kprintf("Luca Seggiani - 2026\n\n");

	// lower privilege
	priv_down();

	// jump to userspace
	main();

	return 0;
}

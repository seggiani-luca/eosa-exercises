#include "../include/lib.h"
PROCESS_PRIORITY(PRIO_HIGH)

int process_a_main(void) {
	pin_mode(SYS_GPIO0, 0, MODE_OUTPUT);
	digital_write(SYS_GPIO0, 0, 1);

	return 0;
}

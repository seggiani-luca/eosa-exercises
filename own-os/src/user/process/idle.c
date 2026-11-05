#include "../include/lib.h"

PROCESS_PRIORITY(PRIO_IDLE)

int idle_main(void) {
	for (;;)
		yield();

	// never reached
	return 0;
}

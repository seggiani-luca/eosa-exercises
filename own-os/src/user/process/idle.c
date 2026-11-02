#include "../../kernel/include/wfi.h"

int idle_main(void) {
	for (;;)
		wfi();

	// never reached
	return 0;
}

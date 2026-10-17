#include "../../../kernel/include/wfi.h"
#include "../../include/lib.h"

#define SLEEP_TIMER SYS_SYSTICK

void sleep(unsigned int ms) {
	unsigned int now = tick(SLEEP_TIMER);
	while (tick(SLEEP_TIMER) - now < ms)
		wfi();
}

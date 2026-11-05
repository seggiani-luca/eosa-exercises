#include "../include/lib.h"
extern void kprintf(const char *fmt, ...);

PROCESS_PRIORITY(PRIO_HIGH)

extern char b_buf[];

int process_a_main(void) {
	fwrite(SYS_SERIAL0, b_buf, 10);
	fread(SYS_SERIAL0, b_buf, 10);

	yield();

	return 0;
}

#include "../include/lib.h"

PROCESS_PRIORITY(PRIO_HIGH)

#define BUF_SIZE 256
char b_buf[BUF_SIZE];

int process_b_main(void) {
	b_buf[0] = '?';

	sleep(1000);

	printf("%s\n", b_buf);

	return 0;
}

#include "../include/lib.h"

int process_b_main(void) {
	int i = 0;
	for (;;) {
		printf("Sono il processo B, secondo %d!\n", i++);
		sleep(1000);
	}

	return 0;
}

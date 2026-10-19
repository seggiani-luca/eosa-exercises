#include "../include/lib.h"

int process_a_main(void) {
	int i = 0;
	for (;;) {
		printf("sono il processo A, secondo %d!\n", i++);
		sleep(1000);
	}

	return 0;
}

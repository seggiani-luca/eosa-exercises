#include "../include/lib.h"
PROCESS_PRIORITY(PRIO_HIGH)

int process_b_main(void) {
	printf("Ciao dal processo B!\n");

	printf("Processo B si addormenta fino a 3000 ms\n");
	until(SYS_SYSTICK, 3000);

	printf("Ciao da B!\n");

	return 0;
}

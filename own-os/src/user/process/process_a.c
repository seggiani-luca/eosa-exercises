#include "../include/lib.h"

#define BUF_SIZ 128

int process_a_main(void) {
	printf("Ciao dal processo A!\n");

	printf("Processo A si addormenta per 1000 ms\n");
	sleep(1000);

	printf("Processo A chiede: come ti chiami?\n");

	char name_buf[BUF_SIZ];
	gets(name_buf, BUF_SIZ);

	printf("Ciao da A, %s!\n", name_buf);

	return 0;
}

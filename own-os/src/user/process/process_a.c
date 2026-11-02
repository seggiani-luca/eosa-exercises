#include "../include/lib.h"

#define BUF_SIZ 128

int process_a_main(void) {
	printf("Ciao dal processo A!\n");
	printf("Processo A chiede: come ti chiami?\n");

	char name_buf[BUF_SIZ];
	gets(name_buf, BUF_SIZ);

	printf("Ciao, %s!\n", name_buf);

	return 0;
}

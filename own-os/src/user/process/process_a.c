#include "../include/lib.h"

int process_a_main(void) {
	fprintf(SYS_SERIAL0, "Ciao seriale 0!\n");
	fprintf(SYS_SERIAL1, "Ciao seriale 1!\n");
	fprintf(SYS_SERIAL2, "Ciao seriale 2!\n");

	return 0;
}

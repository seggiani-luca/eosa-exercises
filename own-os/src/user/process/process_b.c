#include "../include/lib.h"

int process_b_main(void) {
	int a = 3;
	printf("Ciao dal processo B!\n");
	printf("Il mio stack inizia a %x (%d)\n", &a, a);
	;

	return 0;
}

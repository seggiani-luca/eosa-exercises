#include "../include/lib.h"

int process_a_main(void) {
	int a = 2;
	printf("Ciao dal processo A!\n");
	printf("Il mio stack inizia a %x (%d)\n", &a, a);
	;

	return 0;
}

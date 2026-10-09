#include "../include/semihost.h"

char_driver shst_driver = {
	.init = 0,
	.putc = shst_putc,
	.getc = shst_getc,
};

void shst_putc(char c) { shst_call(SYS_WRITEC, &c); }

int shst_getc() { return shst_call(SYS_READC, 0); }

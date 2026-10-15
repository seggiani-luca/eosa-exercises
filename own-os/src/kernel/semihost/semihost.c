#include "../include/semihost.h"

char_driver shst_driver = {
	.init = 0,
	.write = shst_write,
	.read = shst_read,
};

void shst_putc(char c) { shst_call(SHST_WRITEC, &c); }

int shst_getc() { return shst_call(SHST_READC, 0); }

unsigned int shst_write(const char *buf, unsigned int len) {
	for (int i = 0; i < len; i++)
		shst_putc(buf[i]);
	return len;
}

unsigned int shst_read(char *buf, unsigned int len) {
	unsigned int i;

	for (i = 0; i < len; i++) {
		int c = shst_getc();

		if (c < 0)
			return i;

		buf[i] = (char)c;

		if (c == '\n' || c == '\r')
			return i + 1;
	}

	return i;
}

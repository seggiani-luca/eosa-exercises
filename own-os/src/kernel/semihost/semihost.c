#include "../include/semihost.h"

char_driver shst_driver = {
	.init = 0,
	.write = shst_write,
	.read = shst_read,
	.putc = shst_putc,
	.getc = shst_getc,
	.ctx = 0
};

void shst_putc(void* ctx, char c) { shst_call(SHST_WRITEC, &c); }

int shst_getc(void* ctx) { return shst_call(SHST_READC, 0); }

unsigned int shst_write(void* ctx, const char *buf, unsigned int len) {
	for (int i = 0; i < len; i++)
		shst_putc(ctx, buf[i]);
	return len;
}

unsigned int shst_read(void* ctx, char *buf, unsigned int len) {
	unsigned int i;

	for (i = 0; i < len; i++) {
		int c = shst_getc(ctx);

		if (c < 0)
			return i;

		buf[i] = (char)c;

		if (c == '\n' || c == '\r')
			return i + 1;
	}

	return i;
}

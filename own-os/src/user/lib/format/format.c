#include "../../include/lib.h"

void sformat(vputc out, void *ctx, const char *s) {
	char c;
	while ((c = *(s++)))
		out(ctx, c);
}

void uformat(vputc out, void *ctx, unsigned int n) {
	char buf[16];
	int i = 0;

	do {
		int r = n % 10;
		buf[i++] = r + '0';

		n = n / 10;
	} while (n != 0);

	for (int j = i - 1; j >= 0; j--)
		out(ctx, buf[j]);
}

void iformat(vputc out, void *ctx, int n) {
	if (n < 0) {
		out(ctx, '-');
		uformat(out, ctx, (unsigned int)(-n));
		out(ctx, -(unsigned int)n);
	} else {
		uformat(out, ctx, (unsigned int)n);
	}
}

void xformat(vputc out, void *ctx, uint32_t w) {
	sformat(out, ctx, "0x");

	char buf[8];
	for (int i = 0; i < 8; i++) {
		char v = w & 0xF;
		if (v >= 10)
			buf[i] = v - 10 + 'A';
		else
			buf[i] = v + '0';

		w >>= 4;
	}

	for (int i = 7; i >= 0; i--)
		out(ctx, buf[i]);
}

void vformat(vputc out, void *ctx, const char *fmt, va_list ap) {
	char c;
	while ((c = *fmt)) {
		fmt++;

		if (c != '%') {
			out(ctx, c);
			continue;
		}

		switch (*fmt++) {
		case 'c':
			out(ctx, va_arg(ap, int));
			break;

		case 's':
			sformat(out, ctx, va_arg(ap, const char *));
			break;

		case 'd':
		case 'i':
			iformat(out, ctx, va_arg(ap, int));
			break;

		case 'x':
			xformat(out, ctx, va_arg(ap, uint32_t));
			break;

		case 'u':
			uformat(out, ctx, va_arg(ap, unsigned int));
			break;

		case '%':
			out(ctx, '%');
			break;
		}
	}
}

int lformat(vgetc in, vputc out, vflush flush, void *ctx, char *buf, int siz) {
	int i = 0;

	if (siz <= 0)
		return 0;

	while (i < siz - 1) {
		int c = in(ctx);

		if (c < 0)
			break;

		if (c == '\r' || c == '\n') {
			out(ctx, '\r');
			out(ctx, '\n');
			flush(ctx);
			break;
		}

		if (c == '\b' || c == 127) {
			if (i > 0) {
				i--;
				out(ctx, '\b');
				out(ctx, ' ');
				out(ctx, '\b');
				flush(ctx);
			}
			continue;
		}

		if (c >= 32 && c <= 126) {
			buf[i++] = (char)c;
			out(ctx, (char)c);
			flush(ctx);
		}
	}

	buf[i] = '\0';
	return i;
}

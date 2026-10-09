#include "../include/stdio.h"

void io_puts(char_driver *dev, const char *s) {
	char c;
	while ((c = *(s++)))
		dev->putc(c);
}

void io_putu(char_driver *dev, unsigned int n) {
	char buf[16];
	int i = 0;

	do {
		int r = n % 10;
		buf[i++] = r + '0';

		n = n / 10;
	} while (n != 0);

	for (int j = i - 1; j >= 0; j--)
		dev->putc(buf[j]);
}

void io_puti(char_driver *dev, int n) {
	if (n < 0) {
		dev->putc('-');
		io_putu(dev, -(unsigned int)n);
	} else {
		io_putu(dev, (unsigned int)n);
	}
}

void io_vprintf(char_driver *dev, const char *fmt, va_list ap) {
	char c;
	while ((c = *fmt)) {
		fmt++;

		if (c != '%') {
			dev->putc(c);
			continue;
		}

		switch (*fmt++) {
		case 'c':
			dev->putc(va_arg(ap, int));
			break;

		case 's':
			io_puts(dev, va_arg(ap, const char *));
			break;

		case 'd':
		case 'i':
			io_puti(dev, va_arg(ap, int));
			break;

		case 'u':
			io_putu(dev, va_arg(ap, unsigned int));
			break;

		case '%':
			dev->putc('%');
			break;
		}
	}
}

int io_gets(char_driver *dev, char *buf, int siz) {
	int i = 0;

	if (siz <= 0)
		return 0;

	while (i < siz - 1) {
		int c = dev->getc();

		if (c == '\r' || c == '\n') {
			io_puts(dev, "\r\n");
			break;
		}

		if (c == '\b' || c == 127) {
			if (i > 0) {
				i--;
				io_puts(dev, "\b \b");
			}
			continue;
		}

		if (c >= 32 && c <= 126) {
			buf[i++] = (char)c;
			dev->putc(c);
		}
	}

	buf[i] = '\0';
	return i;
}

int atou(const char *s) {
	unsigned int n = 0;

	while (*s >= '0' && *s <= '9')
		n = n * 10 + (*s++ - '0');
	return n;
}

int atoi(const char *s) {
	if (*s == '-')
		return -atou(++s);
	else
		return atou(s);
}

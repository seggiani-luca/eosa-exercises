#include "../include/semihost.h"
#include <stdarg.h>

void sh_putc(char c) {
	sh_call(SYS_WRITEC, &c);
}

void sh_puts(const char* s) {
	sh_call(SYS_WRITE0, (char*)s);
}

void sh_putu(unsigned int n) {
	char buf[16];
	int i = 0;

	do {
		int r = n % 10;
		buf[i++] = r + '0';

		n = n / 10;
	} while(n != 0);

	for(int j = i - 1; j >= 0; j--) sh_putc(buf[j]);
}

void sh_puti(int n) {
	if(n < 0) {
		sh_putc('-');
		sh_putu(-(unsigned int)n);
	} else {
		sh_putu((unsigned int)n);
	}
}

void sh_printf(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	char c;
	while((c = *fmt)) {
		fmt++;
		
		if(c != '%') {
			sh_putc(c);
			continue;
		}

        switch (*fmt++) {
			case 'c':
				sh_putc(va_arg(ap, int));
				break;

			case 's':
				sh_puts(va_arg(ap, const char *));
				break;

			case 'd':
			case 'i':
				sh_puti(va_arg(ap, int));
				break;

			case 'u':
				sh_putu(va_arg(ap, unsigned int));
				break;

			case '%':
				sh_putc('%');
				break;
		}
	}

	va_end(ap);
}

int sh_getc() {
	return sh_call(SYS_READC, 0);
}

int sh_gets(char* buf, int siz) {
	int i = 0;

	while(i < siz - 1) {
		int c = sh_getc();

		if(c == '\r' || c == '\n') break;
		
		if(c == '\b' || c == 127) {
			if(i <= 0) continue;

			i--;
			sh_puts("\b \b");
			continue;
		}

		buf[i++] = c;
		sh_putc(c);
	}

	buf[i] = '\0';

	sh_putc('\n');
	return i;
}

int atou(const char* s) {
	unsigned int n = 0;

	while(*s >= '0' && *s <= '9') n = n * 10 + (*s++ - '0');
	return n;
}

int atoi(const char* s) {
	if(*s == '-') return -atou(++s);
	else return atou(s);
}

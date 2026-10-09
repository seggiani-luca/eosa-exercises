#include "include/lib.h"

#include <stdarg.h>

void fputc(int handle, char c) { SYSCALL(SYS_PUTC); }

int fgetc(int handle) {
	SYSCALL(SYS_GETC);
	return r0;
}

void fputs(int handle, const char *s) { SYSCALL(SYS_PUTC); }

void fputu(int handle, unsigned int n) { SYSCALL(SYS_PUTU); }

void fputi(int handle, int n) { SYSCALL(SYS_PUTI); }

void fprintf(int handle, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	register va_list *r2 __asm__("r2") = &ap;

	SYSCALL(SYS_VPRINTF);

	va_end(ap);
}

int fgets(int handle, char *buf, int siz) { SYSCALL(SYS_GETS); }

int tick() {
	SYSCALL(SYS_TICK);
	return r0;
}

#include "../include/log.h"

#include "../../user/include/lib.h"
#include "../include/serial.h"
#include <stdarg.h>

// user stdio putc function
void kern_putc(void *ctx, char c) { uart_putc(c); }

void kprintf(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	vformat(kern_putc, 0, fmt, ap);

	va_end(ap);
}

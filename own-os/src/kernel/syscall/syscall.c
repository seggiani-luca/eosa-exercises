#include "../../syscall.h"

#include "../include/semihost.h"
#include "../include/serial.h"
#include "../include/stdio.h"
#include "../include/systick.h"
#include <stdint.h>

char_driver *get_char_dev(int dev) {
	switch (dev) {
	case SYS_SERIAL:
		return &uart_driver;
	case SYS_SEMIHOST:
		return &shst_driver;
	default:
		kprintf("Invalid character device");
		return 0;
	}
}

int putc_syscall(int dev, int c) {
	char_driver *driver = get_char_dev(dev);
	if (!dev)
		return -1;

	driver->putc(c);
	return 0;
}

int getc_syscall(int dev) {
	char_driver *driver = get_char_dev(dev);
	if (!dev)
		return -1;

	return driver->getc();
}

int puts_syscall(int dev, const char *s) {
	char_driver *driver = get_char_dev(dev);
	if (!dev)
		return -1;

	io_puts(driver, s);
	return 0;
}

int putu_syscall(int dev, unsigned int n) {
	char_driver *driver = get_char_dev(dev);
	if (!dev)
		return -1;

	io_putu(driver, n);
	return 0;
}

int puti_syscall(int dev, int n) {
	char_driver *driver = get_char_dev(dev);
	if (!dev)
		return -1;

	io_puti(driver, n);
	return 0;
}

int vprintf_syscall(int dev, const char *fmt, va_list *ap) {
	char_driver *driver = get_char_dev(dev);
	if (!dev)
		return -1;

	io_vprintf(driver, fmt, *ap);
	return 0;
}

int gets_syscall(int dev, char *buf, int siz) {
	char_driver *driver = get_char_dev(dev);
	if (!dev)
		return -1;

	io_gets(driver, buf, siz);
	return 0;
}

int tick_syscall() { return systick_uptime_ms(); }

typedef struct {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t lr;
	uint32_t pc;
	uint32_t xpsr;
} exception_frame;

void dispatch_syscall(exception_frame *frame, uint32_t syscall) {
	switch (syscall) {
	case SYS_PUTC:
		putc_syscall(frame->r0, frame->r1);
		break;

	case SYS_GETC:
		frame->r0 = getc_syscall(frame->r0);
		break;

	case SYS_PUTS:
		puts_syscall(frame->r0, (const char *)frame->r1);
		break;

	case SYS_PUTU:
		putu_syscall(frame->r0, frame->r1);
		break;

	case SYS_PUTI:
		puti_syscall(frame->r0, frame->r1);
		break;

	case SYS_VPRINTF:
		vprintf_syscall(frame->r0, (const char *)frame->r1,
						(va_list *)frame->r2);
		break;

	case SYS_GETS:
		gets_syscall(frame->r0, (char *)frame->r1, frame->r2);
		break;

	case SYS_TICK:
		frame->r0 = tick_syscall();
		break;

	default:
		kprintf("Invalid system call");
	}
}

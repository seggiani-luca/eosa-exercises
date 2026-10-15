#include "../../syscall.h"

#include "../include/process.h"
#include "../include/semihost.h"
#include "../include/serial.h"
#include "../include/systick.h"
#include <stdint.h>

char_driver *get_char_dev(int dev) {
	switch (dev) {
	case SYS_SERIAL:
		return &uart_driver;
	case SYS_SEMIHOST:
		return &shst_driver;
	default:
		kprintf("Invalid character device\n");
		return 0;
	}
}

unsigned int write_syscall(int dev, const char *buf, unsigned int len) {
	char_driver *driver = get_char_dev(dev);
	if (!driver)
		return -1;

	return driver->write(buf, len);
	return 0;
}

unsigned int read_syscall(int dev, char *buf, unsigned int len) {
	char_driver *driver = get_char_dev(dev);
	if (!driver)
		return -1;

	return driver->read(buf, len);
}

int tick_syscall() { return systick_uptime_ms(); }

void start_proc_syscall(process *proc) { do_start_process(proc); }

void exit_proc_syscall() { do_exit_process(); }

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

uint32_t dispatch_syscall(exception_frame *frame, uint32_t syscall,
						  uint32_t exc_return) {
	switch (syscall) {
	case SYS_WRITE:
		frame->r0 =
			write_syscall(frame->r0, (const char *)frame->r1, frame->r2);
		return exc_return;

	case SYS_READ:
		frame->r0 = read_syscall(frame->r0, (char *)frame->r1, frame->r2);
		return exc_return;

	case SYS_TICK:
		frame->r0 = tick_syscall();
		return exc_return;

	case SYS_START_PROC:
		start_proc_syscall((process *)frame->r0);
		return 0xFFFFFFFD; // thread mode, PSP

	case SYS_EXIT_PROC:
		exit_proc_syscall();
		return 0xFFFFFFF9; // thread mode, MSP

	default:
		kprintf("Invalid system call\n");
		frame->r0 = (uint32_t)-1;
		return exc_return;
	}
}

#include "../../syscall.h"
#include "../include/log.h"
#include "../include/process.h"
#include "../include/semihost.h"
#include "../include/serial.h"
#include "../include/systick.h"
#include "../include/timer.h"
#include <stdint.h>

typedef uintptr_t syscall_arg;
typedef uint32_t syscall_ret;

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

char_driver *get_char_dev(int dev) {
	switch (dev) {
	case SYS_SERIAL0:
		return &uart0_driver;
	case SYS_SERIAL1:
		return &uart1_driver;
	case SYS_SERIAL2:
		return &uart2_driver;
	case SYS_SEMIHOST:
		return &shst_driver;
	default:
		kprintf("Invalid character device\n");
		return 0;
	}
}

timer_driver *get_timer_dev(int dev) {
	switch (dev) {
	case SYS_SYSTICK:
		return &systick_driver;
	case SYS_TIMER0:
		return &timer0_driver;
	case SYS_TIMER1:
		return &timer1_driver;
	default:
		kprintf("Invalid timer device\n");
		return 0;
	}
}

syscall_ret syscall_write(syscall_arg dev, syscall_arg buf, syscall_arg len) {
	char_driver *driver = get_char_dev(dev);
	if (!driver)
		return -1;

	return driver->write(driver->ctx, (const char *)buf, len);
}

syscall_ret syscall_read(syscall_arg dev, syscall_arg buf, syscall_arg len) {
	char_driver *driver = get_char_dev(dev);
	if (!driver)
		return -1;

	return driver->read(driver->ctx, (char *)buf, len);
}

syscall_ret syscall_tick(syscall_arg dev,
						 syscall_arg arg1 __attribute__((unused)),
						 syscall_arg arg2 __attribute__((unused))) {
	timer_driver *driver = get_timer_dev(dev);
	if (!driver)
		return -1;

	return driver->tick(driver->ctx);
}

syscall_ret syscall_start_proc(syscall_arg proc,
							   syscall_arg b __attribute__((unused)),
							   syscall_arg c __attribute__((unused))) {
	do_start_process((process *)proc);
	return 0;
}

syscall_ret syscall_exit_proc(syscall_arg arg0 __attribute__((unused)),
							  syscall_arg arg1 __attribute__((unused)),
							  syscall_arg arg2 __attribute__((unused))) {
	do_exit_process();
	return 0;
}

// syscall table
typedef syscall_ret (*syscall_fn)(syscall_arg, syscall_arg, syscall_arg);
static const syscall_fn syscall_table[] = {
	[SYS_WRITE] = syscall_write,		 [SYS_READ] = syscall_read,
	[SYS_TICK] = syscall_tick,			 [SYS_START_PROC] = syscall_start_proc,
	[SYS_EXIT_PROC] = syscall_exit_proc,
};
#define SYSCALL_TABLE_SIZE (sizeof(syscall_table) / sizeof(syscall_fn))

uint32_t dispatch_syscall(exception_frame *frame, uint32_t syscall,
						  syscall_ret exc_return) {
	if (syscall < 0 || syscall >= SYSCALL_TABLE_SIZE) {
		kprintf("Invalid system call\n");
		frame->r0 = (uint32_t)-1;
		return exc_return;
	}

	frame->r0 = syscall_table[syscall](frame->r0, frame->r1, frame->r2);

	if (syscall == SYS_START_PROC)
		return 0xFFFFFFFD;
	if (syscall == SYS_EXIT_PROC)
		return 0xFFFFFFF9;
	return exc_return;
}

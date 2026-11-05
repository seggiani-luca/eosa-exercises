#include "../../syscall.h"
#include "../include/atomic.h"
#include "../include/control.h"
#include "../include/delay.h"
#include "../include/log.h"
#include "../include/process.h"
#include "../include/semihost.h"
#include "../include/serial.h"
#include "../include/systick.h"
#include "../include/timer.h"
#include <stdint.h>
#include <strings.h>

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

// get character device from id
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
		return 0;
	}
}

// get timer device from id
timer_driver *get_timer_dev(int dev) {
	switch (dev) {
	case SYS_SYSTICK:
		return &systick_driver;
	case SYS_TIMER0:
		return &timer0_driver;
	case SYS_TIMER1:
		return &timer1_driver;
	default:
		return 0;
	}
}

// get timer queue from id
timer_queue *get_timer_queue(int dev) {
	switch (dev) {
	case SYS_SYSTICK:
		return &systick_queue;
	case SYS_TIMER0:
		return &timer0_queue;
	case SYS_TIMER1:
		return &timer1_queue;
	default:
		return 0;
	}
}

// check buffer against process partition
int check_buf(uintptr_t buf, size_t len, process *proc) {
	// check buffer beginning
	uintptr_t beg = proc->spart;
	uintptr_t end = proc->epart;
	if (buf < beg || buf >= end)
		return 0;

	// check buffer length
	uintptr_t off = buf - beg;
	size_t size = end - beg;
	if (off > size || len > size - off)
		return 0;

	return 1;
}

syscall_ret syscall_write(syscall_arg dev, syscall_arg buf, syscall_arg len) {
	char_driver *driver = get_char_dev(dev);
	if (!driver) {
		kprintf("[kernel] invalid character device\n");
		return -1;
	}

	if (!check_buf(buf, len, current)) {
		kprintf("[kernel] invalid buffer\n");
		return -1;
	}

	return driver->write(driver->ctx, (const char *)buf, len);
}

syscall_ret syscall_read(syscall_arg dev, syscall_arg buf, syscall_arg len) {
	char_driver *driver = get_char_dev(dev);
	if (!driver) {
		kprintf("[kernel] invalid character device\n");
		return -1;
	}

	if (!check_buf(buf, len, current)) {
		kprintf("[kernel] invalid buffer\n");
		return -1;
	}

	return driver->read(driver->ctx, (char *)buf, len);
}

syscall_ret syscall_tick(syscall_arg dev,
						 syscall_arg arg1 __attribute__((unused)),
						 syscall_arg arg2 __attribute__((unused))) {
	timer_driver *driver = get_timer_dev(dev);
	if (!driver) {
		kprintf("[kernel] invalid timer device\n");
		return -1;
	}

	return driver->tick(driver->ctx);
}

syscall_ret syscall_delay(syscall_arg dev, syscall_arg ms,
						  syscall_arg arg2 __attribute__((unused))) {
	timer_queue *queue = get_timer_queue(dev);
	if (!queue) {
		kprintf("[kernel] invalid timer queue\n");
		return -1;
	}

	timer_queue_push(queue, current, ms);
	scheduler();

	return 0;
}

syscall_ret syscall_until(syscall_arg dev, syscall_arg ms,
						  syscall_arg arg2 __attribute__((unused))) {
	timer_queue *queue = get_timer_queue(dev);
	if (!queue) {
		kprintf("[kernel] invalid timer queue\n");
		return -1;
	}
	timer_driver *driver = get_timer_dev(dev);
	if (!driver) {
		kprintf("[kernel] invalid timer device\n");
		return -1;
	}

	// just sleep, only if needed
	unsigned int delta = ms - driver->tick(driver->ctx);
	if (delta > 0) {
		timer_queue_push(queue, current, ms);
		scheduler();
	}

	return 0;
}

syscall_ret syscall_yield(syscall_arg arg0 __attribute__((unused)),
						  syscall_arg arg1 __attribute__((unused)),
						  syscall_arg arg2 __attribute__((unused))) {
	scheduler();

	return 0;
}

syscall_ret syscall_start_proc(syscall_arg arg0 __attribute__((unused)),
							   syscall_arg arg1 __attribute__((unused)),
							   syscall_arg arg2 __attribute__((unused))) {
	priv_down();

	return 0;
}

syscall_ret syscall_exit_proc(syscall_arg arg0 __attribute__((unused)),
							  syscall_arg arg1 __attribute__((unused)),
							  syscall_arg arg2 __attribute__((unused))) {
	cleanup_process();
	scheduler();

	return 0;
}

syscall_ret syscall_sem_new(syscall_arg initial,
							syscall_arg arg1 __attribute__((unused)),
							syscall_arg arg2 __attribute__((unused))) {
	semaphore *sem = ksem_new(initial);
	return (uintptr_t)sem;
}

syscall_ret syscall_sem_lock(syscall_arg sem,
							 syscall_arg arg1 __attribute__((unused)),
							 syscall_arg arg2 __attribute__((unused))) {
	if (!is_ksem_valid((semaphore *)sem)) {
		kprintf("[kernel] invalid semaphore\n");
		return -1;
	}

	ksem_lock((semaphore *)sem, current);
	if (current->state == PROC_BLOCKED)
		scheduler();
	return 0;
}

syscall_ret syscall_sem_unlock(syscall_arg sem,
							   syscall_arg arg1 __attribute__((unused)),
							   syscall_arg arg2 __attribute__((unused))) {
	if (!is_ksem_valid((semaphore *)sem)) {
		kprintf("[kernel] invalid semaphore\n");
		return -1;
	}

	ksem_unlock((semaphore *)sem);
	return 0;
}

syscall_ret syscall_sem_free(syscall_arg sem,
							 syscall_arg arg1 __attribute__((unused)),
							 syscall_arg arg2 __attribute__((unused))) {
	if (!is_ksem_valid((semaphore *)sem)) {
		kprintf("[kernel] invalid semaphore\n");
		return -1;
	}

	ksem_free((semaphore *)sem);
	return 0;
}

syscall_ret syscall_mutex_new(syscall_arg arg0 __attribute__((unused)),
							  syscall_arg arg1 __attribute__((unused)),
							  syscall_arg arg2 __attribute__((unused))) {
	mutex *mutex = kmutex_new();
	return (uintptr_t)mutex;
}

syscall_ret syscall_mutex_lock(syscall_arg mut,
							   syscall_arg arg1 __attribute__((unused)),
							   syscall_arg arg2 __attribute__((unused))) {
	if (!is_kmutex_valid((mutex *)mut)) {
		kprintf("[kernel] invalid mutex\n");
		return -1;
	}

	kmutex_lock((mutex *)mut, current);
	if (current->state == PROC_BLOCKED)
		scheduler();
	return 0;
}

syscall_ret syscall_mutex_unlock(syscall_arg mut,
								 syscall_arg arg1 __attribute__((unused)),
								 syscall_arg arg2 __attribute__((unused))) {
	if (!is_kmutex_valid((mutex *)mut)) {
		kprintf("[kernel] invalid mutex\n");
		return -1;
	}

	kmutex_unlock((mutex *)mut, current);
	return 0;
}

syscall_ret syscall_mutex_free(syscall_arg mut,
							   syscall_arg arg1 __attribute__((unused)),
							   syscall_arg arg2 __attribute__((unused))) {
	if (!is_kmutex_valid((mutex *)mut)) {
		kprintf("[kernel] invalid mutex\n");
		return -1;
	}

	kmutex_free((mutex *)mut);
	return 0;
}

// syscall table
typedef syscall_ret (*syscall_fn)(syscall_arg, syscall_arg, syscall_arg);
static const syscall_fn syscall_table[] = {
	[SYS_WRITE] = syscall_write,
	[SYS_READ] = syscall_read,
	[SYS_TICK] = syscall_tick,
	[SYS_DELAY] = syscall_delay,
	[SYS_UNTIL] = syscall_until,
	[SYS_YIELD] = syscall_yield,
	[SYS_SEM_NEW] = syscall_sem_new,
	[SYS_SEM_LOCK] = syscall_sem_lock,
	[SYS_SEM_UNLOCK] = syscall_sem_unlock,
	[SYS_SEM_FREE] = syscall_sem_free,
	[SYS_MUTEX_NEW] = syscall_mutex_new,
	[SYS_MUTEX_LOCK] = syscall_mutex_lock,
	[SYS_MUTEX_UNLOCK] = syscall_mutex_unlock,
	[SYS_MUTEX_FREE] = syscall_mutex_free,
	[SYS_START_PROC] = syscall_start_proc,
	[SYS_EXIT_PROC] = syscall_exit_proc};
#define SYSCALL_TABLE_SIZE (sizeof(syscall_table) / sizeof(syscall_fn))

void dispatch_syscall(exception_frame *frame, uint32_t syscall) {
	if (syscall >= SYSCALL_TABLE_SIZE || !syscall_table[syscall]) {
		kprintf("[kernel] invalid system call\n");
		frame->r0 = (uint32_t)-1;
		return;
	}

	frame->r0 = syscall_table[syscall](frame->r0, frame->r1, frame->r2);
}

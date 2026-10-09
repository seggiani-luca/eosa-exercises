#ifndef SYSCALL_H
#define SYSCALL_H

// defined system call types
typedef enum {
	// character device handling
	SYS_PUTC = 1,
	SYS_GETC = 2,
	SYS_PUTS = 3,
	SYS_PUTU = 4,
	SYS_PUTI = 5,
	SYS_VPRINTF = 6,
	SYS_GETS = 7,
	SYS_TICK = 8
} syscall_types;

// defined character device aliases
typedef enum { SYS_SERIAL = 1, SYS_SEMIHOST = 2 } character_dev_types;

#endif

#ifndef SYSCALL_H
#define SYSCALL_H

// defined system call types
#define SYS_WRITE 1 // request write to a character device
#define SYS_READ 2	// request read from a character device
#define SYS_TICK 3	// request counted ms from a timer
#define SYS_DELAY 4 // block, sleeping on a timer for given ms
#define SYS_YIELD 5 // yield control to scheduler

#define SYS_START_PROC 128 // starts a process
#define SYS_EXIT_PROC 129  // terminates a process

// defined character device aliases
#define SYS_SERIAL0 1
#define SYS_SERIAL1 2
#define SYS_SERIAL2 3
#define SYS_SEMIHOST 4

// defined timer device aliases
#define SYS_SYSTICK 1
#define SYS_TIMER0 2
#define SYS_TIMER1 3

#endif

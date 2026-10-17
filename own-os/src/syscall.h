#ifndef SYSCALL_H
#define SYSCALL_H

// defined system call types
#define SYS_WRITE 1
#define SYS_READ 2
#define SYS_TICK 3

#define SYS_START_PROC 128
#define SYS_EXIT_PROC 129

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

#ifndef SYSCALL_H
#define SYSCALL_H

// defined system call types
#define SYS_WRITE 1 // request write to a character device
#define SYS_READ 2	// request read from a character device
#define SYS_TICK 3	// request counted ms from a timer
#define SYS_DELAY 4 // block, sleeping on a timer for given ms
#define SYS_UNTIL 5 // block, sleeping on a timer until given ms
#define SYS_YIELD 6 // yield control to scheduler

#define SYS_SEM_NEW 64	  // reserves a semaphore
#define SYS_SEM_LOCK 65	  // locks a semaphore
#define SYS_SEM_UNLOCK 66 // unlocks a semaphore
#define SYS_SEM_FREE 67	  // frees a semaphore

#define SYS_MUTEX_NEW 68	// reserves a mutex
#define SYS_MUTEX_LOCK 69	// locks a mutex
#define SYS_MUTEX_UNLOCK 70 // unlocks a mutex
#define SYS_MUTEX_FREE 71	// frees a mutex

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

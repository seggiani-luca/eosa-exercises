/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * System calls: the only way unprivileged code can ask the kernel to do
 * something for it. This header is shared by both sides:
 *   - the kernel (kernel/syscall.c) uses the service numbers to pick
 *     the function that does the work;
 *   - the application uses the functions declared below, written in
 *     lib/syscalls.s, which run `svc #number`.
 */
#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>

/* Service numbers: the number written after `svc`.
 * 0 is not a service: `svc #0` (or any unknown number) returns -1.
 *
 * Careful: the number after `svc` is part of the instruction itself, so
 * the assembler needs it written as a plain number. An assembly file
 * (lib/syscalls.s) cannot use these C names, so it writes 1, 2 and 3
 * directly. If you change a number here, change it there too. The
 * compiler cannot check this for you: the two files are never
 * compiled together. */
#define SYS_WRITE  1    /* send bytes to the console */
#define SYS_READ   2    /* take one received byte, if any */
#define SYS_UPTIME 3    /* milliseconds since start */

/* Offer len bytes of buf for sending. Returns how many bytes the kernel
 * accepted: maybe fewer than len, maybe 0 when it has no room right
 * now. It never waits. The caller sends the rest later. */
int sys_write( const char *buf, unsigned len );

/* Return one received byte (0..255), or -1 if no byte is waiting.
 * It never waits. */
int sys_read( void );

/* Return the milliseconds since the kernel started its clock. */
uint32_t sys_uptime( void );

#endif /* SYSCALLS_H */

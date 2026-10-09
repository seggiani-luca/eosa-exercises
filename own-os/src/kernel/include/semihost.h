#ifndef SEMIHOST_H
#define SEMIHOST_H

#include "device.h"

// semihosting operation types
typedef enum {
	SYS_OPEN = 0x01,   // opens a file/stream on the host
	SYS_ISTTY = 0x09,  // checks if file handle on the host is a tty
	SYS_WRITE = 0x05,  // writes a file/stream on the host
	SYS_READ = 0x06,   // reads a file/stream on the host
	SYS_CLOSE = 0x02,  // closes a file/stream on the host
	SYS_FLEN = 0x0C,   // gets length of a file on the host
	SYS_SEEK = 0x0A,   // seeks on a file/stream on the host
	SYS_TMPNAM = 0x0D, // gets a temporary path on the host
	SYS_REMOVE = 0x0E, // removes a file on the host
	SYS_RENAME = 0x0F, // renames a file on the host

	SYS_WRITEC = 0x03, // writes a character on the host tty
	SYS_WRITE0 = 0x04, // writes a NULL terminated string on the host tty
	SYS_READC = 0x07,  // reads a character from the host tty

	SYS_CLOCK = 0x10,	 // gets processor time since program start
	SYS_ELAPSED = 0x30,	 // gets high-resolution timer
	SYS_TICKFREQ = 0x31, // tick frequency for high-resolution timer
	SYS_TIME = 0x11,	 // gets seconds since UNIX epoch

	SYS_ERRNO = 0x13,		// gets errno from the host's libc
	SYS_GET_CMDLINE = 0x15, // gets argc and argv from the hosts' libc
	SYS_HEAPINFO = 0x16,	// gets heap information from the host
	SYS_ISERROR = 0x08,		// tests whether a semihosting return means error
	SYS_SYSTEM = 0x12		// executes a command on the host's shell
} sh_ops;

// makes a semihosting call with the given operation and operand
int shst_call(int op, void *arg);

// puts a character on the host tty via semihosting
void shst_putc(char c);

// gets a character from the host tty via semihosting
int shst_getc();

// semihosting device driver
extern char_driver shst_driver;

#endif

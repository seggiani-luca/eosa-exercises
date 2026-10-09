#ifndef LIB_H
#define LIB_H

#include "../../syscall.h"

// makes syscall
#define SYSCALL(n)                                                             \
	register int r0 __asm__("r0");                                             \
	__asm__ volatile("svc %1" : "=r"(r0) : "I"(n) : "memory");                 \
	r0;

// puts a char
void fputc(int handle, char c);

// gets a char
int fgetc(int handle);

// puts a string
void fputs(int handle, const char *s);

// puts an unsigned integer
void fputu(int handle, unsigned int n);

// puts a signed integer
void fputi(int handle, int n);

// minimal format print with the types above
void fprintf(int handle, const char *fmt, ...);

// gets a newlined string
int fgets(int handle, char *buf, int siz);

// libc thin wrappers
#define putc(c) fputc(SYS_SERIAL, (c))
#define getc() fgetc(SYS_SERIAL)
#define puts(s) fputs(SYS_SERIAL, (s))
#define putu(n) fputu(SYS_SERIAL, (n))
#define puti(n) fputi(SYS_SERIAL, (n))
#define printf(fmt, ...) fprintf(SYS_SERIAL, (fmt), ##__VA_ARGS__)
#define gets(buf, siz) fgets(SYS_SERIAL, (buf), (siz))

// ASCII to unsigned integer
extern int atou(const char *s);

// ASCII to signed integer
extern int atoi(const char *s);

// gets current system tick
int tick();

#endif

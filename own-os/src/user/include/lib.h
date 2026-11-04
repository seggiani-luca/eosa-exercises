#ifndef LIB_H
#define LIB_H

#include "../../syscall.h"
#include <stdarg.h>
#include <stdint.h>

/* synchronization */

// definition for spinlock, just an integer
typedef uint32_t spinlock;

// locks a spinlock
void spin_lock(spinlock *lock);

// unlocks a spinlock
void spin_unlock(spinlock *lock);

// definition for opaque semaphore
typedef uint32_t semaphore_t;

// definition for opaque mutex
typedef uint32_t mutex_t;

/* syscalls */

// request write to a character device
extern unsigned int fwrite(int dev, const char *buf, unsigned int len);

// request read from a character device
extern unsigned int fread(int dev, char *buf, unsigned int len);

// request counted ms from a timer
extern int tick(int dev);

// block, sleeping on a timer for given ms
extern void delay(int dev, unsigned int ms);

// block, sleeping on a timer until given ms
extern void until(int dev, unsigned int ms);

// yield control to scheduler
extern void yield();

// terminate this process
extern void terminate();

// reserves a semaphore
extern semaphore_t sem_new(int initial);

// locks a semaphore
extern void sem_lock(semaphore_t sem);

// unlocks a semaphore
extern void sem_unlock(semaphore_t sem);

// frees a semaphore
extern void sem_free(semaphore_t sem);

// reserves a mutex
extern mutex_t mutex_new();

// locks a mutex
extern void mutex_lock(mutex_t mut);

// unlocks a mutex
extern void mutex_unlock(mutex_t mut);

// frees a mutex
extern void mutex_free(mutex_t mut);

// not a syscall, process priority specifier
#define PROCESS_PRIORITY(p)

/* format */

// virtual putc and getc functions
typedef void (*vputc)(void *ctx, char c);
typedef int (*vgetc)(void *ctx);

// virtual flush function
typedef void (*vflush)(void *ctx);

// formats a string
void sformat(vputc out, void *ctx, const char *s);

// formats an unsigned integer
void uformat(vputc out, void *ctx, unsigned int n);

// formats a signed integer
void iformat(vputc out, void *ctx, int n);

// formats an hexadecimal word
void xformat(vputc out, void *ctx, uint32_t w);

// formats expression
void vformat(vputc out, void *ctx, const char *fmt, va_list ap);

// gets formatted line input
int lformat(vgetc in, vputc out, vflush flush, void *ctx, char *buf, int siz);

/* stdio */

// size of standard I/O buffer
#define STDIO_BUF_SIZE 64

// puts a char
void fputc(int dev, char c);

// gets a char
int fgetc(int dev);

// puts a string
void fputs(int dev, const char *s);

// puts an unsigned integer
void fputu(int dev, unsigned int n);

// puts a signed integer
void fputi(int dev, int n);

// puts an hexadecimal word
void fputx(int dev, uint32_t w);

// minimal format print with the types above
void fprintf(int dev, const char *fmt, ...);

// gets a newlined string
int fgets(int dev, char *buf, int siz);

// libc thin wrappers
#define putc(c) fputc(SYS_SERIAL0, (c))
#define getc() fgetc(SYS_SERIAL0)
#define puts(s) fputs(SYS_SERIAL0, (s))
#define putu(n) fputu(SYS_SERIAL0, (n))
#define puti(n) fputi(SYS_SERIAL0, (n))
#define putx(w) fputx(SYS_SERIAL0, (w))
#define printf(fmt, ...) fprintf(SYS_SERIAL0, (fmt), ##__VA_ARGS__)
#define gets(buf, siz) fgets(SYS_SERIAL0, (buf), (siz))

/* util */

// ASCII to unsigned integer
int atou(const char *s);

// ASCII to signed integer
int atoi(const char *s);

/* time */

// delay on systick
#define sleep(ms) delay(SYS_SYSTICK, (ms))

#endif

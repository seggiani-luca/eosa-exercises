#ifndef LIB_H
#define LIB_H

#include "../../syscall.h"
#include <stdarg.h>
#include <stdint.h>

/* syscalls */

// writes a buffer to a device
unsigned int fwrite(int dev, const char *buf, unsigned int len);

// reads a buffer from a device
unsigned int fread(int dev, char *buf, unsigned int len);

// gets current system tick
int tick();

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
#define putc(c) fputc(SYS_SERIAL, (c))
#define getc() fgetc(SYS_SERIAL)
#define puts(s) fputs(SYS_SERIAL, (s))
#define putu(n) fputu(SYS_SERIAL, (n))
#define puti(n) fputi(SYS_SERIAL, (n))
#define putx(w) fputx(SYS_SERIAL, (w))
#define printf(fmt, ...) fprintf(SYS_SERIAL, (fmt), ##__VA_ARGS__)
#define gets(buf, siz) fgets(SYS_SERIAL, (buf), (siz))

/* util */

// ASCII to unsigned integer
int atou(const char *s);

// ASCII to signed integer
int atoi(const char *s);

#endif

#ifndef STDIO_H
#define STDIO_H

#include "device.h"
#include <stdarg.h>

// puts a string
void io_puts(char_driver *dev, const char *s);

// puts an unsigned integer
void io_putu(char_driver *dev, unsigned int n);

// puts a signed integer
void io_puti(char_driver *dev, int n);

// minimal format print with the types above
void io_vprintf(char_driver *dev, const char *fmt, va_list ap);

// gets a newlined string (echoes as convenience)
int io_gets(char_driver *dev, char *buf, int siz);

// ASCII to unsigned integer
int atou(const char *s);

// ASCII to signed integer
int atoi(const char *s);

#endif

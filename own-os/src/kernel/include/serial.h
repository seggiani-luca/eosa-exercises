#ifndef SERIAL_H
#define SERIAL_H

#include "cmsdk.h"
#include "device.h"

// initializes the serial interface
void uart_init();

// puts a character on the serial interface
void uart_putc(char c);

// gets a character from the serial interface
int uart_getc();

// serial device driver
extern char_driver uart_driver;

// kernel debug printf
void kprintf(const char *fmt, ...);

#endif

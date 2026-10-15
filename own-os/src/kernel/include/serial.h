#ifndef SERIAL_H
#define SERIAL_H

#include "cmsdk.h"
#include "device.h"

// initializes the serial interface
void uart_init();

// puts a character on the serial interface (polling)
void uart_putc(char c);

// gets a character from the serial interface (polling)
int uart_getc();

// copy as many bytes as fit into the send buffer, start if idle
unsigned int uart_write(const char *buf, unsigned int len);

// copy as many bytes as fit from the receive buffer, start if idle
unsigned int uart_read(char *buf, unsigned int len);

// serial device driver
extern char_driver uart_driver;

// kernel debug printf
void kprintf(const char *fmt, ...);

/* buffers */

// size of serial buffers
#define SERIAL_BUF_SIZE 64

#endif

#ifndef SERIAL_H
#define SERIAL_H

#include "cmsdk.h"
#include "driver.h"

// initializes the serial interface
void uart_init(void *ctx);

// puts a character on the serial interface (polling)
void uart_putc(void *ctx, char c);

// gets a character from the serial interface (polling)
int uart_getc(void *ctx);

// copy as many bytes as fit into the send buffer, start if idle
unsigned int uart_write(void *ctx, const char *buf, unsigned int len);

// copy as many bytes as fit from the receive buffer, start if idle
unsigned int uart_read(void *ctx, char *buf, unsigned int len);

// serial device drivers
extern char_driver uart0_driver;
extern char_driver uart1_driver;
extern char_driver uart2_driver;

/* buffers */

// size of serial buffers
#define SERIAL_BUF_SIZE 64

#endif

/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * UART0 driver. Sending is done in two ways:
 *   - polled: wait in a loop until the UART can take the next byte;
 *   - buffered: put bytes in a ring buffer and let the UART interrupt
 *     send them one by one while the CPU does other work.
 * Receiving is polled, or driven by the receive interrupt into a ring buffer.
 */
#ifndef UART_H
#define UART_H

#include "cmsdk.h"

/* Set up UART0 and its interrupt. Call it once, before any other use. */
void uart_init( void );

/* Send one byte now. It waits until the UART has room, so no byte is lost. */
void uart_putc_polled( char c );

/* Return a received byte, or -1 if no byte is waiting. It never waits. */
int uart_getc_polled( void );

/* Copy up to len bytes into the send buffer and return how many were
 * accepted (0 if the buffer is full). The caller must retry the rest. */
unsigned uart_write( const char *buf, unsigned len );

/* Return the oldest received byte, or -1 if none is waiting. It never
 * waits. The byte comes from the receive ring buffer. */
int uart_getc( void );

/* The same two operations on any UART, so UART0 and UART1 share code. */
void uart_init_on( CMSDK_UART_TypeDef *u );
void uart_putc_polled_on( CMSDK_UART_TypeDef *u, char c );

/* The UART0 receive interrupt handler: it stores the byte in the ring
 * buffer. It is named in the vector table. */
void UART0RX_Handler( void );

/* The UART0 send interrupt handler. It is named in the vector table. */
void UART0TX_Handler( void );

#endif /* UART_H */

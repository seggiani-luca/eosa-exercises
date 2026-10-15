/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * UART0 driver: see uart.h for the list of functions.
 */

#include "drivers/uart.h"
#include "cmsdk.h"
#include "kernel/irq.h"

/* Size of the send ring buffer. A power of two, so "index & (SIZE - 1)"
 * wraps an index back to 0. */
#define TX_BUF_SIZE 64

/* The send ring buffer: bytes waiting to be sent. head is written by
 * uart_write; tail by the interrupt handler (and by uart_write when it
 * primes the UART). volatile: the handler changes them behind the main code's back. */
static char tx_buf[TX_BUF_SIZE];
static volatile unsigned tx_head;
static volatile unsigned tx_tail;
/* true while the UART is busy sending and its interrupt will come. */
static volatile int tx_busy;

/* The buffer is empty when head and tail are equal. */
static int tx_empty( void )
{
    return tx_head == tx_tail;
}

/* The buffer is full when the next head would hit the tail: one slot is
 * always unused, so "full" and "empty" never look the same. */
static int tx_full( void )
{
    return ( ( tx_head + 1 ) & ( TX_BUF_SIZE - 1 ) ) == tx_tail;
}

/* Take the oldest byte out of the buffer. The buffer must not be empty. */
static char tx_pop( void )
{
    char c = tx_buf[tx_tail];                   /* read the oldest byte */
    tx_tail = ( tx_tail + 1 ) & ( TX_BUF_SIZE - 1 ); /* move on, wrap at the end */
    return c;
}

/* Set up UART0, its send interrupt, and an empty buffer. */
void uart_init( void )
{
    /* Baud-rate divider: QEMU ignores the speed anyway. */
    CMSDK_UART0->BAUDDIV = 16;
    /* Switch on the transmitter and the receiver. */
    CMSDK_UART0->CTRL = UART_CTRL_TXEN | UART_CTRL_RXEN;

    /* Start with an empty buffer and an idle transmitter. */
    tx_head = 0;
    tx_tail = 0;
    tx_busy = 0;

    /* Priority 0xC0: below the SVCall (0x80), above SysTick (0xE0). */
    NVIC->IPR[UART0TX_IRQn] = 0xC0;
    /* Let the NVIC deliver the interrupt (ISER: write 1 to enable). */
    NVIC->ISER[UART0TX_IRQn / 32] = 1UL << ( UART0TX_IRQn % 32 );
}

/* Send one byte, waiting while the UART is full. */
void uart_putc_polled( char c )
{
    /* Wait while the UART send buffer is full. */
    while ( CMSDK_UART0->STATE & UART_STATE_TXBF )
    {
    }
    /* Now there is room: send the byte. */
    CMSDK_UART0->DATA = (uint32_t) (unsigned char) c;
}

/* Return one received byte, or -1 if none. Never waits. */
int uart_getc_polled( void )
{
    /* No byte waiting: report it with -1. */
    if ( ( CMSDK_UART0->STATE & UART_STATE_RXBF ) == 0 )
    {
        return -1;
    }
    /* A byte is waiting: reading DATA returns it. Keep the low 8 bits. */
    return (int) ( CMSDK_UART0->DATA & 0xFF );
}

/* Copy as many bytes as fit into the send buffer and start the UART if idle. */
unsigned uart_write( const char *buf, unsigned len )
{
    unsigned accepted = 0;

    /* Mask interrupts: the handler also uses the indices and the flag. */
    uint32_t saved = irq_save();

    /* Copy bytes while there are some left and the buffer has room. */
    while ( accepted < len && !tx_full() )
    {
        tx_buf[tx_head] = buf[accepted];                   /* store the byte */
        tx_head = ( tx_head + 1 ) & ( TX_BUF_SIZE - 1 );   /* move on, wrap at the end */
        accepted++;                                        /* count it */
    }

    /* If the UART is idle and we stored something, start it: no
     * interrupt is coming to do it for us. */
    if ( !tx_busy && !tx_empty() )
    {
        /* Enable the send interrupt first: it is raised only if enabled during the write. */
        CMSDK_UART0->CTRL |= UART_CTRL_TXIRQEN;
        /* The UART may still be sending a polled byte: wait for room. */
        while ( CMSDK_UART0->STATE & UART_STATE_TXBF )
        {
        }
        /* Send the first byte ("priming"); its interrupt makes the handler send the next. */
        CMSDK_UART0->DATA = (uint32_t) (unsigned char) tx_pop();
        tx_busy = 1;
    }

    /* Put the interrupt mask back as it was. */
    irq_restore( saved );
    return accepted;
}

/* UART0 send interrupt: send the next buffered byte, or go idle. */
void UART0TX_Handler( void )
{
    /* Acknowledge at the device (write 1 to clear): the NVIC line stays
     * active until the UART's own flag is cleared. */
    CMSDK_UART0->INTSTATUS = UART_INT_TX;

    if ( !tx_empty() )
    {
        /* More to send: the previous byte is gone, send the next. */
        CMSDK_UART0->DATA = (uint32_t) (unsigned char) tx_pop();
    }
    else
    {
        /* Nothing left: stop the interrupt and mark the UART idle. */
        CMSDK_UART0->CTRL &= ~UART_CTRL_TXIRQEN;
        tx_busy = 0;
    }
}

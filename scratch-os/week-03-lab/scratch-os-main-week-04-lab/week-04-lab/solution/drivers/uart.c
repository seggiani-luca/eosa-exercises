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

/* Size of the send ring buffer. It must be a power of two, so that
 * "index & (SIZE - 1)" wraps an index back to 0 (same as "index % SIZE"). */
#define TX_BUF_SIZE 64

/* The ring buffer: bytes waiting to be sent. */
static char tx_buf[TX_BUF_SIZE];
/* head: where the next byte will be stored (written only by uart_write).
 * tail: where the next byte will be taken (written only by the handler,
 * and by uart_write when it primes the UART). volatile: the interrupt
 * handler changes them behind the back of the main code. */
static volatile unsigned tx_head;
static volatile unsigned tx_tail;
/* Size of the receive ring buffer, also a power of two. */
#define RX_BUF_SIZE 32
static char rx_buf[RX_BUF_SIZE];
/* Here the handler writes rx_head and the reader writes rx_tail: the
 * opposite roles from the send buffer. */
static volatile unsigned rx_head;
static volatile unsigned rx_tail;

/* true while the UART is busy sending and its interrupt will come. */
static volatile int tx_busy;

/* The buffer is empty when head and tail are equal. */
static int tx_empty( void )
{
    return tx_head == tx_tail;
}

/* The buffer is full when the next head would hit the tail. One slot is
 * always left unused, so "full" and "empty" never look the same. */
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

void uart_init( void )
{
    /* Baud-rate divider: the clock (25 MHz) divided by 16 is far
     * above any real baud rate; QEMU ignores the speed anyway. */
    CMSDK_UART0->BAUDDIV = 16;
    /* Switch on the transmitter and the receiver. The send interrupt
     * stays off until there is something to send. */
    CMSDK_UART0->CTRL = UART_CTRL_TXEN | UART_CTRL_RXEN | UART_CTRL_RXIRQEN;

    /* Start with an empty buffer and an idle transmitter. */
    tx_head = 0;
    tx_tail = 0;
    tx_busy = 0;
    rx_head = 0;
    rx_tail = 0;

    /* Priority 0xC0: less urgent than the system call exception (0x80),
     * more urgent than SysTick (0xE0). Lower number = more urgent. */
    NVIC->IPR[UART0TX_IRQn] = 0xC0;
    /* Let the NVIC deliver this interrupt. ISER is write-1-to-enable:
     * writing 0 bits changes nothing, so a plain "=" is safe. */
    NVIC->ISER[UART0TX_IRQn / 32] = 1UL << ( UART0TX_IRQn % 32 );

    /* The receive interrupt: same priority, same two enables (the
     * device's RXIRQEN above, and the NVIC bit here). */
    NVIC->IPR[UART0RX_IRQn] = 0xC0;
    NVIC->ISER[UART0RX_IRQn / 32] = 1UL << ( UART0RX_IRQn % 32 );
}

void uart_init_on( CMSDK_UART_TypeDef *u )
{
    u->BAUDDIV = 16;
    u->CTRL = UART_CTRL_TXEN | UART_CTRL_RXEN;
}

void uart_putc_polled_on( CMSDK_UART_TypeDef *u, char c )
{
    /* Wait while the UART send buffer is full. */
    while ( u->STATE & UART_STATE_TXBF )
    {
    }
    /* Now there is room: send the byte. */
    u->DATA = (uint32_t) (unsigned char) c;
}

/* The old function, kept so that kprintf needs no change. */
void uart_putc_polled( char c )
{
    uart_putc_polled_on( CMSDK_UART0, c );
}

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

unsigned uart_write( const char *buf, unsigned len )
{
    unsigned accepted = 0;

    /* Block interrupts: the handler also uses the indices and the flag,
     * and must not run in the middle of our updates. */
    uint32_t saved = irq_save();

    /* Copy bytes while there are some left and the buffer has room. */
    while ( accepted < len && !tx_full() )
    {
        tx_buf[tx_head] = buf[accepted];                   /* store the byte */
        tx_head = ( tx_head + 1 ) & ( TX_BUF_SIZE - 1 );   /* move on, wrap at the end */
        accepted++;                                        /* count it */
    }

    /* If the UART is idle and we stored something, start it: nobody
     * else will, because no interrupt is coming. */
    if ( !tx_busy && !tx_empty() )
    {
        /* First enable the send interrupt, then write the byte. The
         * interrupt is raised only if it is enabled during the write. */
        CMSDK_UART0->CTRL |= UART_CTRL_TXIRQEN;
        /* The UART may still be sending a polled byte: wait for room. */
        while ( CMSDK_UART0->STATE & UART_STATE_TXBF )
        {
        }
        /* Send the first byte ("priming"). Its completion interrupt
         * will make the handler send the next one. */
        CMSDK_UART0->DATA = (uint32_t) (unsigned char) tx_pop();
        tx_busy = 1;
    }

    /* Put the interrupt mask back as it was. */
    irq_restore( saved );
    return accepted;
}

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

void UART0RX_Handler( void )
{
    /* Reading DATA takes the byte out of the UART. */
    char c = (char) ( CMSDK_UART0->DATA & 0xFF );

    /* Store it unless the buffer is full; a full buffer drops the byte. */
    unsigned next = ( rx_head + 1 ) & ( RX_BUF_SIZE - 1 );
    if ( next != rx_tail )
    {
        rx_buf[rx_head] = c;
        rx_head = next;
    }

    /* Acknowledge at the device (write 1 to clear). */
    CMSDK_UART0->INTSTATUS = UART_INT_RX;
}

int uart_getc( void )
{
    int c = -1;

    /* Block interrupts: the handler writes rx_head. */
    uint32_t saved = irq_save();
    if ( rx_head != rx_tail )
    {
        c = (unsigned char) rx_buf[rx_tail];
        rx_tail = ( rx_tail + 1 ) & ( RX_BUF_SIZE - 1 );
    }
    irq_restore( saved );
    return c;
}

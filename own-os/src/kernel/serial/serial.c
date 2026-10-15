#include "../include/serial.h"
#include "../include/irq.h"
#include "../include/wfi.h"

char_driver uart_driver = {
	.init = uart_init,
	.write = uart_write,
	.read = uart_read,
};

// serial buffer
typedef struct {
	// data in buffer
	char dat[SERIAL_BUF_SIZE];

	// head of buffer
	unsigned int head;

	// tail of buffer
	unsigned int tail;
} sbuf;

int sbuf_empty(sbuf *b) { return b->head == b->tail; }

int sbuf_full(sbuf *b) { return ((b->head + 1) % SERIAL_BUF_SIZE) == b->tail; }

char sbuf_pop(sbuf *b) {
	char c = b->dat[b->tail];
	b->tail = (b->tail + 1) % SERIAL_BUF_SIZE;
	return c;
}

void sbuf_push(sbuf *b, char c) {
	b->dat[b->head] = c;
	b->head = (b->head + 1) % SERIAL_BUF_SIZE;
}

// serial transmit buffer
static sbuf tx_buf;

// serial receive buffer
static sbuf rx_buf;

// serial transimtter busy flag
static int tx_busy = 0;

// serial receiver busy flag
static int rx_busy = 0;

void uart_init() {
	// set baud divider
	CMSDK_UART0->BAUDDIV = 16;

	// start transmitter and receiver
	CMSDK_UART0->CTRL = UART_CTRL_TXEN | UART_CTRL_RXEN;

	// clean buffers
	tx_buf.head = tx_buf.tail = 0;
	rx_buf.head = rx_buf.tail = 0;

	// set NVIC priority
	NVIC->IPR[UART0TX_IRQn] = 0xC0;
	NVIC->IPR[UART0RX_IRQn] = 0xC0;

	// enable NVIC interrupts
	NVIC->ISER[UART0TX_IRQn / 32] = 1UL << (UART0TX_IRQn % 32);
	NVIC->ISER[UART0RX_IRQn / 32] = 1UL << (UART0RX_IRQn % 32);
}

void uart_putc(char c) {
	// poll
	while (CMSDK_UART0->STATE & UART_STATE_TXBF)
		;

	// send byte
	CMSDK_UART0->DATA = (uint32_t)(unsigned char)c;
}

int uart_getc() {
	// poll
	while ((CMSDK_UART0->STATE & UART_STATE_RXBF) == 0)
		;

	// receive byte
	return (int)(CMSDK_UART0->DATA & 0xFF);
}

unsigned int uart_write(const char *buf, unsigned int len) {
	if (len == 0)
		return 0;
	unsigned int accepted = 0;

	uint32_t saved = irq_save();

	// load transmit buffer
	while (accepted < len && !sbuf_full(&tx_buf)) {
		sbuf_push(&tx_buf, buf[accepted]);
		accepted++;
	}

	// start if idle
	if (!tx_busy && !sbuf_empty(&tx_buf)) {
		CMSDK_UART0->CTRL |= UART_CTRL_TXIRQEN;

		// wait until ready
		while (CMSDK_UART0->STATE & UART_STATE_TXBF)
			;

		// start transmission
		CMSDK_UART0->DATA = (uint32_t)(unsigned char)sbuf_pop(&tx_buf);
		tx_busy = 1;
	}

	irq_restore(saved);
	return accepted;
}

unsigned int uart_read(char *buf, unsigned int len) {
	if (len == 0)
		return 0;
	unsigned int accepted = 0;

	uint32_t saved = irq_save();

	// start if idle
	if (!rx_busy && sbuf_empty(&rx_buf)) {
		// start reception
		CMSDK_UART0->CTRL |= UART_CTRL_RXIRQEN;
		rx_busy = 1;

		// wait for first byte
		irq_restore(saved);

		while (sbuf_empty(&rx_buf))
			wfi();

		saved = irq_save();
	}

	// load from receiver buffer
	while (accepted < len && !sbuf_empty(&rx_buf)) {
		buf[accepted] = sbuf_pop(&rx_buf);
		accepted++;
	}

	irq_restore(saved);
	return accepted;
}

// serial transmission handler
void UART0TX_Handler(void) {
	CMSDK_UART0->INTSTATUS = UART_INTSTATUS_TXIRQ;

	if (!sbuf_empty(&tx_buf)) {
		// send more
		CMSDK_UART0->DATA = (uint32_t)(unsigned char)sbuf_pop(&tx_buf);
	} else {
		// stop the serial interface
		CMSDK_UART0->CTRL &= ~UART_CTRL_TXIRQEN;
		tx_busy = 0;
	}
}

// serial reception handler
void UART0RX_Handler(void) {
	if (!sbuf_full(&rx_buf)) {
		// get more
		sbuf_push(&rx_buf, (int)(CMSDK_UART0->DATA & 0xFF));
	} else {
		// stop the serial interface
		CMSDK_UART0->CTRL &= ~UART_CTRL_RXIRQEN;
		rx_busy = 0;
	}

	CMSDK_UART0->INTSTATUS = UART_INTSTATUS_RXIRQ;
}

#include "../../include/serial.h"
#include "../../include/irq.h"
#include "../../include/wfi.h"

// serial buffer
typedef struct {
	// data in buffer
	char dat[SERIAL_BUF_SIZE];

	// head of buffer
	unsigned int head;

	// tail of buffer
	unsigned int tail;
} sbuf;

// serial handling block
typedef struct {
	// uart
	CMSDK_UART_TypeDef *uart;

	// transmit IRQ number
	int tx_irq;

	// receive IRQ number
	int rx_irq;

	// transmit buffer
	sbuf tx_buf;

	// receive buffer
	sbuf rx_buf;

	// transimtter busy flag
	int tx_busy;

	// receiver busy flag
	int rx_busy;
} serial_block;

// serial blocks
static serial_block uart0 = {
	.uart = CMSDK_UART0, .tx_irq = UART0TX_IRQn, .rx_irq = UART0RX_IRQn};
static serial_block uart1 = {
	.uart = CMSDK_UART1, .tx_irq = UART1TX_IRQn, .rx_irq = UART1RX_IRQn};
static serial_block uart2 = {
	.uart = CMSDK_UART2, .tx_irq = UART2TX_IRQn, .rx_irq = UART2RX_IRQn};

// drivers
char_driver uart0_driver = {.init = uart_init,
							.write = uart_write,
							.read = uart_read,
							.putc = uart_putc,
							.getc = uart_getc,
							.ctx = &uart0};
char_driver uart1_driver = {.init = uart_init,
							.write = uart_write,
							.putc = uart_putc,
							.getc = uart_getc,
							.read = uart_read,
							.ctx = &uart1};
char_driver uart2_driver = {.init = uart_init,
							.write = uart_write,
							.putc = uart_putc,
							.getc = uart_getc,
							.read = uart_read,
							.ctx = &uart2};

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

void uart_init(void *ctx) {
	serial_block *bl = ctx;

	// set baud divider
	bl->uart->BAUDDIV = 16;

	// start transmitter and receiver
	bl->uart->CTRL = UART_CTRL_TXEN | UART_CTRL_RXEN;

	// clean buffers
	bl->tx_buf.head = bl->tx_buf.tail = 0;
	bl->rx_buf.head = bl->rx_buf.tail = 0;

	// set NVIC priority
	NVIC->IPR[bl->tx_irq] = 0xC0;
	NVIC->IPR[bl->rx_irq] = 0xC0;

	// enable NVIC interrupts
	NVIC->ISER[bl->tx_irq / 32] = 1UL << (bl->tx_irq % 32);
	NVIC->ISER[bl->rx_irq / 32] = 1UL << (bl->rx_irq % 32);
}

void uart_putc(void *ctx, char c) {
	serial_block *bl = ctx;

	// poll
	while (bl->uart->STATE & UART_STATE_TXBF)
		;

	// send byte
	bl->uart->DATA = (uint32_t)(unsigned char)c;
}

int uart_getc(void *ctx) {
	serial_block *bl = ctx;

	// poll
	while ((bl->uart->STATE & UART_STATE_RXBF) == 0)
		;

	// receive byte
	return (int)(bl->uart->DATA & 0xFF);
}

unsigned int uart_write(void *ctx, const char *buf, unsigned int len) {
	serial_block *bl = ctx;

	if (len == 0)
		return 0;
	unsigned int accepted = 0;

	uint32_t saved = irq_save();

	// load transmit buffer
	while (accepted < len && !sbuf_full(&bl->tx_buf)) {
		sbuf_push(&bl->tx_buf, buf[accepted]);
		accepted++;
	}

	// start if idle
	if (!bl->tx_busy && !sbuf_empty(&bl->tx_buf)) {
		bl->uart->CTRL |= UART_CTRL_TXIRQEN;

		// wait until ready
		while (bl->uart->STATE & UART_STATE_TXBF)
			;

		// start transmission
		bl->uart->DATA = (uint32_t)(unsigned char)sbuf_pop(&bl->tx_buf);
		bl->tx_busy = 1;
	}

	irq_restore(saved);
	return accepted;
}

unsigned int uart_read(void *ctx, char *buf, unsigned int len) {
	serial_block *bl = ctx;

	if (len == 0)
		return 0;
	unsigned int accepted = 0;

	uint32_t saved = irq_save();

	// start if idle
	if (!bl->rx_busy && sbuf_empty(&bl->rx_buf)) {
		// start reception
		bl->uart->CTRL |= UART_CTRL_RXIRQEN;
		bl->rx_busy = 1;

		// wait for first byte
		irq_restore(saved);

		while (sbuf_empty(&bl->rx_buf))
			wfi();

		saved = irq_save();
	}

	// load from receiver buffer
	while (accepted < len && !sbuf_empty(&bl->rx_buf)) {
		buf[accepted] = sbuf_pop(&bl->rx_buf);
		accepted++;
	}

	irq_restore(saved);
	return accepted;
}

// serial transmission handler 0
void UART0TX_Handler(void) {
	CMSDK_UART0->INTSTATUS = UART_INTSTATUS_TXIRQ;

	if (!sbuf_empty(&uart0.tx_buf)) {
		// send more
		CMSDK_UART0->DATA = (uint32_t)(unsigned char)sbuf_pop(&uart0.tx_buf);
	} else {
		// stop the serial interface
		CMSDK_UART0->CTRL &= ~UART_CTRL_TXIRQEN;
		uart0.tx_busy = 0;
	}
}

// serial reception handler 0
void UART0RX_Handler(void) {
	if (!sbuf_full(&uart0.rx_buf)) {
		// get more
		sbuf_push(&uart0.rx_buf, (int)(CMSDK_UART0->DATA & 0xFF));
	} else {
		// stop the serial interface
		CMSDK_UART0->CTRL &= ~UART_CTRL_RXIRQEN;
		uart0.rx_busy = 0;
	}

	CMSDK_UART0->INTSTATUS = UART_INTSTATUS_RXIRQ;
}

// serial transmission handler 1
void UART1TX_Handler(void) {
	CMSDK_UART1->INTSTATUS = UART_INTSTATUS_TXIRQ;

	if (!sbuf_empty(&uart1.tx_buf)) {
		// send more
		CMSDK_UART1->DATA = (uint32_t)(unsigned char)sbuf_pop(&uart1.tx_buf);
	} else {
		// stop the serial interface
		CMSDK_UART1->CTRL &= ~UART_CTRL_TXIRQEN;
		uart1.tx_busy = 0;
	}
}

// serial reception handler 1
void UART1RX_Handler(void) {
	if (!sbuf_full(&uart1.rx_buf)) {
		// get more
		sbuf_push(&uart1.rx_buf, (int)(CMSDK_UART1->DATA & 0xFF));
	} else {
		// stop the serial interface
		CMSDK_UART1->CTRL &= ~UART_CTRL_RXIRQEN;
		uart1.rx_busy = 0;
	}

	CMSDK_UART1->INTSTATUS = UART_INTSTATUS_RXIRQ;
}

// serial transmission handler 2
void UART2TX_Handler(void) {
	CMSDK_UART2->INTSTATUS = UART_INTSTATUS_TXIRQ;

	if (!sbuf_empty(&uart2.tx_buf)) {
		// send more
		CMSDK_UART2->DATA = (uint32_t)(unsigned char)sbuf_pop(&uart2.tx_buf);
	} else {
		// stop the serial interface
		CMSDK_UART2->CTRL &= ~UART_CTRL_TXIRQEN;
		uart2.tx_busy = 0;
	}
}

// serial reception handler 2
void UART2RX_Handler(void) {
	if (!sbuf_full(&uart2.rx_buf)) {
		// get more
		sbuf_push(&uart2.rx_buf, (int)(CMSDK_UART2->DATA & 0xFF));
	} else {
		// stop the serial interface
		CMSDK_UART2->CTRL &= ~UART_CTRL_RXIRQEN;
		uart2.rx_busy = 0;
	}

	CMSDK_UART2->INTSTATUS = UART_INTSTATUS_RXIRQ;
}

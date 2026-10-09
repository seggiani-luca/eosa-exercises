#include "../include/serial.h"
#include "../include/stdio.h"

char_driver uart_driver = {
	.init = uart_init,
	.putc = uart_putc,
	.getc = uart_getc,
};

void uart_init() {
	// set baud divider
	CMSDK_UART0->BAUDDIV = 16;

	// enable UART
	CMSDK_UART0->CTRL = UART_CTRL_TXEN | UART_CTRL_RXEN;
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

void kprintf(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	io_vprintf(&uart_driver, fmt, ap);

	va_end(ap);
}

#include "../../include/lib.h"
#include <stdarg.h>

/* output */

// user stdio buffer
typedef struct {
	// data in buffer
	char dat[STDIO_BUF_SIZE];

	// head of buffer
	unsigned int head;

	// tail of buffer
	unsigned int tail;
} ustdio_buf;

int ubuf_empty(ustdio_buf *b) { return b->head == b->tail; }

int ubuf_full(ustdio_buf *b) {
	return ((b->head + 1) % STDIO_BUF_SIZE) == b->tail;
}

unsigned int ubuf_len(ustdio_buf *b) {
	return ((b->head - b->tail) + STDIO_BUF_SIZE) % STDIO_BUF_SIZE;
}

char ubuf_pop(ustdio_buf *b) {
	char c = b->dat[b->tail];
	b->tail = (b->tail + 1) % STDIO_BUF_SIZE;
	return c;
}

void ubuf_push(ustdio_buf *b, char c) {
	b->dat[b->head] = c;
	b->head = (b->head + 1) % STDIO_BUF_SIZE;
}

// user stdio context
typedef struct {
	// targeted device
	int dev;

	// output buffer
	ustdio_buf out;

	// input buffer
	ustdio_buf in;
} ustdio_ctx;

void uctx_new(ustdio_ctx *ctx, int dev) {
	ctx->dev = dev;
	ctx->in.head = ctx->in.tail = 0;
	ctx->out.head = ctx->out.tail = 0;
}

void uctx_flush(ustdio_ctx *ctx) {
	int left = ubuf_len(&ctx->out);
	while (left) {
		unsigned int chunk = STDIO_BUF_SIZE - ctx->out.tail;
		if (chunk > left)
			chunk = left;

		unsigned int accepted =
			fwrite(ctx->dev, ctx->out.dat + ctx->out.tail, chunk);

		if (accepted == 0 || accepted == -1)
			break;

		left -= accepted;
		ctx->out.tail = (ctx->out.tail + accepted) % STDIO_BUF_SIZE;
	}
}

// user stdio flush function
void ustdio_flush(void *ctx) { uctx_flush(ctx); }

// user stdio putc function
void ustdio_putc(void *ctx, char c) {
	ustdio_ctx *uctx = (ustdio_ctx *)ctx;

	// push to buffer
	ubuf_push(&uctx->out, c);
	if (!ubuf_full(&uctx->out) && c != '\n' && c != '\r')
		return;

	// if full or newline, empty buffer
	uctx_flush(uctx);
}

void fputc(int dev, char c) {
	// write single character directly
	fwrite(dev, &c, 1);
}

void fputs(int dev, const char *s) {
	ustdio_ctx ctx;
	uctx_new(&ctx, dev);

	sformat(ustdio_putc, &ctx, s);

	uctx_flush(&ctx);
}

void fputu(int dev, unsigned int n) {
	ustdio_ctx ctx;
	uctx_new(&ctx, dev);

	uformat(ustdio_putc, &ctx, n);

	uctx_flush(&ctx);
}

void fputi(int dev, int n) {
	ustdio_ctx ctx;
	uctx_new(&ctx, dev);

	iformat(ustdio_putc, &ctx, n);

	uctx_flush(&ctx);
}

void fputx(int dev, uint32_t w) {
	ustdio_ctx ctx;
	uctx_new(&ctx, dev);

	xformat(ustdio_putc, &ctx, w);

	uctx_flush(&ctx);
}

void fprintf(int dev, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	ustdio_ctx ctx;
	uctx_new(&ctx, dev);

	vformat(ustdio_putc, &ctx, fmt, ap);

	uctx_flush(&ctx);

	va_end(ap);
}

/* input */

// user stdio getc function
int ustdio_getc(void *ctx) {
	ustdio_ctx *uctx = (ustdio_ctx *)ctx;

	// if empty, try to fill buffer
	while (ubuf_empty(&uctx->in)) {
		uctx->in.head = uctx->in.tail = 0;
		unsigned int accepted = fread(uctx->dev, uctx->in.dat, STDIO_BUF_SIZE);
		uctx->in.head = accepted;
	}

	// pop from buffer
	return ubuf_pop(&uctx->in);
}

int fgetc(int dev) {
	// read single character directly
	int r = '\0';
	fread(dev, (char *)&r, 1);

	return r;
}

int fgets(int dev, char *buf, int siz) {
	ustdio_ctx ctx;
	uctx_new(&ctx, dev);

	return lformat(ustdio_getc, ustdio_putc, ustdio_flush, &ctx, buf, siz);
}

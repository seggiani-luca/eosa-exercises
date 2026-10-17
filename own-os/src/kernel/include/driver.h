#ifndef DRIVER_H
#define DRIVER_H

// character device driver
typedef struct {
	void (*init)(void *ctx);
	unsigned int (*write)(void *ctx, const char *buf, unsigned int len);
	unsigned int (*read)(void *ctx, char *buf, unsigned int len);
	void (*putc)(void *ctx, char c);
	int (*getc)(void *ctx);
	void *ctx;
} char_driver;

// timer device driver
typedef struct {
	void (*init)(void *ctx, int period_ms);
	unsigned int (*tick)(void *ctx);
	void *ctx;
} timer_driver;

#endif

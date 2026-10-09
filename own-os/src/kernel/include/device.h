#ifndef DEVICE_H
#define DEVICE_H

// character device driver
typedef struct {
	void (*init)(void);
	void (*putc)(char c);
	int (*getc)(void);
} char_driver;

#endif

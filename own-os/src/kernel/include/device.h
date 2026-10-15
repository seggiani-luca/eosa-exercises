#ifndef DEVICE_H
#define DEVICE_H

// character device driver
typedef struct {
	void (*init)(void);
	unsigned int (*write)(const char *buf, unsigned int len);
	unsigned int (*read)(char *buf, unsigned int len);
} char_driver;

#endif

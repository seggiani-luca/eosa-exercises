#ifndef DRIVER_H
#define DRIVER_H

#include "../../syscall.h"

// character device driver
typedef struct {
    void (*init)(void *ctx); // initialize device
    unsigned int (*write)(void *ctx,
                          const char *buf,
                          unsigned int len); // write buffer to device (asynchronous)
    unsigned int (*read)(void *ctx,
                         char *buf,
                         unsigned int len); // read buffer from device (first
                                            // character synchronous)
    void (*putc)(void *ctx, char c);        // put character to device (synchronous)
    int (*getc)(void *ctx);                 // read character from device (synchronous)
    void *ctx;                              // device context
} char_driver;

// timer device driver
typedef struct {
    void (*init)(void *ctx, int period_ms); // initialize device
    unsigned int (*tick)(void *ctx);        // get current device tick
    void *ctx;                              // device context
} timer_driver;

// bitwise device driver
typedef struct {
    void (*init)(void *ctx);                            // initialize device
    void (*digital_write)(void *ctx, int idx, int val); // write to pin
    int (*digital_read)(void *ctx, int idx);            // read from pin
    void (*pin_mode)(void *ctx, int idx, int mode);     // set pin mode
    void *ctx;                                          // device context
} bitwise_driver;

#endif

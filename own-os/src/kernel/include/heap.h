#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>

// userspace heap initializer
extern int heap_init();

// heap bounds
extern uint32_t _sheap;
extern uint32_t _eheap;

#endif

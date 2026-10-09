#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>

// saves the current value of PRIMASK
uint32_t irq_save(void);

// restores the old value of PRIMASK
void irq_restore(uint32_t);

#endif

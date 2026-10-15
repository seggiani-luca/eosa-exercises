#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

// initializes the MPU
void mem_init();

// sets up a memory region
void mem_region_set(uint32_t num,	  // region number
					uint32_t base,	  // region base
					uint32_t size,	  // region size
					uint32_t access,  // access mode
					uint32_t execute, // executable?
					uint8_t srd		  // subregion disable
);

// resets a memory region
void mem_region_reset(uint32_t num);

#endif

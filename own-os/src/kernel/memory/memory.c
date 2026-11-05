#include "../include/memory.h"
#include "../include/cmsdk.h"

void fence() {
	__asm volatile("dsb" ::: "memory");
	__asm volatile("isb" ::: "memory");
}

void mem_init() {
	// MPU on with PRIVDEFENA
	MPU->CTRL = MPU_CTRL_ENABLE | MPU_CTRL_PRIVDEFENA;

	fence();
}

void mem_region_set(uint32_t num, uint32_t base, uint32_t size, uint32_t access,
					uint32_t execute, uint8_t srd) {
	// validate
	if (num >= 8)
		return;
	if (size < 32 || (size & (size - 1)) != 0)
		return;
	if (base & (size - 1))
		return;
	if (size < 256 && srd != 0)
		return;

	uint32_t log2_size = __builtin_ctz(size);

	// configure MPU
	MPU->RBAR = base | MPU_RBAR_VALID | num;
	MPU->RASR = MPU_RASR_AP(access) | MPU_RASR_S | MPU_RASR_C |
				MPU_RASR_SIZE(log2_size) | MPU_RASR_ENABLE |
				((uint32_t)srd << 8) | (execute ? 0 : MPU_RASR_XN);

	fence();
}

void mem_region_reset(uint32_t num) {
	// validate
	if (num >= 8)
		return;

	MPU->RNR = num;
	MPU->RASR = 0;

	fence();
}

/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * MPU set-up: see kernel/mpu.h.
 *
 * Prepared from week-04/solution/: the body of mpu_set_task_region was
 * taken out (exercises A1 to A3 of week-05-lab/README.md).
 *
 * Region 0 lets tasks read and run FLASH (code and constants).
 * Region 1 is meant to let a task read and write its own memory,
 * nothing else. Any other access by a task raises a MemManage fault.
 */

#include <stdint.h>
#include "cmsdk.h"
#include "kernel/mpu.h"

void mpu_init( void )
{
    /* Region 0 = FLASH, from address 0, 4 MB = 2^22 bytes.
     * Read-only for everyone, and code may run from it (no XN). */
    MPU->RBAR = 0x00000000 | MPU_RBAR_VALID | 0;       /* = 0x00000010 */
    MPU->RASR = MPU_RASR_AP( MPU_AP_RO ) | MPU_RASR_C
              | MPU_RASR_SIZE( 22 ) | MPU_RASR_ENABLE; /* = 0x0602002B */

    /* Switch the MPU on. PRIVDEFENA: the kernel (privileged) keeps the
     * normal memory map, so only tasks are restricted. */
    MPU->CTRL = MPU_CTRL_ENABLE | MPU_CTRL_PRIVDEFENA;  /* = 5 */
    __asm volatile ( "dsb" ::: "memory" );  /* finish the writes ... */
    __asm volatile ( "isb" ::: "memory" );  /* ... before the next instruction */
}

extern uint32_t _shared_start;
extern uint32_t _shared_end;
extern uint32_t _task_a_part_start;

void mpu_set_task_region( uint32_t base, uint32_t size )
{
	uint32_t log2_size = __builtin_ctz(size);

   	MPU->RBAR = base | MPU_RBAR_VALID | 1; 
	MPU->RASR = MPU_RASR_AP( MPU_AP_FULL_RW ) | MPU_RASR_S | MPU_RASR_C |
				MPU_RASR_SIZE( log2_size ) | MPU_RASR_ENABLE | MPU_RASR_XN;

	// region 2
	if(base != (uint32_t)&_task_a_part_start) {
		log2_size = __builtin_ctz(&_shared_end - &_shared_start);
		MPU->RBAR = (uint32_t)&_shared_start | MPU_RBAR_VALID | 2;
		MPU->RASR = MPU_RASR_AP( MPU_AP_RO ) | MPU_RASR_S | MPU_RASR_C |
					MPU_RASR_SIZE( log2_size ) | MPU_RASR_ENABLE | MPU_RASR_XN;
	} else {
		MPU->RBAR = MPU_RBAR_VALID | 2;
		MPU->RASR = 0;
	}

	__asm volatile ( "dsb" ::: "memory" );  /* finish the writes ... */
    __asm volatile ( "isb" ::: "memory" );  /* ... before the next instruction */
}

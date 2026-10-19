/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * MPU set-up: see kernel/mpu.h.
 *
 * Region 0 lets tasks read and run FLASH (code and constants).
 * Region 1 lets a task read and write its own partition (A1, A2).
 * Region 2 lets every other task read the shared window (A3).
 * Any other access by a task raises a MemManage fault.
 */

#include <stdint.h>
#include "cmsdk.h"
#include "kernel/mpu.h"

/* The shared window, from the linker script. */
extern char _shared_start[], _shared_end[];

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

void mpu_set_task_region( uint32_t base, uint32_t size )
{
    /* A1, A2. The SIZE field holds log2(size) - 1. __builtin_ctz counts
     * the trailing zero bits, which is log2 of a power of two.
     * Example: 4096 = 2^12, so SIZE = 11. */
    uint32_t log2_size = __builtin_ctz( size );

    /* Region 1 = the task's partition: read-write for everyone, but
     * never executable (XN). base must be a multiple of size. */
    MPU->RBAR = base | MPU_RBAR_VALID | 1;
    MPU->RASR = MPU_RASR_XN | MPU_RASR_AP( MPU_AP_FULL_RW )
              | MPU_RASR_S | MPU_RASR_C
              | MPU_RASR_SIZE( log2_size ) | MPU_RASR_ENABLE;

    /* A3. Region 2 = the shared window: 256 bytes, read-only, never
     * executable. The owner already reaches the window through region
     * 1, so it gets no region 2. A region stays programmed until it is
     * written again, so the owner's launch must switch region 2 off. */
    uint32_t w0 = ( uint32_t ) _shared_start;
    uint32_t w1 = ( uint32_t ) _shared_end;
    MPU->RBAR = w0 | MPU_RBAR_VALID | 2;
    if ( w0 >= base && w1 <= base + size )
    {
        MPU->RASR = 0;                      /* the owner: region 2 off */
    }
    else
    {
        MPU->RASR = MPU_RASR_XN | MPU_RASR_AP( MPU_AP_RO )
                  | MPU_RASR_S | MPU_RASR_C
                  | MPU_RASR_SIZE( 8 ) | MPU_RASR_ENABLE;   /* 2^8 = 256 B */
    }

    __asm volatile ( "dsb" ::: "memory" );
    __asm volatile ( "isb" ::: "memory" );
}

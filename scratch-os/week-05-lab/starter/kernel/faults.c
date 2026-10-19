/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Fault handlers. Each one prints its name and the fault status
 * register, then stops. They replace the weak handlers in startup.s.
 */

#include "cmsdk.h"
#include "kernel/kprintf.h"

/* Stop here for ever. */
static void halt( void )
{
    for ( ;; )
    {
    }
}

/* Escalated or serious fault. HFSR tells why it came here. */
void HardFault_Handler( void )
{
    kprintf( "HardFault: HFSR=0x%x\n", SCB->HFSR );
    halt();
}

/* Memory protection fault. CFSR holds its cause bits. */
void MemManage_Handler( void )
{
    uint32_t cfsr = SCB->CFSR;
    kprintf( "MemManage: CFSR=0x%x\n", cfsr );

    /* MMFAR is only meaningful when MMARVALID is set. */
    if ( cfsr & SCB_CFSR_MMARVALID )
        kprintf( "  address 0x%x\n", SCB->MMFAR );

    /* Name the cause. Several bits can be set at once. */
    if ( cfsr & SCB_CFSR_DACCVIOL )
        kprintf( "  cause: data access\n" );
    if ( cfsr & SCB_CFSR_IACCVIOL )
        kprintf( "  cause: instruction fetch\n" );
    if ( cfsr & SCB_CFSR_MSTKERR )
        kprintf( "  cause: stacking\n" );
    halt();
}

/* Bus error on a memory access. */
void BusFault_Handler( void )
{
    kprintf( "BusFault: CFSR=0x%x\n", SCB->CFSR );
    halt();
}

/* Bad instruction, bad state, or divide by zero. */
void UsageFault_Handler( void )
{
    kprintf( "UsageFault: CFSR=0x%x\n", SCB->CFSR );
    halt();
}

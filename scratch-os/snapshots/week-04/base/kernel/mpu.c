/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * MPU set-up: see kernel/mpu.h.
 *
 * Both functions are empty: the MPU stays off until Unit 3.
 */

#include <stdint.h>
#include "cmsdk.h"
#include "kernel/mpu.h"

void mpu_init( void )
{
    /* >>> UNIT 3: region 0 (FLASH) and enable the MPU here. <<< */
}

void mpu_set_task_region( uint32_t base, uint32_t size )
{
    (void) base;
    (void) size;
    /* >>> UNIT 3: region 1 (the task's partition) here. <<< */
}

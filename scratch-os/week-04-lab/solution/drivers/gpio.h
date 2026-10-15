/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * GPIO driver: output pins of CMSDK_GPIO0.
 */
#ifndef GPIO_H
#define GPIO_H

/* Make pin a push-pull output. */
void gpio_set_output( unsigned pin );

/* Drive an output pin high (level 1) or low (level 0). */
void gpio_write( unsigned pin, int level );

#endif /* GPIO_H */

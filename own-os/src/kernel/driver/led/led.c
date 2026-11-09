#include "../../include/led.h"
#include "../../include/cmsdk.h"

bitwise_driver fpgaio_driver = {.init = 0,
                                .digital_write = led_write,
                                .digital_read = button_read,
                                .pin_mode = 0,
                                .ctx = 0};

void led_write(void *ctx, int idx, int val)
{
    if (idx < 0 || idx >= FPGAIO_LEDS)
        return;

    uint32_t mask = 1UL << idx;

    if (val)
        FPGAIO->LED |= mask;
    else
        FPGAIO->LED &= ~mask;
}

int button_read(void *ctx, int idx)
{
    if (idx < 0 || idx >= FPGAIO_BUTTONS)
        return -1;

    uint32_t mask = 1UL << idx;

    return FPGAIO->BUTTON & mask;
}

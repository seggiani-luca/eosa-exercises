#include "../../include/gpio.h"
#include "../../include/cmsdk.h"

// gpio handling block
typedef struct {
	// GPIO
	CMSDK_GPIO_TypeDef *gpio;

	// GPIO pin modes
	int modes[GPIO_PINS];
} gpio_block;

static gpio_block gpio0 = {.gpio = CMSDK_GPIO0};
static gpio_block gpio1 = {.gpio = CMSDK_GPIO1};
static gpio_block gpio2 = {.gpio = CMSDK_GPIO2};
static gpio_block gpio3 = {.gpio = CMSDK_GPIO3};

bitwise_driver gpio0_driver = {.init = gpio_init,
							  .digital_write = gpio_write,
							  .digital_read = gpio_read,
							  .pin_mode = gpio_pin_mode,
							  .ctx = &gpio0};
bitwise_driver gpio1_driver = {.init = gpio_init,
							  .digital_write = gpio_write,
							  .digital_read = gpio_read,
							  .pin_mode = gpio_pin_mode,
							  .ctx = &gpio1};
bitwise_driver gpio2_driver = {.init = gpio_init,
							  .digital_write = gpio_write,
							  .digital_read = gpio_read,
							  .pin_mode = gpio_pin_mode,
							  .ctx = &gpio2};
bitwise_driver gpio3_driver = {.init = gpio_init,
							  .digital_write = gpio_write,
							  .digital_read = gpio_read,
							  .pin_mode = gpio_pin_mode,
							  .ctx = &gpio3};

void gpio_init(void *ctx) {
	gpio_block *bl = ctx;
	uint32_t mask = 0;

	for (int i = 0; i < GPIO_PINS; i++) {
		mask |= 1UL << i;
		bl->modes[i] = MODE_INPUT;
	}

	bl->gpio->OUTENABLECLR = mask;
}

void gpio_write(void *ctx, int idx, int val) {
	if (idx < 0 || idx >= GPIO_PINS)
		return;
	gpio_block *bl = ctx;

	if (bl->modes[idx] != MODE_OUTPUT)
		return;

	uint32_t mask = 1UL << idx;

	if (val)
		bl->gpio->DATAOUT |= mask;
	else
		bl->gpio->DATAOUT &= ~mask;
}

int gpio_read(void *ctx, int idx) {
	if (idx < 0 || idx >= GPIO_PINS)
		return -1;
	gpio_block *bl = ctx;

	if (bl->modes[idx] != MODE_INPUT)
		return -2;

	uint32_t mask = 1UL << idx;

	return bl->gpio->DATA & mask;
}

void gpio_pin_mode(void *ctx, int idx, int mode) {
	if (idx < 0 || idx >= GPIO_PINS)
		return;
	gpio_block *bl = ctx;

	uint32_t mask = 1UL << idx;

	switch (mode) {
	case MODE_INPUT:
		bl->gpio->OUTENABLECLR = mask;
		bl->modes[idx] = MODE_INPUT;
		break;
	case MODE_OUTPUT:
		bl->gpio->OUTENABLESET = mask;
		bl->modes[idx] = MODE_OUTPUT;
		break;
	default:
		break;
	}
}

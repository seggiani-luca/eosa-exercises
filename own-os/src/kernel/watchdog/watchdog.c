#include "../include/watchdog.h"
#include "../include/cmsdk.h"

void watchdog_init(int period_ms) {
	// unlock
	CMSDK_WDOG->LOCK = WDOG_UNLOCK_KEY;

	CMSDK_WDOG->LOAD = 25000U * period_ms;
	CMSDK_WDOG->CTRL = WDOG_CTRL_INTEN | WDOG_CTRL_RESEN;

	// lock
	CMSDK_WDOG->LOCK = 1;
}

void watchdog_feed() {
	// unlock
	CMSDK_WDOG->LOCK = WDOG_UNLOCK_KEY;

	CMSDK_WDOG->INTCLR = 1;

	// lock
	CMSDK_WDOG->LOCK = 1;
}

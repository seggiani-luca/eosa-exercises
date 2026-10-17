#ifndef WATCHDOG_H
#define WATCHDOG_H

// initializes the watchdog
void watchdog_init(int period_ms);

// feeds the watchdog a bone
void watchdog_feed();

#endif

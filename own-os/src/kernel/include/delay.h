#ifndef DELAY_H
#define DELAY_H

#include "../include/driver.h"
#include "../include/process.h"

// timer queue definition
typedef process_queue timer_queue;

// push to timer queue
void timer_queue_push(timer_queue *queue, process *proc, unsigned int ms);

// advance time in timer queue
void timer_queue_advance(timer_queue *queue, unsigned int ms);

#endif

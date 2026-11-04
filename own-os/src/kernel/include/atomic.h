#ifndef ATOMIC_H
#define ATOMIC_H

#include "process.h"
#include <stdint.h>

/* test and set */

// exclusive load on word
uint32_t ldrex(uint32_t *addr);

// exclusive store on word
uint32_t strex(uint32_t *addr, uint32_t dat);

// data memory barrier
void dmb();

/* semaphores */

// number of static semaphores
#define SEMAPHORE_NUM 64

// definition of semaphore
typedef struct semaphore {
	int tokens;
	process_queue queue;
	struct semaphore *next;
	int alloc;
} semaphore;

// initializes semaphore heap
void ksem_init_heap();

// checks if semaphore address is valid
int is_ksem_valid(semaphore *sem);

// reserves a semaphore
semaphore *ksem_new(int initial);

// locks a semaphore
void ksem_lock(semaphore *sem, process *proc);

// unlocks a semaphore
void ksem_unlock(semaphore *sem);

// frees a semaphore
void ksem_free(semaphore *sem);

/* mutexes */

// number of static mutexes
#define MUTEX_NUM 64

// definition of mutex
typedef struct mutex {
	process *owner;
	process_queue queue;
	struct mutex *next;
	int alloc;
} mutex;

// initializes mutex heap
void kmutex_init_heap();

// checks if mutex address is valid
int is_kmutex_valid(mutex *mut);

// reserves a mutex
mutex *kmutex_new();

// locks a mutex
void kmutex_lock(mutex *mut, process *proc);

// unlocks a mutex
void kmutex_unlock(mutex *mut, process *proc);

// frees a mutex
void kmutex_free(mutex *mut);

#endif

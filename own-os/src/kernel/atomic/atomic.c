#include "../include/atomic.h"
#include "../include/irq.h"

/* semaphores */

// semaphore buffer
static semaphore semaphore_buf[SEMAPHORE_NUM];

// free semaphore list
static semaphore *semaphore_free = 0;

void ksem_init_heap() {
	semaphore_free = 0;

	for (int i = SEMAPHORE_NUM - 1; i >= 0; i--) {
		semaphore_buf[i].next = semaphore_free;
		semaphore_free = &semaphore_buf[i];
		semaphore_buf[i].alloc = 0;
	}
}

int is_ksem_valid(semaphore *sem) {
	uintptr_t addr = (uintptr_t)sem;
	uintptr_t start = (uintptr_t)&semaphore_buf[0];
	uintptr_t end = (uintptr_t)&semaphore_buf[SEMAPHORE_NUM];

	return addr >= start && addr < end &&
		   (addr - start) % sizeof(semaphore) == 0 && sem->alloc;
}

semaphore *ksem_new(int initial) {
	if (!semaphore_free)
		return 0;

	uint32_t saved = irq_save();

	semaphore *sem = semaphore_free;
	semaphore_free = semaphore_free->next;

	sem->tokens = initial;
	sem->queue = 0;
	sem->next = 0;
	sem->alloc = 1;

	irq_restore(saved);
	return sem;
}

void ksem_lock(semaphore *sem, process *proc) {
	uint32_t saved = irq_save();

	if (sem->tokens > 0)
		sem->tokens--;
	else {
		proc->state = PROC_BLOCKED;
		process_queue_push(&sem->queue, proc);
	}

	irq_restore(saved);
}

void ksem_unlock(semaphore *sem) {
	uint32_t saved = irq_save();

	if (sem->queue) {
		process *proc = process_queue_pop(&sem->queue);
		proc->state = PROC_READY;
		process_queue_push(queue_from_prio(proc->prio), proc);
	} else
		sem->tokens++;

	irq_restore(saved);
}

void ksem_free(semaphore *sem) {
	uint32_t saved = irq_save();

	// free queue
	while (sem->queue) {
		process *proc = process_queue_pop(&sem->queue);
		proc->state = PROC_READY;
		process_queue_push(queue_from_prio(proc->prio), proc);
	}

	sem->alloc = 0;
	sem->next = semaphore_free;
	semaphore_free = sem;

	irq_restore(saved);
}

/* mutexes */

// mutex buffer
static mutex mutex_buf[MUTEX_NUM];

// free mutex list
static mutex *mutex_free = 0;

void kmutex_init_heap() {
	mutex_free = 0;

	for (int i = MUTEX_NUM - 1; i >= 0; i--) {
		mutex_buf[i].next = mutex_free;
		mutex_free = &mutex_buf[i];
		mutex_buf[i].alloc = 0;
	}
}

int is_kmutex_valid(mutex *mut) {
	uintptr_t addr = (uintptr_t)mut;
	uintptr_t start = (uintptr_t)&mutex_buf[0];
	uintptr_t end = (uintptr_t)&mutex_buf[MUTEX_NUM];

	return addr >= start && addr < end && (addr - start) % sizeof(mutex) == 0 &&
		   mut->alloc;
}

mutex *kmutex_new() {
	if (!mutex_free)
		return 0;

	uint32_t saved = irq_save();

	mutex *mut = mutex_free;
	mutex_free = mutex_free->next;

	mut->owner = 0;
	mut->queue = 0;
	mut->next = 0;
	mut->alloc = 1;

	irq_restore(saved);
	return mut;
}

void kmutex_lock(mutex *mut, process *proc) {
	uint32_t saved = irq_save();

	if (!mut->owner)
		mut->owner = proc;
	else {
		proc->state = PROC_BLOCKED;
		process_queue_push(&mut->queue, proc);
	}

	irq_restore(saved);
}

void kmutex_unlock(mutex *mut, process *proc) {
	uint32_t saved = irq_save();

	if (mut->owner != proc) {
		return; // enforce ownership
		irq_restore(saved);
	}

	if (mut->queue) {
		process *proc = process_queue_pop(&mut->queue);
		mut->owner = proc;
		proc->state = PROC_READY;
		process_queue_push(queue_from_prio(proc->prio), proc);
	} else
		mut->owner = 0;

	irq_restore(saved);
}

void kmutex_free(mutex *mut) {
	uint32_t saved = irq_save();

	// free queue
	while (mut->queue) {
		process *proc = process_queue_pop(&mut->queue);
		proc->state = PROC_READY;
		process_queue_push(queue_from_prio(proc->prio), proc);
	}

	mut->alloc = 0;
	mut->next = mutex_free;
	mutex_free = mut;

	irq_restore(saved);
}

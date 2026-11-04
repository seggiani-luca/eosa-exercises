#include "../include/lib.h"

PROCESS_PRIORITY(PRIO_HIGH)

int a = 0;
volatile int b_done = 0;

#if defined(USE_SPINLOCK)

spinlock l = 0;

#define LOCK() spin_lock(&l)
#define UNLOCK() spin_unlock(&l)

#elif defined(USE_SEMAPHORE)

semaphore_t s;

#define LOCK() sem_lock(s)
#define UNLOCK() sem_unlock(s)

#elif defined(USE_MUTEX)

mutex_t m;

#define LOCK() mutex_lock(m)
#define UNLOCK() mutex_unlock(m)

#else
#error "Select a synchronization primitive"
#endif

int plus_one(int v) {
	v++;

	for (volatile int j = 0; j < 10; j++)
		;

	return v;
}

int process_a_main(void) {
#if defined(USE_SEMAPHORE)

	s = sem_new(1);

#elif defined(USE_MUTEX)

	m = mutex_new();

#endif

	for (int i = 0; i < 1000000; i++) {
		LOCK();
		a = plus_one(a);
		UNLOCK();
	}

	while (!b_done)
		yield();

	printf("A sees a=%d\n", a);
	return 0;
}

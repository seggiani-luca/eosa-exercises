#include "../include/lib.h"

PROCESS_PRIORITY(PRIO_HIGH)

extern int a;
extern volatile int b_done;
extern int plus_one(int);

#if defined(USE_SPINLOCK)

extern spinlock l;

#define LOCK() spin_lock(&l)
#define UNLOCK() spin_unlock(&l)

#elif defined(USE_SEMAPHORE)

extern semaphore_t s;

#define LOCK() sem_lock(s)
#define UNLOCK() sem_unlock(s)

#elif defined(USE_MUTEX)

extern mutex_t m;

#define LOCK() mutex_lock(m)
#define UNLOCK() mutex_unlock(m)

#else
#error "Select a synchronization primitive"
#endif

int process_b_main(void) {
	for (int i = 0; i < 1000000; i++) {
		LOCK();
		a = plus_one(a);
		UNLOCK();
	}

	b_done = 1;
	return 0;
}

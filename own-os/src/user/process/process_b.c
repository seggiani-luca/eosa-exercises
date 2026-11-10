#include "../include/lib.h"
PROCESS_PRIORITY(PRIO_HIGH)

// test parameters
#define ITERS 10
#define SIZE 200

SHARE semaphore_t sb;
extern semaphore_t sa;

int process_b_main(void)
{
	sb = sem_new(0);

    void *ptrs[ITERS];

    for (int i = 0; i < ITERS; i++) {
        ptrs[i] = malloc(SIZE);

		printf("[process_b] ");
        if (ptrs[i])
            printf("allocated region at %x\n", ptrs[i]);
        else
            printf("allocation failed!\n");

		yield();
    }

    printf("[process_b] deallocating all regions...\n");
    for (int i = 0; i < ITERS; i++) {
        free(ptrs[i]);
    }

	sem_unlock(sb);
	sem_lock(sa);

    for (int i = 0; i < ITERS; i++) {
        ptrs[i] = malloc(SIZE);

		printf("[process_b] ");
        if (ptrs[i])
            printf("allocated region at %x\n", ptrs[i]);
        else
            printf("allocation failed!\n");
		
		yield();
    }

    return 0;
}

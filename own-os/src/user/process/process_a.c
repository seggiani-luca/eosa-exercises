#include "../include/lib.h"
PROCESS_PRIORITY(PRIO_HIGH)

// test parameters
#define ITERS 10
#define SIZE 200

SHARE semaphore_t sa;
extern semaphore_t sb;

int process_a_main(void)
{
	sa = sem_new(0);

    void *ptrs[ITERS];

    for (int i = 0; i < ITERS; i++) {
        ptrs[i] = malloc(SIZE);

		printf("[process_a] ");
        if (ptrs[i])
            printf("allocated region at %x\n", ptrs[i]);
        else
            printf("allocation failed!\n");

		yield();
    }

    printf("[process_a] deallocating all regions...\n");
    for (int i = 0; i < ITERS; i++) {
        free(ptrs[i]);
    }

	sem_unlock(sa);
	sem_lock(sb);

    for (int i = 0; i < ITERS; i++) {
        ptrs[i] = malloc(SIZE);

		printf("[process_a] ");
        if (ptrs[i])
            printf("allocated region at %x\n", ptrs[i]);
        else
            printf("allocation failed!\n");
		
		yield();
    }

    return 0;
}

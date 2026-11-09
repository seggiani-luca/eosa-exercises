#include "../include/lib.h"
PROCESS_PRIORITY(PRIO_HIGH)

int process_a_main(void)
{
    int *ptr = malloc(sizeof(int));
    if (!ptr) {
        printf("malloc() failed\n");
        return 1;
    }

    *ptr = 420;
    printf("*ptr=%d\n", *ptr);

    free(ptr);

    return 0;
}

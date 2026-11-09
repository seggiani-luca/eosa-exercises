#include "../../../kernel/include/atomic.h"
#include "../../include/lib.h"

void spin_lock(spinlock *lock)
{
    for (;;) {
        if (ldrex(lock) != 0)
            yield();
        if (strex(lock, 1) == 0)
            break;
    }

    dmb();
}

void spin_unlock(spinlock *lock)
{
    dmb();

    *lock = 0;
}

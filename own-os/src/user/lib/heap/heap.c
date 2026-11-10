#include "../../include/lib.h"
#include <stdint.h>

// heap is structured as follows:
//
// +---------------------------------------+ _sheap
// | heap_control_t (28 B)                 |
// +---------------------------------------+ _sheap + 28
// | heap_block descriptors (8 B each)     |
// +---------------------------------------+ _sheap + 28 + 8 * num_blocks
// |                                       |
// | SMALL region (variable)               |
// |                                       |
// +---------------------------------------+
// |                                       |
// | MEDIUM region (variable)              |
// |                                       |
// è---------------------------------------+
// |                                       |
// | LARGE region (variable)               |
// |                                       |
// +---------------------------------------+ _eheap

// userspace heap parameters
#define HEAP_SIZE 0x1000 /* 4 KiB */

#define HEAP_SMALL_BLOCK_SIZE 64   // size of SMALL heap block
#define HEAP_MEDIUM_BLOCK_SIZE 128 // size of MEDIUM heap block
#define HEAP_LARGE_BLOCK_SIZE 256  // size of LARGE heap block

#define HEAP_LARGE_BOUND (HEAP_SIZE / 2)  // LARGE block beg.
#define HEAP_MEDIUM_BOUND (HEAP_SIZE / 4) // MEDIUM block beg.

// the actual heap
__attribute__((section(".heap"))) uint8_t heap[HEAP_SIZE];

// heap block (8 bytes)
typedef struct heap_block {
    uintptr_t start;         // region start, 0 signals taken
    struct heap_block *next; // next region in list
} heap_block;

// heap control information
typedef struct {
    // free block lists
    heap_block *free_small_blocks;
    heap_block *free_medium_blocks;
    heap_block *free_large_blocks;

    // block counters
    unsigned int num_small_blocks;
    unsigned int num_medium_blocks;
    unsigned int num_large_blocks;

    unsigned int num_blocks;
} heap_control_t;

// maps to heap control information in actual heap
#define heap_control ((heap_control_t *)(void *)heap)

// maps to beginning of blocks region in actual heap
#define blocks ((heap_block *)(void *)(heap + sizeof(heap_control_t)))

// helper for block allocation
int allocate_block(uintptr_t *cur_end,
                   uintptr_t *cur_block,
                   int *cur_block_idx,
                   unsigned int block_size,
                   heap_block ***next)
{
    if (*cur_end <= *cur_block)
        return -1;

    // allocate block
    heap_block *block = &blocks[(*cur_block_idx)++];
    *cur_block += sizeof(heap_block);

    // setup block
    block->start = (uintptr_t)heap + *cur_end;
    block->next = 0;
    **next = block;
    *next = &block->next;

    // allocate space
    *cur_end -= block_size;

    heap_control->num_blocks++;

    return 0;
}

// initializes the heap
int heap_init()
{
    // initialize control info
    *heap_control = (typeof(*heap_control)){0};

    // heap cursors
    uintptr_t cur_end = HEAP_SIZE;
    uintptr_t cur_block = sizeof(*heap_control);

    // block cursors
    int cur_block_idx = 0;

    // pre-allocate space
    cur_end -= HEAP_LARGE_BLOCK_SIZE;

    // allocate LARGE blocks
    heap_block **next = &heap_control->free_large_blocks;
    while (cur_end > HEAP_LARGE_BOUND) {
        if (allocate_block(&cur_end,
                           &cur_block,
                           &cur_block_idx,
                           HEAP_LARGE_BLOCK_SIZE,
                           &next))
            return -1;

        heap_control->num_large_blocks++;
    }

    // allocate MEDIUM blocks
    next = &heap_control->free_medium_blocks;
    while (cur_end > HEAP_MEDIUM_BOUND) {
        if (allocate_block(&cur_end,
                           &cur_block,
                           &cur_block_idx,
                           HEAP_MEDIUM_BLOCK_SIZE,
                           &next))
            return -1;

        heap_control->num_medium_blocks++;
    }

    // allocate SMALL blocks
    next = &heap_control->free_small_blocks;
    do {
        if (allocate_block(&cur_end,
                           &cur_block,
                           &cur_block_idx,
                           HEAP_SMALL_BLOCK_SIZE,
                           &next))
            return -1;

        heap_control->num_small_blocks++;
    } while (cur_end > cur_block);

    return 0;
}

void *malloc(unsigned int size)
{
    if (size == 0)
        return 0;

    heap_block **free_blocks = 0;

    // choose block list
    if (size > HEAP_LARGE_BLOCK_SIZE)
        return 0;
    if (size > HEAP_MEDIUM_BLOCK_SIZE)
        free_blocks = &heap_control->free_large_blocks;
    else if (size > HEAP_SMALL_BLOCK_SIZE)
        free_blocks = &heap_control->free_medium_blocks;
    else
        free_blocks = &heap_control->free_small_blocks;

    // allocate
    if (!*free_blocks)
        return 0;

    uintptr_t ret = (*free_blocks)->start;
    (*free_blocks)->start = 0; // flag as taken

    *free_blocks = (*free_blocks)->next;

    return (void *)ret;
}

// helper for block recovery
heap_block *recover_block(uintptr_t ptr,
                          unsigned int block_size,
                          unsigned int block_base,
                          unsigned int block_count,
                          uintptr_t first_offset)
{
    // compute base, check against valid heap
    uintptr_t base = (uintptr_t)heap;
    if (ptr < base || ptr >= base + HEAP_SIZE)
        return 0;

    // compute offset, check against first
    uintptr_t offset = ptr - base;
    if (offset > first_offset)
        return 0;

    // compute delta from first
    uintptr_t delta = first_offset - offset;
    if (delta % block_size)
        return 0;

    // delta correspons to index, check against actual num.
    unsigned int idx = delta / block_size;
    if (idx >= block_count)
        return 0;

    return &blocks[block_base + idx];
}

void free(void *ptr)
{
    if (!ptr)
        return;

    heap_block **free_blocks = 0;
    heap_block *block = 0;

    // choose block
    uintptr_t beg = (uintptr_t)ptr - (uintptr_t)heap;
    if (beg > HEAP_LARGE_BOUND && beg < HEAP_SIZE) {
        free_blocks = &heap_control->free_large_blocks;
        block = recover_block((uintptr_t)ptr,
                              HEAP_LARGE_BLOCK_SIZE,
                              0,
                              heap_control->num_large_blocks,
                              HEAP_SIZE - HEAP_LARGE_BLOCK_SIZE);

    } else if (beg > HEAP_MEDIUM_BOUND && beg <= HEAP_LARGE_BOUND) {
        free_blocks = &heap_control->free_medium_blocks;
        block = recover_block((uintptr_t)ptr,
                              HEAP_MEDIUM_BLOCK_SIZE,
                              heap_control->num_large_blocks,
                              heap_control->num_medium_blocks,
                              HEAP_LARGE_BOUND);

    } else if (beg >= sizeof(*heap_control)
                          + heap_control->num_blocks * sizeof(heap_block)
               && beg <= HEAP_MEDIUM_BOUND) {
        free_blocks = &heap_control->free_small_blocks;
        block = recover_block((uintptr_t)ptr,
                              HEAP_SMALL_BLOCK_SIZE,
                              heap_control->num_large_blocks
                                  + heap_control->num_medium_blocks,
                              heap_control->num_small_blocks,
                              HEAP_MEDIUM_BOUND);

    } else {
        printf("[user] trying to free() outside heap at %x\n", ptr);
        return;
    }

    if (!block) {
        printf("[user] misaligned free() at %x\n", ptr);
        return;
    }

    if (block->start != 0) {
        printf("[user] double free() at %x\n", ptr);
        return;
    }

    // deallocate
    block->start = (uintptr_t)ptr;
    block->next = *free_blocks;
    *free_blocks = block;
}

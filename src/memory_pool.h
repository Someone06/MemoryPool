#ifndef DFS_MEMORY_POOL_H
#define DFS_MEMORY_POOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct MemoryNode {
    struct MemoryNode *neighbours;
} MemoryNode;

uint16_t memoryNode_get_neighbour_count(MemoryNode const *memoryNode);
MemoryNode *memoryNode_getNeighbour(MemoryNode const *memoryNode, const uint16_t index);
void memoryNode_setNeighbour(MemoryNode *memoryNode, MemoryNode const *const neighbour, const uint16_t index);
void *memoryNode_get_data(MemoryNode const *memoryNode);

/*
 * Assumes a 64-bit pointer type, where the top most 16 bits are 0 and the
 * lowest bit is 0.
 */
typedef struct MemoryPoolNode {
    /*
     * High 16 bits denote the size of the memory this node governs.
     * Lowest bit denotes whether the space has been allocated:
     *     0 means allocated, 1 means free.
     *
     *  Unused bits are the second and third-lowest bit, as well as bit 47.
     */
    struct MemoryPoolNode *next;
} MemoryPoolNode;

typedef struct {
    void *space;
    MemoryPoolNode *head;
    MemoryNode **rootSet;
    size_t rootSetSize;
    size_t rootSetCapacity;
} MemoryPool;

typedef void (*free_fn)(void *);

MemoryPool memory_pool_new(size_t pool_size);
void memoryPool_free(MemoryPool *memoryPool, void (*free_data)(void *));

/*
 * Requires that  `data size + sizeof(void*) * neighbours < 1ULL << 16`.
 */
MemoryNode *memoryPool_alloc(MemoryPool *memoryPool, size_t data_size, size_t neighbours);
bool memoryPool_add_root_node(MemoryPool *memoryPool, MemoryNode *memoryNode);
void memoryPool_gc_mark_and_sweep(MemoryPool *memoryPool, free_fn f);


#endif

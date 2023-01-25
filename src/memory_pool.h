#ifndef DFS_MEMORY_POOL_H
#define DFS_MEMORY_POOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * This type represents user allocated memory in the pool.
 * Each node can have reference to other nodes in the pool.
 */
typedef struct MemoryNode {
    struct MemoryNode *neighbours;
} MemoryNode;

uint16_t memoryNode_get_neighbour_count(MemoryNode const *memoryNode);
MemoryNode *memoryNode_getNeighbour(MemoryNode const *memoryNode, uint16_t index);
void memoryNode_setNeighbour(MemoryNode *memoryNode, MemoryNode const *neighbour, uint16_t index);
void *memoryNode_get_data(MemoryNode const *memoryNode);

/*
 * A type that is internal to MemoryPool, but cannot be hidden from this header
 * file.
 *
 * DO NOT USE!
 */
typedef struct MemoryPoolNode {
    struct MemoryPoolNode *next;
} MemoryPoolNode;

/*
 * The MemoryPool holds a certain amount of memory from which MemoryNodes can be
 * allocated. The pool is garbage collected.
 *
 * A MemoryNode is considered garbage, if it is not a root node and if it can
 * not be (transitively) reached by from any root node.
 */
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

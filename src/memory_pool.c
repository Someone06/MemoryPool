#include <assert.h>
#include <string.h>

#include "memory.h"
#include "memory_pool.h"
#include "pointer_bit_hacks.h"

// ---------- Memory Pool Node ----------
static MemoryPoolNode *memoryPoolNode_new(void *const location, MemoryPoolNode *const next, const uint16_t size, const bool is_free) {
    assert(sizeof(MemoryPoolNode *) == 8);
    assert(extract_top_bits(location) == 0);
    assert(extract_lowest_bit(location) == 0);

    MemoryPoolNode *const n = set_lowest_bit(set_top_bits(next, size), is_free);
    MemoryPoolNode *const node = location;
    node->next = n;
    return node;
}

static MemoryPoolNode *memoryPoolNode_get_next(MemoryPoolNode const *const memoryPoolNode) {
    MemoryPoolNode *const node = memoryPoolNode->next;
    return extract_ptr_bits(node);
}

static void memoryPoolNode_set_next(MemoryPoolNode *const memoryPoolNode, MemoryPoolNode const *const next) {
    MemoryPoolNode *const node = memoryPoolNode->next;
    const uint16_t size = extract_top_bits(node);
    const bool is_free = extract_lowest_bit(node);
    memoryPoolNode->next = set_lowest_bit(set_top_bits(next, size), is_free);
}

static uint16_t memoryPoolNode_get_free_space(MemoryPoolNode const *const memoryPoolNode) {
    MemoryPoolNode *const node = memoryPoolNode->next;
    return extract_top_bits(node);
}

static bool memoryPoolNode_is_free(MemoryPoolNode const *const memoryPoolNode) {
    MemoryPoolNode *const node = memoryPoolNode->next;
    return extract_lowest_bit(node);
}

static void memoryPoolNode_set_is_free(MemoryPoolNode *const memoryPoolNode, const bool is_free) {
    MemoryPoolNode *const node = memoryPoolNode->next;
    memoryPoolNode->next = set_lowest_bit(node, is_free);
}

static void *memoryPoolNode_get_data(MemoryPoolNode const *const memoryPoolNode) {
    return (char *) memoryPoolNode + sizeof(MemoryPoolNode);
}


// ---------- Memory Node ----------
static MemoryNode *memoryNode_new(void *location, const uint16_t neighbours) {
    assert(~neighbours);

    memset(location, 0, sizeof(void *) * neighbours);
    MemoryNode *const node = location;
    node->neighbours = set_top_bits(NULL, neighbours);
    return node;
}

static bool memoryNode_is_marked(MemoryNode const *const memoryNode) {
    MemoryNode *const neighbours = memoryNode->neighbours;
    return extract_lowest_bit(neighbours);
}

static void memoryNode_set_is_marked(MemoryNode *const memoryNode, const bool isMarked) {
    MemoryNode *const neighbours = memoryNode->neighbours;
    memoryNode->neighbours = set_lowest_bit(neighbours, isMarked);
}

static void **memoryNode_ptr_to_neighbour_ptr(MemoryNode const *const memoryNode, const uint16_t index) {
    assert(index <= memoryNode_get_neighbour_count(memoryNode) == 0 ? 1 : memoryNode_get_neighbour_count(memoryNode));
    return (void **) ((uintptr_t) memoryNode + sizeof(void *) * index);
}

static uint16_t memoryNode_get_counter(MemoryNode const *const memoryNode) {
    const uint16_t count = memoryNode_get_neighbour_count(memoryNode);
    assert(count > 1);

    void const *const second = *memoryNode_ptr_to_neighbour_ptr(memoryNode, 1);
    return extract_top_bits(second);
}

static uint16_t memoryNode_inc_counter(MemoryNode *const memoryNode) {
    const uint16_t count = memoryNode_get_neighbour_count(memoryNode);
    assert(count > 1);

    void **const second = memoryNode_ptr_to_neighbour_ptr(memoryNode, 1);
    uintptr_t new_counter_value = extract_top_bits(*second) + 1;
    *second = set_top_bits(*second, new_counter_value);
    return new_counter_value;
}

static void memoryNode_reset_counter(MemoryNode *const memoryNode) {
    const uint16_t count = memoryNode_get_neighbour_count(memoryNode);
    assert(count > 1);

    void **const second = memoryNode_ptr_to_neighbour_ptr(memoryNode, 1);
    *second = set_top_bits(*second, 0);
}

uint16_t memoryNode_get_neighbour_count(MemoryNode const *const memoryNode) {
    MemoryNode *const neighbours = memoryNode->neighbours;
    return extract_top_bits(neighbours);
}

MemoryNode *memoryNode_getNeighbour(MemoryNode const *const memoryNode, const uint16_t index) {
    void *const ptr = *memoryNode_ptr_to_neighbour_ptr(memoryNode, index);
    return extract_ptr_bits(ptr);
}

void *memoryNode_get_data(MemoryNode const *const memoryNode) {
    uint16_t size = memoryNode_get_neighbour_count(memoryNode);
    if (size == 0) size = 1;

    return memoryNode_ptr_to_neighbour_ptr(memoryNode, size);
}

void memoryNode_setNeighbour(MemoryNode *const memoryNode, MemoryNode const *const neighbour, const uint16_t index) {
    void **const ptr = memoryNode_ptr_to_neighbour_ptr(memoryNode, index);
    const uint16_t top = extract_top_bits(*ptr);
    const bool low_bit = extract_lowest_bit(*ptr);
    void *const decorated = set_lowest_bit(set_top_bits(neighbour, top), low_bit);
    *ptr = decorated;
}


// ---------- Memory Pool ----------
static const size_t DEFAULT_ROOT_SET_SIZE = 8;
static const size_t MemoryPoolNode_MaxMemoryPerNode = (1ULL << 16) - 1;

MemoryPool memory_pool_new(const size_t pool_size, const FreeFn freeFn) {
    assert(pool_size >= sizeof(MemoryPoolNode));


    void *const space = MALLOC(pool_size);
    void *const rootSet = MALLOC(DEFAULT_ROOT_SET_SIZE * sizeof(MemoryNode *));

    if (!space || !rootSet) {
        FREE(space);
        FREE(rootSet);
        MemoryPool pool;
        memset(&pool, 0, sizeof(MemoryPool));
        return pool;
    }

    assert(((uintptr_t) space & 7) == 0);

#define min(a, b) (a) <= (b) ? (a) : (b)

    size_t remaining_size = pool_size - sizeof(MemoryPoolNode);
    const size_t head_size = min(remaining_size, MemoryPoolNode_MaxMemoryPerNode);
    MemoryPoolNode *const head = memoryPoolNode_new(space, NULL, head_size, true);
    remaining_size -= head_size;
    MemoryPoolNode *current = head;
    char *remaining_space = (char *) space + sizeof(MemoryPoolNode) + head_size;

    while (remaining_size > sizeof(MemoryPoolNode)) {
        remaining_size -= sizeof(MemoryPoolNode);
        const size_t node_size = min(remaining_size, MemoryPoolNode_MaxMemoryPerNode);
        remaining_size -= node_size;

        MemoryPoolNode *const next = memoryPoolNode_new(remaining_space, NULL, node_size, true);
        memoryPoolNode_set_next(current, next);
        current = next;

        remaining_space = remaining_space + sizeof(MemoryPoolNode) + node_size;
    }

#undef min

    return (MemoryPool){.head = space, .rootSet = rootSet, .rootSetSize = 0, .rootSetCapacity = DEFAULT_ROOT_SET_SIZE, .freeFn = freeFn};
}

void memoryPool_free(MemoryPool *const memoryPool) {
    if (memoryPool->freeFn) {
        MemoryPoolNode *node = memoryPool->head;
        while (node) {
            if (!memoryPoolNode_is_free(node)) {
                memoryPool->freeFn(memoryNode_get_data(memoryPoolNode_get_data(node)));
            }

            node = memoryPoolNode_get_next(node);
        }
    }

    FREE(memoryPool->rootSet);
    FREE(memoryPool->head);
    memset(memoryPool, 0, sizeof(MemoryPool));
}

MemoryNode *memoryPool_alloc(MemoryPool *const memoryPool, const size_t data_size, const size_t neighbours) {
    const size_t memoryNode_size = sizeof(MemoryNode *) * (neighbours == 0 ? 1 : neighbours);
    const size_t total_size = memoryNode_size + data_size;
    assert(total_size < 1ULL << 16);

    MemoryPoolNode *head = memoryPool->head;
    while (head && (!memoryPoolNode_is_free(head) || memoryPoolNode_get_free_space(head) < total_size)) {
        head = memoryPoolNode_get_next(head);
    }

    if (!head)
        return NULL;

    memoryPoolNode_set_is_free(head, false);
    void *const space = memoryPoolNode_get_data(head);
    MemoryNode *const memoryNode = memoryNode_new(space, neighbours);

    const size_t total_space = memoryPoolNode_get_free_space(head);
    const size_t remaining_space = total_space - total_size;
    if (remaining_space > sizeof(MemoryPoolNode)) {
        void *const location = (char *) space + total_size;
        MemoryPoolNode *next = memoryPoolNode_new(location, memoryPoolNode_get_next(head), remaining_space - sizeof(MemoryPoolNode), true);
        memoryPoolNode_set_next(head, next);
    }


    return memoryNode;
}

bool memoryPool_add_root_node(MemoryPool *const memoryPool, MemoryNode *const memoryNode) {
    if (memoryPool->rootSetSize == memoryPool->rootSetCapacity) {
        MemoryNode **newSet = REALLOC(memoryPool->rootSet, memoryPool->rootSetCapacity, memoryPool->rootSetCapacity * sizeof(MemoryNode *) * 2);
        if (!newSet)
            return false;

        memoryPool->rootSet = newSet;
        memoryPool->rootSetCapacity *= 2;
    }

    memoryPool->rootSet[memoryPool->rootSetSize++] = memoryNode;
    return true;
}

static void dfs(MemoryNode *current, void (*const for_each)(void *)) {

#define BACK_OFF                                                                \
    while (true) {                                                              \
        memoryNode_reset_counter(current);                                      \
        next = current;                                                         \
        current = previous;                                                     \
        if (current == NULL)                                                    \
            break;                                                              \
                                                                                \
        previous = memoryNode_getNeighbour(current, 0);                         \
        const uint16_t preNeighbours = memoryNode_get_neighbour_count(current); \
        memoryNode_setNeighbour(current, next, preNeighbours);                  \
        if (preNeighbours >= 2) {                                               \
            memoryNode_inc_counter(current);                                    \
            break;                                                              \
        }                                                                       \
    }

#define FORWARD                                                      \
    while (true) {                                                   \
        next = memoryNode_getNeighbour(current, 0);                  \
        if (memoryNode_is_marked(next)) {                            \
            BACK_OFF                                                 \
            break;                                                   \
        }                                                            \
                                                                     \
        memoryNode_set_is_marked(next, true);                        \
        if (for_each != NULL)                                        \
            for_each(memoryNode_get_data(next));                     \
                                                                     \
        const int neighbours = memoryNode_get_neighbour_count(next); \
        if (neighbours == 0) {                                       \
            BACK_OFF                                                 \
            break;                                                   \
        }                                                            \
                                                                     \
        memoryNode_setNeighbour(current, previous, 0);               \
        previous = current;                                          \
        current = next;                                              \
        if (neighbours != 1) {                                       \
            break;                                                   \
        }                                                            \
    }

    if (current == NULL || memoryNode_is_marked(current))
        return;

    if (for_each != NULL)
        for_each(memoryNode_get_data(current));

    memoryNode_set_is_marked(current, true);

    const uint16_t neighbours = memoryNode_get_neighbour_count(current);
    if (neighbours == 0)
        return;

    MemoryNode *previous = NULL, *next = NULL;
    if (neighbours == 1) {
        FORWARD
    }

    while (current != NULL) {
        const uint16_t neighbours = memoryNode_get_neighbour_count(current);
        assert(neighbours > 1);

        const uint16_t counter = memoryNode_get_counter(current);
        if (counter == neighbours) {
            BACK_OFF
            continue;
        }

        next = memoryNode_getNeighbour(current, counter);
        if (next == NULL || memoryNode_is_marked(next)) {
            memoryNode_inc_counter(current);
            continue;
        }

        if (for_each != NULL)
            for_each(memoryNode_get_data(next));

        const int next_neighbours = memoryNode_get_neighbour_count(next);
        if (next_neighbours == 0)
            continue;

        memoryNode_setNeighbour(current, previous, counter);
        previous = current;
        current = next;

        if (next_neighbours > 1)
            continue;

        FORWARD
    }

#undef FORWARD
#undef BACK_OFF
}

static void memoryPool_gc_mark(MemoryPool *const memoryPool) {
    const size_t rootSetSize = memoryPool->rootSetSize;
    for (int i = 0; i < rootSetSize; ++i)
        dfs(memoryPool->rootSet[i], NULL);
}


static void memoryPool_gc_sweep(MemoryPool *const memoryPool) {
    MemoryPoolNode *current = memoryPool->head;
    FreeFn f = memoryPool->freeFn;
    while (current) {
        if (memoryPoolNode_is_free(current))
            goto next;

        MemoryNode *memoryNode = memoryPoolNode_get_data(current);
        const bool is_marked = memoryNode_is_marked(memoryNode);
        if (is_marked) {
            memoryNode_set_is_marked(memoryNode, false);
            goto next;
        }

        void *data = memoryNode_get_data(memoryNode);
        if (f) f(data);
        memoryPoolNode_set_is_free(current, true);
        goto next;

    next:
        current = memoryPoolNode_get_next(current);
    }
}

void memoryPool_gc_mark_and_sweep(MemoryPool *const memoryPool) {
    memoryPool_gc_mark(memoryPool);
    memoryPool_gc_sweep(memoryPool);
}

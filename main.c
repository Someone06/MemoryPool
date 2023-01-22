#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>
#include <string.h>

#include "src/memory.h"
#include "src/pointer_bit_hacks.h"

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
    return extract_ptr_bits(memoryPoolNode->next);
}

static void memoryPoolNode_set_next(MemoryPoolNode *const memoryPoolNode, MemoryPoolNode const *const next) {
    const uint16_t size = extract_top_bits(memoryPoolNode->next);
    const bool is_free = extract_lowest_bit(memoryPoolNode->next);
    memoryPoolNode->next = set_lowest_bit(set_top_bits(next, size), is_free);
}

static uint16_t memoryPoolNode_get_free_space(MemoryPoolNode const *const memoryPoolNode) {
    return extract_top_bits(memoryPoolNode);
}

static bool memoryPoolNode_is_free(MemoryPoolNode const *const memoryPoolNode) {
    return extract_lowest_bit(memoryPoolNode->next);
}

static void memoryPoolNode_set_is_free(MemoryPoolNode *const memoryPoolNode, const bool is_free) {
    memoryPoolNode->next = set_lowest_bit(memoryPoolNode->next, is_free);
}

static void *memoryPoolNode_get_data(MemoryPoolNode const *const memoryPoolNode) {
    return (char *) memoryPoolNode + sizeof(MemoryPoolNode);
}

static const size_t MemoryPoolNode_MaxMemoryPerNode = (1ULL << 16) - 1;

typedef struct {
    void *neighbours;
} MemoryNode;

static MemoryNode *memoryNode_new(void *location, const uint16_t neighbours) {
    assert(~neighbours);

    memset(location, 0, sizeof(void *) * neighbours);
    MemoryNode *const node = location;
    node->neighbours = set_top_bits(NULL, neighbours);
    return node;
}

uint16_t memoryNode_get_neighbour_count(MemoryNode const *const memoryNode) {
    return extract_top_bits(memoryNode->neighbours);
}

static bool memoryNode_is_marked(MemoryNode const *const memoryNode) {
    return extract_lowest_bit(memoryNode->neighbours);
}

static void memoryNode_set_is_marked(MemoryNode *const memoryNode, const bool isMarked) {
    memoryNode->neighbours = set_lowest_bit(memoryNode->neighbours, isMarked);
}

static void **memoryNode_ptr_to_neighbour_ptr(MemoryNode const *const memoryNode, const uint16_t index) {
    assert(index < memoryNode_get_neighbour_count(memoryNode));
    return (void **) ((uintptr_t) memoryNode + sizeof(void *) * index);
}

void *memoryNode_get_data(MemoryNode const *const memoryNode) {
    uint16_t size = memoryNode_get_neighbour_count(memoryNode);
    if (size == 0) size = 1;

    return *memoryNode_ptr_to_neighbour_ptr(memoryNode, size);
}


MemoryNode *memoryNode_getNeighbour(MemoryNode const *const memoryNode, const uint16_t index) {
    void *const ptr = *memoryNode_ptr_to_neighbour_ptr(memoryNode, index);
    return extract_ptr_bits(ptr);
}

void memoryNode_setNeighbour(MemoryNode *const memoryNode, MemoryNode const *const neighbour, const uint16_t index) {
    void **const ptr = memoryNode_ptr_to_neighbour_ptr(memoryNode, index);
    const uint16_t top = extract_top_bits(*ptr);
    const bool low_bit = extract_lowest_bit(*ptr);
    void *const decorated = set_lowest_bit(set_top_bits(neighbour, top), low_bit);
    *ptr = decorated;
}

static uint16_t memoryNode_get_counter(MemoryNode const *const memoryNode) {
    const uint16_t count = memoryNode_get_neighbour_count(memoryNode);
    assert(count);

    if (count == 1)
        return 0;

    void const *const second = *memoryNode_ptr_to_neighbour_ptr(memoryNode, 1);
    return extract_top_bits(second);
}

static uint16_t memoryNode_inc_counter(MemoryNode *const memoryNode) {
    const uint16_t count = memoryNode_get_neighbour_count(memoryNode);
    assert(count);

    if (count == 1)
        return 0;

    void **const second = memoryNode_ptr_to_neighbour_ptr(memoryNode, 1);
    uintptr_t new_counter_value = extract_top_bits(*second) + 1;
    if (new_counter_value == count) new_counter_value = 0;

    *second = set_top_bits(*second, new_counter_value);
    return new_counter_value;
}

typedef struct {
    void *space;
    MemoryPoolNode *head;
    MemoryNode *rootSet;
    size_t rootSetSize;
    size_t rootSetCapacity;
} MemoryPool;

static const size_t DEFAULT_ROOT_SET_SIZE = 8;


MemoryPool memory_pool_new(const size_t pool_size) {
    assert(pool_size >= sizeof(MemoryPoolNode));


    void *const space = MALLOC(pool_size);
    void *const rootSet = MALLOC(DEFAULT_ROOT_SET_SIZE);

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

    return (MemoryPool){.space = space, .head = space, .rootSet = rootSet, .rootSetSize = 0, .rootSetCapacity = DEFAULT_ROOT_SET_SIZE};
}

void memoryPool_free(MemoryPool *const memoryPool, void (*free_data)(void *)) {
    if (free_data) {
        MemoryPoolNode *node = memoryPool->head;
        while (node) {
            if (!memoryPoolNode_is_free(node)) {
                free_data(memoryNode_get_data(memoryPoolNode_get_data(node)));
            }

            node = memoryPoolNode_get_next(node);
        }
    }

    FREE(memoryPool->rootSet);
    FREE(memoryPool->space);
    memset(memoryPool, 0, sizeof(MemoryPool));
}

/*
 * Requires that  `data size + sizeof(void*) * neighbours < 1ULL << 16`.
 */
MemoryNode *memoryPool_alloc(MemoryPool *const memoryPool, const size_t data_size, const size_t neighbours) {
    const size_t memoryNode_size = sizeof(void *) * (neighbours == 0 ? 1 : neighbours);
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

bool memoryPool_add_root_node(MemoryPool * const memoryPool, MemoryNode * const memoryNode) {
       if(memoryPool->rootSetSize == memoryPool->rootSetCapacity) {
            MemoryNode * newSet = REALLOC(memoryPool->rootSet, memoryPool->rootSetCapacity, memoryPool->rootSetCapacity * 2);
            if(!newSet)
                return false;

            memoryPool->rootSet = newSet;
            memoryPool->rootSetCapacity += 2;
       }

       memoryPool->rootSet[memoryPool->rootSetSize++] = memoryNode;
       return true;
}


/*

void dfs(MemoryNode *current, void (*const for_each)(void *)) {
    if (current == NULL) {
        return;
    }

    if (for_each != NULL) {
        for_each(current->data);
    }
    toggle_marked(current);

    const bool marking = is_marked(current);
    MemoryNode *previous = NULL, *next = NULL;
    while (current != NULL) {
        if (get_ptr_count(current) < current->neighbour_count) {
            next = current->neighbours[get_ptr_count(current)];
            if (next != NULL && is_marked(next) != marking) {
                if (for_each != NULL) {
                    for_each(next->data);
                }
                toggle_marked(next);
                current->neighbours[get_ptr_count(current)] = previous;
                previous = current;
                current = next;
            } else {
                inc_ptr_count(current);
            }
        } else {
            reset_ptr_count(current);
            next = current;
            current = previous;
            if (current != NULL) {
                previous = current->neighbours[get_ptr_count(current)];
                current->neighbours[get_ptr_count(current)] = next;
                inc_ptr_count(current);
            }
        }
    }
}

void reduce_to_dfs_tree(MemoryNode *current) {
    if (current == NULL) {
        return;
    }

    toggle_marked(current);

    const bool marking = is_marked(current);
    MemoryNode *previous = NULL, *next = NULL;
    while (current != NULL) {
        if (get_ptr_count(current) < current->neighbour_count) {
            next = current->neighbours[get_ptr_count(current)];
            if (next != NULL && is_marked(next) != marking) {
                toggle_marked(next);
                current->neighbours[get_ptr_count(current)] = previous;
                previous = current;
                current = next;
            } else {
                current->neighbours[get_ptr_count(current)] = NULL;
                inc_ptr_count(current);
            }
        } else {
            reset_ptr_count(current);
            next = current;
            current = previous;
            if (current != NULL) {
                previous = current->neighbours[get_ptr_count(current)];
                current->neighbours[get_ptr_count(current)] = next;
                inc_ptr_count(current);
            }
        }
    }
}

void free_nodes(MemoryNode *current, void (*const free_data)(void *)) {
    if (current == NULL) {
        return;
    }

    reduce_to_dfs_tree(current);

    MemoryNode *previous = NULL, *next = NULL;
    while (current != NULL) {
        if (get_ptr_count(current) < current->neighbour_count) {
            next = current->neighbours[get_ptr_count(current)];
            if (next != NULL) {
                toggle_marked(next);
                current->neighbours[get_ptr_count(current)] = previous;
                previous = current;
                current = next;
            } else {
                inc_ptr_count(current);
            }
        } else {
            if (free_data != NULL) {
                free_data(current->data);
            }
            FREE(current);
            current = previous;
            if (current != NULL) {
                previous = current->neighbours[get_ptr_count(current)];
                inc_ptr_count(current);
            }
        }
    }
}

int *data = NULL;
size_t data_size = 0;

void init_out(const size_t size) {
    data = CALLOC(size, sizeof(int));
    data_size = size;
}

void free_out() {
    FREE(data);
    data = NULL;
    data_size = 0;
}

void inc_out(void *value) {
    const int index = *(int *) value;
    assert(index >= 0 && index < data_size);
    ++data[index];
}

bool all_same(const int value) {
    for (int i = 0; i < data_size; ++i) {
        if (data[i] != value) {
            return false;
        }
    }

    return true;
}

void test_nodes_inc_by_one(MemoryNode *root, size_t count) {
    init_out(count);
    dfs(root, inc_out);
    assert(all_same(1));
    free_nodes(root, NULL);
    free_out();
}

void test_list() {
    int id[3] = {0, 1, 2};
    MemoryNode *a = new_node(&id[0], 1);
    MemoryNode *b = new_node(&id[1], 1);
    MemoryNode *c = new_node(&id[2], 0);
    a->neighbours[0] = b;
    b->neighbours[0] = c;

    test_nodes_inc_by_one(a, 3);
}

void test_triangle() {
    int id[3] = {0, 1, 2};
    MemoryNode *a = new_node(&id[0], 1);
    MemoryNode *b = new_node(&id[1], 1);
    MemoryNode *c = new_node(&id[2], 1);
    a->neighbours[0] = b;
    b->neighbours[0] = c;
    c->neighbours[0] = a;

    test_nodes_inc_by_one(a, 3);
}

void test_windmill() {
    int id[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    MemoryNode *a = new_node(&id[0], 2);
    MemoryNode *b = new_node(&id[1], 2);
    MemoryNode *c = new_node(&id[2], 2);
    MemoryNode *d = new_node(&id[3], 2);

    MemoryNode *e = new_node(&id[4], 0);
    MemoryNode *f = new_node(&id[5], 0);
    MemoryNode *g = new_node(&id[6], 0);
    MemoryNode *h = new_node(&id[7], 0);

    a->neighbours[0] = b;
    b->neighbours[0] = c;
    c->neighbours[0] = d;
    d->neighbours[0] = a;

    a->neighbours[1] = e;
    b->neighbours[1] = f;
    c->neighbours[1] = g;
    d->neighbours[1] = h;

    test_nodes_inc_by_one(a, 8);
}

void test_8() {
    int id[6] = {0, 1, 2, 3, 4, 5};
    MemoryNode *a = new_node(&id[0], 2);
    MemoryNode *b = new_node(&id[1], 2);
    MemoryNode *c = new_node(&id[2], 2);
    MemoryNode *d = new_node(&id[3], 2);
    MemoryNode *e = new_node(&id[4], 2);
    MemoryNode *f = new_node(&id[5], 2);

    a->neighbours[0] = b;
    b->neighbours[0] = c;
    c->neighbours[0] = d;
    d->neighbours[0] = a;

    b->neighbours[1] = e;
    e->neighbours[0] = f;
    f->neighbours[0] = c;
    c->neighbours[1] = b;

    test_nodes_inc_by_one(b, 6);
}

void test_self_loop() {
    int id[1] = {0};
    MemoryNode *a = new_node(&id[0], 1);
    a->neighbours[0] = a;

    test_nodes_inc_by_one(a, 1);
}

int main() {
    test_list();
    test_triangle();
    test_windmill();
    test_8();
    test_self_loop();
}
 */

int main() {}

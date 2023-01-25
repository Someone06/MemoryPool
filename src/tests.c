#include "tests.h"
#include "assert.h"
#include "memory_pool.h"

static const size_t DEFAULT_POOL_SIZE = 1ULL << 10;

void test_alloc_pool() {
    MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE);
    memoryPool_free(&pool, NULL);
}

void test_alloc_pool_2() {
    MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE - 1);
    memoryPool_free(&pool, NULL);
}

void test_alloc_node() {
    MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE);
    MemoryNode *const node = memoryPool_alloc(&pool, sizeof(uint64_t), 0);
    int *const data = memoryNode_get_data(node);
    *data = 42;
    memoryPool_free(&pool, NULL);
}

void test_alloc_multiple() {
    MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE);

    MemoryNode *const node = memoryPool_alloc(&pool, sizeof(uint64_t), 0);
    int *const data = memoryNode_get_data(node);
    *data = 42;

    MemoryNode *const node2 = memoryPool_alloc(&pool, sizeof(uint64_t), 0);
    int *const data2 = memoryNode_get_data(node2);
    *data2 = 36;

    MemoryNode *const node3 = memoryPool_alloc(&pool, sizeof(uint64_t), 0);
    assert(*data == 42);
    assert(*data2 == 36);

    memoryPool_free(&pool, NULL);
}

void test_add_to_root_set() {
    MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE);
    MemoryNode *const node = memoryPool_alloc(&pool, sizeof(uint64_t), 0);
    int *const data = memoryNode_get_data(node);
    *data = 42;
    memoryPool_add_root_node(&pool, node);
    assert(*data == 42);
    memoryPool_free(&pool, NULL);
}

void test_set_neighbour() {
    MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE);

    MemoryNode *const node = memoryPool_alloc(&pool, sizeof(uint64_t), 1);
    int *const data = memoryNode_get_data(node);
    *data = 42;

    MemoryNode *const node2 = memoryPool_alloc(&pool, sizeof(uint64_t), 2);
    int *const data2 = memoryNode_get_data(node2);
    *data2 = 36;
    memoryNode_setNeighbour(node, node2, 0);

    MemoryNode *const node3 = memoryPool_alloc(&pool, sizeof(uint64_t), 2);
    int *const data3 = memoryNode_get_data(node3);
    *data3 = 1337;
    memoryNode_setNeighbour(node2, node3, 1);

    assert(memoryNode_get_neighbour_count(node) == 1);
    assert(memoryNode_get_neighbour_count(node2) == 2);
    assert(memoryNode_get_neighbour_count(node3) == 2);

    assert(memoryNode_getNeighbour(node, 0) == node2);
    assert(memoryNode_getNeighbour(node2, 0) == NULL);
    assert(memoryNode_getNeighbour(node2, 1) == node3);

    assert(*data == 42);
    assert(*data2 == 36);
    assert(*data3 == 1337);
    memoryPool_free(&pool, NULL);
}

void run_tests() {
    test_alloc_pool();
    test_alloc_pool_2();
    test_alloc_node();
    test_alloc_multiple();
    test_add_to_root_set();
    test_set_neighbour();
}

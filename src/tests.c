#include "tests.h"
#include "assert.h"
#include "memory.h"
#include "memory_pool.h"

static const size_t DEFAULT_POOL_SIZE = 1ULL << 10;

static void test_alloc_pool() {
    MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE, NULL);
    memoryPool_free(&pool);
}

static void test_alloc_pool_2() {
    MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE - 1, NULL);
    memoryPool_free(&pool);
}

static void test_alloc_node() {
    MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE, NULL);
    MemoryNode *const node = memoryPool_alloc(&pool, sizeof(uint64_t), 0);
    int *const data = memoryNode_get_data(node);
    *data = 42;
    memoryPool_free(&pool);
}

static void test_alloc_multiple() {
    MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE, NULL);

    MemoryNode *const node = memoryPool_alloc(&pool, sizeof(uint64_t), 0);
    int *const data = memoryNode_get_data(node);
    *data = 42;

    MemoryNode *const node2 = memoryPool_alloc(&pool, sizeof(uint64_t), 0);
    int *const data2 = memoryNode_get_data(node2);
    *data2 = 36;

    MemoryNode *const node3 = memoryPool_alloc(&pool, sizeof(uint64_t), 0);
    assert(*data == 42);
    assert(*data2 == 36);

    memoryPool_free(&pool);
}

static void test_add_to_root_set() {
    MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE, NULL);
    MemoryNode *const node = memoryPool_alloc(&pool, sizeof(uint64_t), 0);
    int *const data = memoryNode_get_data(node);
    *data = 42;
    memoryPool_add_root_node(&pool, node);
    assert(*data == 42);
    memoryPool_free(&pool);
}

static void test_set_neighbour() {
    MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE, NULL);

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
    memoryPool_free(&pool);
}

uint64_t * data = NULL;
size_t data_size = 0;

static void init_out(const size_t size) {
    data = CALLOC(size, sizeof(uint64_t));
    data_size = size;
}

static void free_out() {
    FREE(data);
    data = NULL;
    data_size = 0;
}

static void inc_out(MemoryNode const *node) {
    const uint64_t index = *(uint64_t *) memoryNode_get_data(node);
    assert(index >= 0 && index < data_size);
    ++data[index];
}

static bool all_same(const int value) {
    for (int i = 0; i < data_size; ++i) {
        if (data[i] != value) {
            return false;
        }
    }

    return true;
}

static void test_nodes_inc_by_one(MemoryNode *root, size_t count) {
    init_out(count);
    memoryPool_dfs(root, inc_out);
    assert(all_same(1));
    free_out();
}

static void test_dfs_triangle() {
    MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE, NULL);

    MemoryNode *const node1 = memoryPool_alloc(&pool, sizeof(uint64_t), 1);
    *(uint64_t *)memoryNode_get_data(node1) =  0;

    MemoryNode *const node2 = memoryPool_alloc(&pool, sizeof(uint64_t), 1);
    *(uint64_t *)memoryNode_get_data(node2) = 1;

    MemoryNode *const node3 = memoryPool_alloc(&pool, sizeof(uint64_t), 1);
    *(uint64_t *)memoryNode_get_data(node3) = 2;

    memoryNode_setNeighbour(node1, node2, 0);
    memoryNode_setNeighbour(node2, node3, 0);
    memoryNode_setNeighbour(node3, node1, 0);

    test_nodes_inc_by_one(node1, 3);
    memoryPool_free(&pool);
}

static void test_dfs_list() {
     MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE, NULL);

    MemoryNode *const node1 = memoryPool_alloc(&pool, sizeof(uint64_t), 1);
    *(uint64_t *)memoryNode_get_data(node1) =  0;

    MemoryNode *const node2 = memoryPool_alloc(&pool, sizeof(uint64_t), 1);
    *(uint64_t *)memoryNode_get_data(node2) = 1;

    MemoryNode *const node3 = memoryPool_alloc(&pool, sizeof(uint64_t), 1);
    *(uint64_t *)memoryNode_get_data(node3) = 2;

    memoryNode_setNeighbour(node1, node2, 0);
    memoryNode_setNeighbour(node2, node3, 0);

    test_nodes_inc_by_one(node1, 3);
    memoryPool_free(&pool);
}

static void test_dfs_single_node() {
     MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE, NULL);

    MemoryNode *const node1 = memoryPool_alloc(&pool, sizeof(uint64_t), 0);
    *(uint64_t *)memoryNode_get_data(node1) =  0;

    test_nodes_inc_by_one(node1, 1);
    memoryPool_free(&pool);
}

static void test_dfs_bin_tree() {
     MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE, NULL);

    MemoryNode *const node1 = memoryPool_alloc(&pool, sizeof(uint64_t), 2);
    *(uint64_t *)memoryNode_get_data(node1) =  0;

    MemoryNode *const node2 = memoryPool_alloc(&pool, sizeof(uint64_t), 2);
    *(uint64_t *)memoryNode_get_data(node2) = 1;

    MemoryNode *const node3 = memoryPool_alloc(&pool, sizeof(uint64_t), 2);
    *(uint64_t *)memoryNode_get_data(node3) = 2;

    MemoryNode *const node4 = memoryPool_alloc(&pool, sizeof(uint64_t), 0);
    *(uint64_t *)memoryNode_get_data(node4) = 3;

    MemoryNode *const node5 = memoryPool_alloc(&pool, sizeof(uint64_t), 0);
    *(uint64_t *)memoryNode_get_data(node5) = 4;

    MemoryNode *const node6 = memoryPool_alloc(&pool, sizeof(uint64_t), 1);
    *(uint64_t *)memoryNode_get_data(node6) = 5;

    MemoryNode *const node7 = memoryPool_alloc(&pool, sizeof(uint64_t), 1);
    *(uint64_t *)memoryNode_get_data(node7) = 6;

    memoryNode_setNeighbour(node1, node2, 0);
    memoryNode_setNeighbour(node1, node3, 1);
    memoryNode_setNeighbour(node2, node4, 0);
    memoryNode_setNeighbour(node2, node5, 1);
    memoryNode_setNeighbour(node3, node6, 0);
    memoryNode_setNeighbour(node3, node7, 1);
    memoryNode_setNeighbour(node7, node1, 0);

    test_nodes_inc_by_one(node1, 7);
    memoryPool_free(&pool);
}

static void test_dfs_split_path() {
    MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE, NULL);

    MemoryNode *const node1 = memoryPool_alloc(&pool, sizeof(uint64_t), 2);
    *(uint64_t *) memoryNode_get_data(node1) = 0;

    MemoryNode *const node2 = memoryPool_alloc(&pool, sizeof(uint64_t), 1);
    *(uint64_t *) memoryNode_get_data(node2) = 1;

    MemoryNode *const node3 = memoryPool_alloc(&pool, sizeof(uint64_t), 1);
    *(uint64_t *) memoryNode_get_data(node3) = 2;

    MemoryNode *const node4 = memoryPool_alloc(&pool, sizeof(uint64_t), 0);
    *(uint64_t *) memoryNode_get_data(node4) = 3;

    MemoryNode *const node5 = memoryPool_alloc(&pool, sizeof(uint64_t), 1);
    *(uint64_t *) memoryNode_get_data(node5) = 4;

    MemoryNode *const node6 = memoryPool_alloc(&pool, sizeof(uint64_t), 1);
    *(uint64_t *) memoryNode_get_data(node6) = 5;


    memoryNode_setNeighbour(node1, node2, 0);
    memoryNode_setNeighbour(node2, node3, 0);
    memoryNode_setNeighbour(node3, node4, 0);
    memoryNode_setNeighbour(node1, node5, 1);
    memoryNode_setNeighbour(node5, node6, 0);
    memoryNode_setNeighbour(node6, node4, 0);

    test_nodes_inc_by_one(node1, 6);
    memoryPool_free(&pool);
}

static void free_fn(void* data) {
    **(uint64_t**) data = 1;
}

static void test_free_nodes_single() {
    MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE, free_fn);
    init_out(1);
    MemoryNode *const node = memoryPool_alloc(&pool, sizeof(uint64_t*), 0);
    *(uint64_t **) memoryNode_get_data(node) = &data[0];
    memoryPool_free(&pool);
    assert(all_same(1));
    free_out();
}

static void test_collected_nodes_single() {
    MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE, free_fn);
    init_out(1);
    MemoryNode *const node = memoryPool_alloc(&pool, sizeof(uint64_t*), 0);
    *(uint64_t **) memoryNode_get_data(node) = &data[0];
    memoryPool_gc_mark_and_sweep(&pool);
    assert(all_same(1));
    free_out();

    init_out(1);
    MemoryNode *const node2 = memoryPool_alloc(&pool, sizeof(uint64_t*), 0);
    *(uint64_t **) memoryNode_get_data(node2) = &data[0];
    memoryPool_add_root_node(&pool, node2);
    memoryPool_gc_mark_and_sweep(&pool);
    assert(all_same(0));
    memoryPool_free(&pool);
    free_out();
}

static void test_create_large_memory_pool() {
    MemoryPool pool = memory_pool_new(1ULL << 20, NULL);
    memoryPool_free(&pool);
}

static void test_alloc_odd_size_data() {
    MemoryPool pool = memory_pool_new((1ULL << 20) - 7, free_fn);

    int counter = 0;
    for(int i = 1; i < 77; i += 17) ++counter;
    init_out(counter);

    counter = 0;
    MemoryNode * next = NULL;
    for(int i = 1; i < 77; i += 17) {
       MemoryNode * node = memoryPool_alloc(&pool, i + sizeof(uint64_t*), 1);
       *(uint64_t **) memoryNode_get_data(node) = &data[counter];
       memoryNode_setNeighbour(node, next, 0);
       next = node;
       ++counter;
    }

    memoryPool_gc_mark_and_sweep(&pool);
    assert(all_same(1));
    memoryPool_free(&pool);
    free_out();
}

static void test_many_root_nodes() {
    MemoryPool pool = memory_pool_new(DEFAULT_POOL_SIZE, free_fn);

    init_out(10);
    MemoryNode const * next = NULL;
    for(int i = 0; i < 10; ++i) {
        MemoryNode * node = memoryPool_alloc(&pool, sizeof(uint64_t*), 1);
        *(uint64_t **) memoryNode_get_data(node) = &data[i];
        memoryNode_setNeighbour(node, next, 0);
        memoryPool_add_root_node(&pool, node);
        next = node;
    }

    memoryPool_gc_mark_and_sweep(&pool);
    assert(all_same(0));
    memoryPool_free(&pool);
    free_out();
}

void run_tests() {
    test_alloc_pool();
    test_alloc_pool_2();
    test_alloc_node();
    test_alloc_multiple();
    test_add_to_root_set();
    test_set_neighbour();
    test_dfs_triangle();
    test_dfs_list();
    test_dfs_bin_tree();
    test_dfs_single_node();
    test_dfs_split_path();
    test_free_nodes_single();
    test_collected_nodes_single();
    test_create_large_memory_pool();
    test_alloc_odd_size_data();
    test_many_root_nodes();
}

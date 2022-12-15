#ifdef NDEBUG
    #undef NDEBUG
#endif

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Allow for replacing malloc and free operations, which is need for custom
// allocators.
static void *(*custom_malloc)(size_t) = malloc;
static void (*custom_free)(void *) = free;

void * MALLOC(const size_t size) {
    void * const p = custom_malloc(size);
    return p;
}

size_t align_8(const size_t size) {
    return (size + 7) & ~7;
}

void * CALLOC(const size_t member_count, const size_t member_size) {
    const size_t nb = align_8(member_size) * member_count;
    void * const p = MALLOC(nb);
    if(p != NULL) {
        memset(p, 0, nb);
    }
    return p;
}

void FREE(void * const p) {
    custom_free(p);
}

struct Node {
    void *data;
    size_t ptrCount;
    size_t neighbour_count;
    struct Node *neighbours[];
};

typedef struct Node Node;

const size_t MARKED_BIT_OFFSET = sizeof(size_t) - 1;

bool is_marked(Node const *const node) {
    return node->ptrCount >> MARKED_BIT_OFFSET;
}

void toggle_marked(Node *const node) {
    node->ptrCount ^= ((size_t) 1) << MARKED_BIT_OFFSET;
}

size_t get_ptr_count(Node *const node) {
    return node->ptrCount & ((((size_t) 1) << MARKED_BIT_OFFSET) - 1);
}

void inc_ptr_count(Node *const node) {
    ++node->ptrCount;
}

void reset_ptr_count(Node *const node) {
    node->ptrCount &= ((size_t) 1) << MARKED_BIT_OFFSET;
}

Node *new_node(void *const data, const size_t neighbours) {
    assert(!(neighbours >> MARKED_BIT_OFFSET));
    Node *const node = CALLOC(1, sizeof(Node) + sizeof(Node *) * neighbours);
    if(node != NULL) {
        node->data = data;
        node->neighbour_count = neighbours;
    }

    return node;
}

/*
 * Perform a Depth-First Search starting at the given root node and apply the
 * given for_each function, to the data of each node. for_each can be NULL.
 */
void dfs(Node *current, void (*const for_each)(void *)) {
    if (current == NULL) {
        return;
    }

    if (for_each != NULL) {
        for_each(current->data);
    }
    toggle_marked(current);

    const bool marking = is_marked(current);
    Node *previous = NULL, *next = NULL;
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

/*
 * Removes edges from the graph, s.t. the remaining graph is a tree.
 */
void reduce_to_dfs_tree(Node *current) {
    if (current == NULL) {
        return;
    }

    toggle_marked(current);

    const bool marking = is_marked(current);
    Node *previous = NULL, *next = NULL;
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

/*
 * Free all nodes of the graph after applying the free_data function to the data
 * of each node. free_data can be NULL.
 */
void free_nodes(Node *current, void (*const free_data)(void *)) {
    if (current == NULL) {
        return;
    }

    reduce_to_dfs_tree(current);

    Node *previous = NULL, *next = NULL;
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

void test_nodes_inc_by_one(Node *root, size_t count) {
    init_out(count);
    dfs(root, inc_out);
    assert(all_same(1));
    free_nodes(root, NULL);
    free_out();
}

void test_list() {
    int id[3] = {0, 1, 2};
    Node *a = new_node(&id[0], 1);
    Node *b = new_node(&id[1], 1);
    Node *c = new_node(&id[2], 0);
    a->neighbours[0] = b;
    b->neighbours[0] = c;

    test_nodes_inc_by_one(a, 3);
}

void test_triangle() {
    int id[3] = {0, 1, 2};
    Node *a = new_node(&id[0], 1);
    Node *b = new_node(&id[1], 1);
    Node *c = new_node(&id[2], 1);
    a->neighbours[0] = b;
    b->neighbours[0] = c;
    c->neighbours[0] = a;

    test_nodes_inc_by_one(a, 3);
}

void test_windmill() {
    int id[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    Node *a = new_node(&id[0], 2);
    Node *b = new_node(&id[1], 2);
    Node *c = new_node(&id[2], 2);
    Node *d = new_node(&id[3], 2);

    Node *e = new_node(&id[4], 0);
    Node *f = new_node(&id[5], 0);
    Node *g = new_node(&id[6], 0);
    Node *h = new_node(&id[7], 0);

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
    Node *a = new_node(&id[0], 2);
    Node *b = new_node(&id[1], 2);
    Node *c = new_node(&id[2], 2);
    Node *d = new_node(&id[3], 2);
    Node *e = new_node(&id[4], 2);
    Node *f = new_node(&id[5], 2);

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
    Node *a = new_node(&id[0], 1);
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

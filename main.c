#ifdef NDEBUG
    #undef NDEBUG
#endif

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

size_t get_ptr_count(Node * const node) {
   return node->ptrCount & ((((size_t) 1) << MARKED_BIT_OFFSET) - 1);
}

void inc_ptr_count(Node * const node) {
   ++node->ptrCount;
}

void reset_ptr_count(Node * const node) {
    node->ptrCount &= ((size_t) 1) << MARKED_BIT_OFFSET;
}

Node *new_node(void *const data, const size_t neighbours) {
    assert(!(neighbours >> MARKED_BIT_OFFSET));
    Node *const node = calloc(1, sizeof(Node) + sizeof(Node *) * neighbours);
    node->data = data;
    node->neighbour_count = neighbours;
    return node;
}

void dfs(Node *current, void (*const for_each)(void *)) {
    if(current == NULL) {
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
ä               inc_ptr_count(current);
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

void reduce_to_dfs_tree(Node *current) {
     if(current == NULL) {
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

void free_nodes(Node *current, void (*const free_data)(void *)) {
    if(current == NULL) {
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
            if(free_data != NULL) {
                free_data(current->data);
            }
            free(current);
            current = previous;
            if (current != NULL) {
                previous = current->neighbours[get_ptr_count(current)];
                inc_ptr_count(current);
            }
        }
    }
}

int * data = NULL;
size_t data_size = 0;

void init_out(const size_t size) {
   data = calloc(size, sizeof(int));
   data_size = size;
}

void free_out() {
    free(data);
    data = NULL;
    data_size = 0;
}

void inc_out(void * value) {
   const int index = * (int*) value;
   assert(index >= 0 && index < data_size);
   ++data[index];
}

bool all_same(const int value) {
    for(int i = 0; i < data_size; ++i) {
       if(data[i] != value) {
           return false;
       }
    }

    return true;
}

void test_nodes_inc_by_one(Node * root, size_t count) {
    init_out(count);
    dfs(root, inc_out);
    assert(all_same(1));
    free_nodes(root, NULL);
    free_out();
}

void test_triangle() {
    int id[3] = {0, 1, 2};
    Node * a = new_node(&id[0], 1);
    Node * b = new_node(&id[1], 1);
    Node * c = new_node(&id[2], 1);
    a->neighbours[0] = b;
    b->neighbours[0] = c;
    c->neighbours[0] = a;

    test_nodes_inc_by_one(a, 3);
}

void test_list() {
    int id[3] = {0, 1, 2};
    Node * a = new_node(&id[0], 1);
    Node * b = new_node(&id[1], 1);
    Node * c = new_node(&id[2], 0);
    a->neighbours[0] = b;
    b->neighbours[0] = c;

    test_nodes_inc_by_one(a, 3);
}

int main() {
    test_list();
    test_triangle();
}

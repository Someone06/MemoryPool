#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

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

Node *new_node(void *const data, const size_t neighbours) {
    assert(!(neighbours >> MARKED_BIT_OFFSET));
    Node *const node = calloc(1, sizeof(Node) + sizeof(Node *) * neighbours);
    node->data = data;
    node->neighbour_count = neighbours;
    return node;
}

void dfs(Node *current, void (*const for_each)(void *)) {
    Node *previous = NULL, *next = NULL;
    const bool marking = is_marked(current);

    while (current != NULL) {
        if (current->ptrCount < current->neighbour_count) {
            next = current->neighbours[current->ptrCount];
            if (next != NULL && is_marked(next) != marking) {
                if (for_each != NULL) {
                    for_each(next->data);
                }
                toggle_marked(next);
                current->neighbours[current->ptrCount] = previous;
                previous = current;
                current = next;
            } else {
                ++current->ptrCount;
            }
        } else {
            current->ptrCount = 0;
            next = current;
            current = previous;
            if (current != NULL) {
                previous = current->neighbours[current->ptrCount];
                current->neighbours[current->ptrCount] = next;
                ++current->ptrCount;
            }
        }
    }
}

void reduce_to_dfs_tree(Node *current) {
    Node *previous = NULL, *next = NULL;
    const bool marking = is_marked(current);

    while (current != NULL) {
        if (current->ptrCount < current->neighbour_count) {
            next = current->neighbours[current->ptrCount];
            if (next != NULL && is_marked(next) != marking) {
                toggle_marked(next);
                current->neighbours[current->ptrCount] = previous;
                previous = current;
                current = next;
            } else {
                current->neighbours[current->ptrCount] = NULL;
                ++current->ptrCount;
            }
        } else {
            current->ptrCount = 0;
            next = current;
            current = previous;
            if (current != NULL) {
                previous = current->neighbours[current->ptrCount];
                current->neighbours[current->ptrCount] = next;
                ++current->ptrCount;
            }
        }
    }
}

void free_nodes(Node *current, void (*const free_data)(void *)) {
    reduce_to_dfs_tree(current);

    Node *previous = NULL, *next = NULL;
    while (current != NULL) {
        if (current->ptrCount < current->neighbour_count) {
            next = current->neighbours[current->ptrCount];
            if (next != NULL) {
                current->neighbours[current->ptrCount] = previous;
                previous = current;
                current = next;
            } else {
                ++current->ptrCount;
            }
        } else {
            if (free_data != NULL) {
                free_data(current->data);
            }

            free(current);
            current = previous;
            if (current != NULL) {
                previous = current->neighbours[current->ptrCount];
                current->neighbours[current->ptrCount] = next;
                ++current->ptrCount;
            }
        }
    }
}


int main() {
    return 0;
}

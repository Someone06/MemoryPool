#include "memory.h"

#include<stdlib.h>
#include <string.h>

void *(*custom_malloc)(size_t) = malloc;
void (*custom_free)(void *) = free;

static size_t align_8(const size_t size) {
    return (size + 7) & ~7;
}

void *MALLOC(const size_t size) {
    return custom_malloc(size);
}

void *CALLOC(const size_t member_count, const size_t member_size) {
    const size_t nb = align_8(member_size) * member_count;
    void *const p = MALLOC(nb);
    if (p != NULL) {
        memset(p, 0, nb);
    }
    return p;
}

void FREE(void *const p) {
    custom_free(p);
}
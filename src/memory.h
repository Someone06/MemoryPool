#ifndef DFS_MEMORY_H
#define DFS_MEMORY_H

#include <stddef.h>

extern void *(*custom_malloc)(size_t);
extern void (*custom_free)(void *);

void* MALLOC(size_t size);
void* CALLOC(size_t member_count, size_t member_size);
void* REALLOC(void * ptr, size_t old_size, size_t new_size);
void FREE(void* p);
#endif

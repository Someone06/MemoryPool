#ifndef DFS_POINTER_BIT_HACKS_H
#define DFS_POINTER_BIT_HACKS_H

#include<stdint.h>
#include<stdbool.h>

uint16_t extract_top_bits(void const * ptr);
void* mask_top_bits(void const * ptr);
void* set_top_bits(void const * ptr, uint16_t top_bits);
void* mask_lowest_bit(void const * ptr);
bool extract_lowest_bit(void const * ptr);
void* set_lowest_bit(void const * ptr, bool lowest_bit);
void* extract_ptr_bits(void const * ptr);
#endif

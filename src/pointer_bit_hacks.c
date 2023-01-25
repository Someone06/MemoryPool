#include "pointer_bit_hacks.h"

uint16_t extract_top_bits(void const * const ptr) {
    return (uint16_t) ((uintptr_t) ptr >> 48);
}

void* mask_top_bits(void const * const ptr) {
    const uintptr_t mask = (1ULL << 48) - 1;
    return (void*) ((uintptr_t) ptr & mask);
}

void* set_top_bits(void const * const ptr, const uint16_t top_bits) {
    void const * const masked = mask_top_bits(ptr);
    return (void*) ((uintptr_t) masked | (uintptr_t) top_bits << 48);
}

void* mask_lowest_bit(void const * const ptr) {
    const uintptr_t mask = (~0ULL) - 1;
    return (void*) ((uintptr_t) ptr & mask);
}

bool extract_lowest_bit(void const * const ptr) {
    return (bool) ((uintptr_t) ptr & 1ULL);
}

void* set_lowest_bit(void const * const ptr, const bool lowest_bit) {
    const bool bit = lowest_bit == 0 ? 0 : 1;
    void const * const masked = mask_lowest_bit(ptr);
    return (void*) ((uintptr_t) masked | (uintptr_t) bit);
}

void* extract_ptr_bits(void const * const ptr) {
    return mask_lowest_bit(mask_top_bits(ptr));
}
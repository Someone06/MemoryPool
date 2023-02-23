#include <gtest/gtest.h>

#include "CMemoryPool.h"

static const size_t DEFAULT_POOL_SIZE = 1ULL << 10;

TEST(CMemoryPoolTest, allocPool) {
    MemoryPool<std::string> pool {DEFAULT_POOL_SIZE};
}

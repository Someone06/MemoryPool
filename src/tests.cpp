#include <gmock/gmock.h>

#include "CMemoryPool.h"

using ::testing::Mock;

static const size_t DEFAULT_POOL_SIZE = 1ULL << 10;

struct DetectDestructorCall final {
    MOCK_METHOD(void, die, ());
    ~DetectDestructorCall() noexcept { die(); }
};

struct Handle final {
public:
    DetectDestructorCall* get() {
        return destructorCall.get();
    }
private:
   std::unique_ptr<DetectDestructorCall> destructorCall = std::make_unique<DetectDestructorCall>();
};

TEST(CMemoryPoolTest, allocPool) {
    MemoryPool<std::string> pool {DEFAULT_POOL_SIZE};
}

TEST(CMemoryPoolTest, isCollected) {
    MemoryPool<Handle> pool {DEFAULT_POOL_SIZE};
    Handle h{};
    auto detectDestructorCall = h.get();
    EXPECT_CALL(*detectDestructorCall, die());
    pool.alloc(std::move(h), 0);
    pool.gc_mark_and_sweep();
    Mock::VerifyAndClearExpectations(detectDestructorCall);
}

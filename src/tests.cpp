#include <gmock/gmock.h>

#include "CMemoryPool.h"

using ::testing::Mock;

static const size_t DEFAULT_POOL_SIZE = 1ULL << 10;

struct DetectDestructorCall final {
    MOCK_METHOD(void, die, ());
    ~DetectDestructorCall() noexcept { die(); }
};

class Handle final {
public:
    explicit Handle(const bool shouldBeDestroyed = true) {
        EXPECT_CALL(*destructorCall.get(), die()).Times(shouldBeDestroyed);
    }

    DetectDestructorCall *get() {
        return destructorCall.get();
    }

private:
    std::unique_ptr<DetectDestructorCall> destructorCall = std::make_unique<DetectDestructorCall>();
};

class DetectDestructionPool final {
public:
    explicit DetectDestructionPool(const std::size_t poolSize = DEFAULT_POOL_SIZE) : pool{poolSize} {}

    int add_handle(const int neighbourCount = 0, const bool shouldBeDestroyed = true, const bool addToRootSet = false) {
        Handle h{shouldBeDestroyed};
        const auto detectDestructorCall = h.get();
        const auto node = pool.alloc(std::move(h), neighbourCount);
        if (addToRootSet) pool.add_root_node(node);
        handles.emplace_back(node, detectDestructorCall);
        return static_cast<int>(handles.size()) - 1;
    }

    void make_neighbours(const int from, const int to, const int index) {
        auto &f = std::get<0>(handles.at(from));
        const auto &t = std::get<0>(handles.at(to));
        f.set_neighbour(t, index);
    }

    void addToRootSet(const int node) {
        pool.add_root_node(std::get<0>(handles.at(node)));
    }

    void gc_and_verify() {
        const auto verify = [](const auto p) { Mock::VerifyAndClearExpectations(std::get<1>(p)); };
        pool.gc_mark_and_sweep();
        std::for_each(handles.begin(), handles.end(), verify);
    }

private:
    MemoryPool<Handle> pool;
    std::vector<std::pair<MemoryNode<Handle>, DetectDestructorCall *>> handles{};
};

TEST(CMemoryPoolTest, isCollected) {
    DetectDestructionPool pool{};
    pool.add_handle();
    pool.gc_and_verify();
}

TEST(CMemoryPoolTest, rootSetNotCollected) {
    DetectDestructionPool pool{};
    pool.add_handle(0, false, true);
    pool.gc_and_verify();
}
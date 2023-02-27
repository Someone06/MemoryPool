#include <gmock/gmock.h>

#include "CMemoryPool.h"

using ::testing::Bool, ::testing::Combine, ::testing::Mock, ::testing::NiceMock,
        ::testing::Range, ::testing::Test, ::testing::WithParamInterface;

static const size_t DEFAULT_POOL_SIZE = 1ULL << 10;

struct DetectDestructorCall {
    MOCK_METHOD(void, die, ());
    ~DetectDestructorCall() noexcept { die(); }
};

class Handle final {
public:
    explicit Handle(const bool shouldBeDestroyed = true) {
        // If a handle is not garbage collected, then it will be destroyed when
        // the MemoryPool is destroyed. That causes an uninteresting call to
        // die() which GMock warns about by default. By using a NiceMock, that
        // false warning is silenced.
        destructorCall = std::unique_ptr<DetectDestructorCall>(
                shouldBeDestroyed ? new DetectDestructorCall{}
                                  : new NiceMock<DetectDestructorCall>{});

        EXPECT_CALL(*destructorCall.get(), die()).Times(shouldBeDestroyed);
    }

    DetectDestructorCall *get() {
        return destructorCall.get();
    }

private:
    std::unique_ptr<DetectDestructorCall> destructorCall;
};

class DetectDestructionPool final {
public:
    explicit DetectDestructionPool(
            const std::size_t poolSize = DEFAULT_POOL_SIZE)
        : pool{poolSize} {}

    int add_handle(const int neighbourCount = 0,
                   const bool shouldBeDestroyed = true,
                   const bool addToRootSet = false) {
        const auto node = pool.alloc(neighbourCount, Handle{shouldBeDestroyed});
        const auto detectDestructorCall = node.get_data().get();
        if (addToRootSet) pool.add_root_node(node);
        handles.emplace_back(node, detectDestructorCall);
        return static_cast<int>(handles.size()) - 1;
    }

    void make_neighbours(const int from, const int to, const int index = 0) {
        auto &f = std::get<0>(handles.at(from));
        const auto &t = std::get<0>(handles.at(to));
        f.set_neighbour(t, index);
    }

    void add_root_node(const int node) {
        pool.add_root_node(std::get<0>(handles.at(node)));
    }

    void gc_and_verify() {
        const auto verify = [](const auto p) {
            Mock::VerifyAndClearExpectations(std::get<1>(p));
        };
        pool.gc_mark_and_sweep();
        std::for_each(handles.begin(), handles.end(), verify);
    }

private:
    MemoryPool<Handle> pool;
    std::vector<std::pair<MemoryNode<Handle>, DetectDestructorCall *>> handles{};
};

class DestructionTest : public Test {
protected:
    DetectDestructionPool pool{};
};

TEST_F(DestructionTest, isCollected) {
    pool.add_handle();
    pool.gc_and_verify();
}

TEST_F(DestructionTest, rootSetNotCollected) {
    pool.add_handle(0, false, true);
    pool.gc_and_verify();
}

class ParameterizedDestructionTest
    : public DestructionTest,
      public WithParamInterface<std::tuple<int, bool>> {};

TEST_P(ParameterizedDestructionTest, cicleIsCollected) {
    const auto [size, isRooted] = GetParam();
    for (int i = 0; i < size; ++i)
        pool.add_handle(1, !isRooted);

    for (int i = 0; i < size; ++i)
        pool.make_neighbours(i, (i + 1) % size);

    if(isRooted) pool.add_root_node(0);
    pool.gc_and_verify();
}

INSTANTIATE_TEST_SUITE_P(range1To5,
                         ParameterizedDestructionTest,
                         Combine(Range(1, 5), Bool()));

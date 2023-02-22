#ifndef MEMORYPOOL_CMEMORYPOOL_H
#define MEMORYPOOL_CMEMORYPOOL_H

#include <cstdint>
#include <stdexcept>

namespace MemoryPoolImplementationDetails {
    #include "memory_pool.h"
}

template<typename T> class MemoryPool;

template<typename T>
class MemoryNode final {
public:
    [[nodiscard]] std::uint16_t get_neighbour_count() const noexcept {
           return MemoryPoolImplementationDetails::memoryNode_get_neighbour_count(node);
   };

    [[nodiscard]] MemoryNode get_neighbour(std::uint16_t index) const {
           const auto count = this->get_neighbour_count();
           if(index >= count) throw std::range_error("Index out of range");
           const auto impl = MemoryPoolImplementationDetails::memoryNode_getNeighbour(node, index);
           return MemoryNode{impl};
    }

    void set_neighbour(MemoryNode const* neighbour, std::uint16_t index) {
           const auto count = this->get_neighbour_count();
           if(index >= count) throw std::range_error("Index out of range");
           MemoryPoolImplementationDetails::memoryNode_setNeighbour(node, neighbour->node, index);
    }

    [[nodiscard]] T* get_data() const noexcept {
          return static_cast<T*>(MemoryPoolImplementationDetails::memoryNode_get_data(node));
    }

private:
    friend class MemoryPool<T>;

    explicit MemoryNode(MemoryPoolImplementationDetails::MemoryNode* memoryNode) : node {memoryNode} {}

    MemoryNode(MemoryPoolImplementationDetails::MemoryNode* memoryNode, T&& initial_value) :node{memoryNode} {
         auto data = MemoryPoolImplementationDetails::memoryNode_get_data(node);
         new(data) T(initial_value);
    }

    [[nodiscard]] MemoryPoolImplementationDetails::MemoryNode * get_node() const noexcept {
         return node;
    }

    MemoryPoolImplementationDetails::MemoryNode* node;

};

template<typename T>
class MemoryPool final {
public:
    explicit MemoryPool(const std::size_t size) {
         auto freeFn = [](void *e) { std::destroy_at<T>(static_cast<T*>(e)); };
         pool = MemoryPoolImplementationDetails::memory_pool_new(size, freeFn);
         if(pool.head == nullptr)
             throw std::bad_alloc();
    }

   MemoryPool(const MemoryPool<T>&) = delete;
   MemoryPool& operator=(const MemoryPool<T>&) = delete;
   MemoryPool(MemoryPool<T>&&)  noexcept = default;
   MemoryPool& operator=(MemoryPool<T>&&)  noexcept = default;

   ~MemoryPool() noexcept {
           MemoryPoolImplementationDetails::memoryPool_free(&pool);
   };

   MemoryNode<T> alloc(T&& value, const std::size_t neighbours) {
       auto node = MemoryPoolImplementationDetails::memoryPool_alloc(&pool, sizeof(T), neighbours);
       if(node == nullptr)
           throw std::bad_alloc();

       return MemoryNode {node, std::forward<T>(value)};
   }

   void add_root_node(const MemoryNode<T>& node) {
      const auto success = MemoryPoolImplementationDetails::memoryPool_add_root_node(&pool, node.get_node());
      if(!success) throw std::runtime_error("Failed to add MemoryNode to root set.");
   }

   void gc_mark_and_sweep() noexcept {
      MemoryPoolImplementationDetails::memoryPool_gc_mark_and_sweep(&pool);
   }

private:
    MemoryPoolImplementationDetails::MemoryPool pool;
};

#endif

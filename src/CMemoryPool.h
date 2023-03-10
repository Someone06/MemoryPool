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
           return MemoryPoolImplementationDetails::memoryNode_get_neighbour_count(&node);
   };

    [[nodiscard]] MemoryNode get_neighbour(std::uint16_t index) const {
           const auto count = this->get_neighbour_count();
           if(index >= count)
               throw std::range_error("Index out of range");

           const auto impl = MemoryPoolImplementationDetails::memoryNode_getNeighbour(&node, index);
           return MemoryNode{impl};
    }

    void set_neighbour(const MemoryNode& neighbour, std::uint16_t index) {
           const auto count = this->get_neighbour_count();
           if(index >= count) throw std::range_error("Index out of range");
           MemoryPoolImplementationDetails::memoryNode_setNeighbour(&node, &neighbour.node, index);
    }

    [[nodiscard]] T& get_data() const noexcept {
          return *static_cast<T*>(MemoryPoolImplementationDetails::memoryNode_get_data(&node));
    }

private:
    friend class MemoryPool<T>;

    explicit MemoryNode(MemoryPoolImplementationDetails::MemoryNode& memoryNode) : node {memoryNode} {}

    MemoryNode(MemoryPoolImplementationDetails::MemoryNode& memoryNode, T&& initial_value) :node{memoryNode} {
         const auto location = getObjectLocation(memoryNode);
         new(location) T(std::forward<T>(initial_value));
    }

    template<typename... Args>
    explicit MemoryNode(MemoryPoolImplementationDetails::MemoryNode& memoryNode, Args&&... args) :node{memoryNode} {
         const auto location = getObjectLocation(memoryNode);
         new(location) T{std::forward<Args>(args)...};
    }

    [[nodiscard]] MemoryPoolImplementationDetails::MemoryNode&  get_node() const noexcept {
         return node;
    }

    template<typename U>
    static size_t offset(const char * const buffer) noexcept {
        const auto alignment = alignof(U);
        const auto numeric = reinterpret_cast<std::uintptr_t>(buffer);
        const auto modulo = numeric % alignment;
        return modulo == 0 ? 0 : alignment - modulo;
    }

    static T* getObjectLocation(MemoryPoolImplementationDetails::MemoryNode& node) {
        const auto data = MemoryPoolImplementationDetails::memoryNode_get_data(&node);
        const auto chars = static_cast<char*>(data);
        const auto location = chars + offset<T>(chars);
        return reinterpret_cast<T*>(location);
    }

    MemoryPoolImplementationDetails::MemoryNode& node;
};

template<typename T>
class MemoryPool final {
public:
    explicit MemoryPool(const std::size_t size) {
         const auto freeFn = [](void *e) { std::destroy_at<T>(static_cast<T*>(e)); };
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

   MemoryNode<T> alloc(const std::size_t neighbours, T&& value) {
       auto& node = allocNode(neighbours);
       return MemoryNode<T> {node, std::forward<T>(value)};
   }

   template<typename... Args>
   MemoryNode<T> alloc_emplace(const std::size_t neighbours, Args&&... args) {
       auto& node = allocNode(neighbours);
       return MemoryNode<T> {node, std::forward<Args>(args)...};
   }

   void add_root_node(const MemoryNode<T>& node) {
      const auto success = MemoryPoolImplementationDetails::memoryPool_add_root_node(&pool, &node.get_node());
      if(!success)
           throw std::runtime_error("Failed to add MemoryNode to root set.");
   }

   void gc_mark_and_sweep() noexcept {
      MemoryPoolImplementationDetails::memoryPool_gc_mark_and_sweep(&pool);
   }

private:
    MemoryPoolImplementationDetails::MemoryNode& allocNode(const std::size_t neighbours) {
      const auto node = MemoryPoolImplementationDetails::memoryPool_alloc(&pool, sizeof(T) + alignof(T), neighbours);
       if(node == nullptr)
           throw std::bad_alloc();

       return *node;
    }

    MemoryPoolImplementationDetails::MemoryPool pool;
};

#endif

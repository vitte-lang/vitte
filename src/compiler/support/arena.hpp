#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace vitte::support {

template <typename Base, typename Id = std::uint32_t>
class Arena {
public:
    explicit Arena(std::size_t chunk_size = 4096)
        : chunk_size_(chunk_size) {
        add_chunk(chunk_size_);
    }

    ~Arena() = default;

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    Arena(Arena&&) = delete;
    Arena& operator=(Arena&&) = delete;

    template <typename T, typename... Args>
    Id emplace(Args&&... args) {
        static_assert(std::is_base_of_v<Base, T>);
        void* mem = allocate(sizeof(T), alignof(T));
        T* obj = new (mem) T(std::forward<Args>(args)...);
        nodes_.push_back(obj);
        return static_cast<Id>(nodes_.size() - 1);
    }

    Base* get(Id id) {
        if (static_cast<std::size_t>(id) >= nodes_.size()) {
            return nullptr;
        }
        return nodes_[static_cast<std::size_t>(id)];
    }

    const Base* get(Id id) const {
        if (static_cast<std::size_t>(id) >= nodes_.size()) {
            return nullptr;
        }
        return nodes_[static_cast<std::size_t>(id)];
    }

    std::size_t size() const {
        return nodes_.size();
    }

private:
    struct Chunk {
        std::unique_ptr<std::byte[]> data;
        std::size_t size;
        std::size_t offset;
    };

    std::vector<Chunk> chunks_;
    std::vector<Base*> nodes_;
    std::size_t chunk_size_;

    void add_chunk(std::size_t min_size) {
        std::size_t size = std::max(chunk_size_, min_size);
        chunks_.push_back({std::make_unique<std::byte[]>(size), size, 0});
    }

    void* allocate(std::size_t size, std::size_t align) {
        Chunk* chunk = &chunks_.back();
        std::size_t offset = chunk->offset;
        std::size_t aligned = (offset + (align - 1)) & ~(align - 1);
        if (aligned + size > chunk->size) {
            add_chunk(size + align);
            chunk = &chunks_.back();
            offset = chunk->offset;
            aligned = (offset + (align - 1)) & ~(align - 1);
        }
        void* ptr = chunk->data.get() + aligned;
        chunk->offset = aligned + size;
        return ptr;
    }
};

} // namespace vitte::support

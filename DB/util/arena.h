#ifndef STORAGE_LEVELDB_UTIL_ARENA_H_
#define STORAGE_LEVELDB_UTIL_ARENA_H_

#include <cstddef>
#include <atomic>
#include <vector>
#include <cstdint>
#include <cassert>
namespace Rabbitdb{

class Arena{
    public:
        Arena();

        Arena(const Arena&) = delete;
        Arena& operator=(const Arena&) = delete;

        ~Arena();

        // Return a pointer to a newly allocated memory block of "bytes" bytes.
        char* Allocate(size_t bytes);

        // Allocate memory with the normal alignment guarantees provided by malloc.
        char* AllocateAligned(size_t bytes);

        // Returns an estimate of the total memory usage of data allocated
        // by the arena.
        size_t MemoryUsage() const {
            return memory_usage_.load(std::memory_order_relaxed);
        }

    private:
        char* AllocateFallback(size_t bytes);
        char* AllocateNewBlock(size_t block_bytes);

        // Allocation state
        char* alloc_ptr_;
        size_t alloc_bytes_remaining_;

        // Array of new[] allocated memory blocks
        std::vector<char*> blocks_;
        
        // Total memory usage of the arena.
        //
        // TODO(costan): This member is accessed via atomics, but the others are
        //               accessed without any locking. Is this OK?
        // 有趣 我也想知道 alloc_ptr_ && remaining 写多 应该保证原子性
        std::atomic<size_t> memory_usage_;
};

inline char* Arena::Allocate(size_t bytes)
{
    // The semantics of what to return are a bit messy if we allow
    // 0-byte allocations, so we disallow them here ( we don't need
    // them for our internal use).
    assert(bytes > 0);
    if(bytes <= alloc_bytes_remaining_)
    {
        char* result = alloc_ptr_;
        alloc_ptr_ += bytes;
        alloc_bytes_remaining_ -= bytes;
        return result;
    }
    return AllocateFallback(bytes);
}
}
#endif
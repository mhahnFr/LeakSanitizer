/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2024  mhahnFr
 *
 * This file is part of the LeakSanitizer.
 *
 * The LeakSanitizer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The LeakSanitizer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with the
 * LeakSanitizer, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cstdlib>
#include <vector>

#include "ObjectPool.hpp"

namespace lsan {
struct MemoryChunk {
    MemoryChunk* next = nullptr;
};

struct MemoryBlock {
    MemoryBlock* next = nullptr;
    std::size_t allocCount = 0;
};

struct ObjectPool {
    std::size_t objectSize;
    std::size_t blockSize;
    ObjectPool* next = nullptr;

    MemoryChunk* chunks = nullptr;

    MemoryBlock* blocks = nullptr;

    constexpr inline ObjectPool(std::size_t objectSize, std::size_t blockSize): objectSize(objectSize), blockSize(blockSize) {}

    inline auto allocate() -> void* {
        if (chunks != nullptr) {
            auto toReturn = chunks;
            chunks = chunks->next;
            return static_cast<void*>(toReturn);
        }
        auto buffer = std::malloc(objectSize * blockSize + sizeof(MemoryBlock)); // TODO: Align
        if (buffer == nullptr) {
            return nullptr;
        }
        auto newBlock = new(buffer) MemoryBlock;
        newBlock->next = blocks;
        blocks = newBlock;

        for (std::size_t i = 0; i < blockSize; ++i) {
            auto newChunk = new(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(buffer) + sizeof(MemoryBlock) + i * objectSize)) MemoryChunk;
            newChunk->next = chunks;
            chunks = newChunk;
        }
        return allocate();
    }

    inline void deallocate(void* pointer) {
        auto chunk = static_cast<MemoryChunk*>(pointer);
        chunk->next = chunks;
        chunks = chunk;
        // TODO: Release the corresponding block if empty
    }
};

static auto pools() -> std::vector<ObjectPool>& {
    static std::vector<ObjectPool>* pools = new std::vector<ObjectPool>(); // TODO: How to get rid of this memory leak?
    return *pools;
}

static inline auto findPool(std::size_t size, bool create = true) -> ObjectPool& {
    for (auto& pool : pools()) {
        if (pool.objectSize == size) {
            return pool;
        }
    }
    if (create) {
        return *pools().insert(pools().end(), ObjectPool(size, 100));
    }
    abort(); // TODO: Fail more gracefully
}

auto allocate(std::size_t size) -> void* {
    return findPool(size).allocate();
}

void deallocate(void* pointer, std::size_t size) {
    findPool(size, false).deallocate(pointer);
}
}

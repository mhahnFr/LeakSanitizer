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
struct MemoryBlock {
    std::size_t blockSize = 0;
    std::size_t allocCount = 0;

    constexpr inline MemoryBlock(std::size_t blockSize): blockSize(blockSize) {}

    inline ~MemoryBlock() {
        __builtin_printf("Deleted block: %p Possible: %zu\n", this, blockSize);
    }
};

struct MemoryChunk {
    MemoryBlock* block = nullptr;
    MemoryChunk* next = nullptr;
    MemoryChunk* previous = nullptr;
};

struct ObjectPool {
    std::size_t objectSize;
    std::size_t blockSize;
    MemoryChunk* chunks = nullptr;
    std::size_t factor = 1;

    constexpr inline ObjectPool(std::size_t objectSize, std::size_t blockSize): objectSize(objectSize), blockSize(blockSize) {}

    inline auto allocate() -> void* {
        if (chunks != nullptr) {
            auto toReturn = chunks;
            chunks = chunks->next;
            if (chunks != nullptr) {
                chunks->previous = nullptr;
            }
            ++toReturn->block->allocCount;
            return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(toReturn) + sizeof(MemoryBlock*));
        }
        auto buffer = std::malloc((objectSize + sizeof(MemoryBlock*)) * (blockSize * factor) + sizeof(MemoryBlock)); // TODO: Align
        if (buffer == nullptr) {
            return nullptr;
        }
        auto newBlock = new(buffer) MemoryBlock(blockSize * factor);

        for (std::size_t i = 0; i < newBlock->blockSize; ++i) {
            auto newChunk = new(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(buffer) + sizeof(MemoryBlock) + i * (objectSize + sizeof(MemoryBlock*)))) MemoryChunk;
            newChunk->next = chunks;
            if (chunks != nullptr) {
                chunks->previous = newChunk;
            }
            newChunk->block = newBlock;
            chunks = newChunk;
        }
        ++factor;
        return allocate();
    }

    inline void deallocate(void* pointer) {
        auto chunk = reinterpret_cast<MemoryChunk*>(reinterpret_cast<uintptr_t>(pointer) - sizeof(MemoryBlock*));
        chunk->next = chunks;
        if (chunks != nullptr) {
            chunks->previous = chunk;
        }
        chunk->previous = nullptr;
        chunks = chunk;
        if (--chunk->block->allocCount == 0) {
            const auto& block = chunk->block;
            for (std::size_t i = 0; i < block->blockSize; ++i) {
                const auto& element = reinterpret_cast<MemoryChunk*>(reinterpret_cast<uintptr_t>(block) + sizeof(MemoryBlock) + i * (objectSize + sizeof(MemoryBlock*)));
                if (element->previous != nullptr) {
                    element->previous->next = element->next;
                }
                if (element->next != nullptr) {
                    element->next->previous = element->previous;
                }
                if (element == chunks) {
                    chunks = element->next;
                }
                element->~MemoryChunk();
            }
            delete block;
            if (factor > 1) {
                --factor;
            }
        }
    }
};

static auto getPools() -> std::vector<ObjectPool>& {
    static std::vector<ObjectPool>* pools = new std::vector<ObjectPool>(); // TODO: How to get rid of this memory leak?
    return *pools;
}

static inline auto findPool(std::size_t size, bool create = true) -> ObjectPool& {
    auto& pools = getPools();
    const auto& it = std::find_if(pools.begin(), pools.end(), [&size](auto element) {
        return element.objectSize == size;
    });
    if (it != pools.end()) {
        return *it;
    } else if (create) {
        return *pools.insert(pools.end(), ObjectPool(size, 100));
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

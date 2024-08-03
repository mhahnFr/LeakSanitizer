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

#ifndef ObjectPool_hpp
#define ObjectPool_hpp

#include <cstddef>

namespace lsan {
class ObjectPool {
    struct MemoryBlock {
        std::size_t blockSize = 0;
        std::size_t allocCount = 0;

        constexpr inline MemoryBlock(std::size_t blockSize): blockSize(blockSize) {}
    };

    struct MemoryChunk {
        MemoryBlock* block = nullptr;
        MemoryChunk* next = nullptr;
        MemoryChunk* previous = nullptr;
    };

    std::size_t objectSize;
    std::size_t blockSize;
    std::size_t factor = 1;
    MemoryChunk* chunks = nullptr;

public:
    constexpr inline ObjectPool(std::size_t objectSize, std::size_t blockSize): objectSize(objectSize), blockSize(blockSize) {}

    auto allocate() -> void*;
    void deallocate(void* pointer);

    constexpr inline auto getObjectSize() const -> std::size_t {
        return objectSize;
    }
};
}

#endif /* ObjectPool_hpp */

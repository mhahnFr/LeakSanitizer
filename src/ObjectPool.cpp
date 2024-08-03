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

#include <cstdint>
#include <cstdlib>
#include <new>

#include "ObjectPool.hpp"

namespace lsan {
auto ObjectPool::allocate() -> void* {
    if (chunks != nullptr) {
        auto toReturn = chunks;
        chunks = chunks->next;
        if (chunks != nullptr) {
            chunks->previous = nullptr;
        }
        ++toReturn->block->allocCount;
        return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(toReturn) + sizeof(MemoryBlock*));
    }
    auto buffer = std::malloc((objectSize + sizeof(MemoryBlock*)) * (blockSize * factor) + sizeof(MemoryBlock));
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

void ObjectPool::deallocate(void* pointer) {
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

void ObjectPool::merge(ObjectPool& other) {
    // TODO: Properly implement
    abort();
}
}

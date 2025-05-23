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

namespace lsan {
/**
 * This class represents a pool of objects.
 */
class ObjectPool {
    /**
     * This structure represents an allocated memory block.
     */
    struct MemoryBlock {
        /** The amount of objects this block can hold.     */
        std::size_t blockSize = 0;
        /** The amount of allocated objects in this block. */
        std::size_t allocCount = 0;

        /**
         * Constructs a memory block with the given values.
         *
         * @param blockSize the amount of objects this block can hold
         */
        explicit constexpr MemoryBlock(const std::size_t blockSize): blockSize(blockSize) {}
    };

    /**
     * @brief This structure represents a chunk of memory.
     *
     * The block pointer is intended to be used as overhead, the next and
     * previous pointers are only needed when this chunk is deallocated.
     */
    struct MemoryChunk {
        /** The associated memory block. */
        MemoryBlock* block = nullptr;
        /** The next memory chunk.       */
        MemoryChunk* next = nullptr;
        /** The previous memory chunk.   */
        MemoryChunk* previous = nullptr;
    };

    /** The size in bytes of one object.                    */
    std::size_t objectSize;
    /** The amount of objects one memory block should hold. */
    std::size_t blockSize;
    /** The factor to multiply the block size with.         */
    std::size_t factor = 1;
    /** The list of available memory chunks.                */
    MemoryChunk* chunks = nullptr;

public:
    /**
     * @brief Constructs an object pool.
     *
     * The user of this class is responsible to make sure the object size is at
     * least the same as the size of two pointers.
     *
     * @param objectSize the size of one object in bytes
     * @param blockSize the amount of objects a block of memory should hold
     */
    constexpr ObjectPool(const std::size_t objectSize, const std::size_t blockSize): objectSize(objectSize), blockSize(blockSize) {}

    /**
     * Allocates an object in the pool.
     *
     * @return the object or `NULL` if unable to allocate
     */
    auto allocate() -> void*;
    /**
     * Deallocates the given object.
     *
     * @param pointer the object to be deallocated
     */
    void deallocate(void* pointer);

    /**
     * @brief Merges this object pool with the given one.
     *
     * The user is responsible to make sure the object size of this pool and
     * the object size of the other object pool are the same.
     *
     * @param other the other object pool to merge with
     */
    void merge(ObjectPool& other);

    /**
     * Returns the size in bytes of one object in this pool.
     *
     * @return the size in bytes of one object
     */
    constexpr auto getObjectSize() const -> std::size_t {
        return objectSize;
    }

    constexpr auto operator==(const ObjectPool& other) const noexcept -> bool {
        return objectSize == other.objectSize
            && blockSize == other.blockSize
            && factor == other.factor
            && chunks == other.chunks;
    }

    constexpr auto operator!=(const ObjectPool& other) const noexcept -> bool {
        return !(*this == other);
    }
};
}

#endif /* ObjectPool_hpp */

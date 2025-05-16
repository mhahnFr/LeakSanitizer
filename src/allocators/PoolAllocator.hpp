/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2024 - 2025  mhahnFr
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

#ifndef PoolAllocator_hpp
#define PoolAllocator_hpp

#include <algorithm>
#include <limits>
#include <memory>
#include <vector>

#include "ObjectPool.hpp"
#include "RealAllocator.hpp"

namespace lsan {
/**
 * This class is an allocator using the object pool class.
 *
 * @tparam T the type of object to be allocated with this allocator
 */
template<typename T>
struct PoolAllocator {
    static_assert(sizeof(T) >= 2 * sizeof(void*), "The PoolAllocator needs to store two pointers in deallocated memory blocks.");

    /** The value type of this allocator.                                                */
    using value_type = T;
    /** Indicates allocators of this type are not always equal.                          */
    using is_always_equal = std::false_type;
    /** Indicates allocators of this type should propagate on container move assignment. */
    using propagate_on_container_move_assignment = std::true_type;
    /** Indicates allocators of this type should propagate on container copy assignment. */
    using propagate_on_container_copy_assignment = std::true_type;
    /** The type used to store object pools.                                             */
    using Pools = std::vector<ObjectPool>;

    PoolAllocator(): pools(std::allocate_shared<Pools>(RealAllocator<Pools>())) {}

    template<typename U>
    explicit constexpr PoolAllocator(const PoolAllocator<U>& other) noexcept: pools(other.getPools()) {}

    constexpr PoolAllocator(PoolAllocator&& other) noexcept: pools(other.pools) {}

    template<typename U>
    explicit constexpr PoolAllocator(PoolAllocator<U>&& other) noexcept: pools(other.getPools()) {}

    constexpr auto operator=(const PoolAllocator& other) noexcept -> PoolAllocator& {
        pools = other.pools;
        return *this;
    }

    constexpr auto operator=(PoolAllocator&& other) noexcept -> PoolAllocator& {
        pools = other.pools;
        return *this;
    }

    /**
     * @brief Allocates the given amount of objects.
     *
     * If more than one object is requested, the object pool is not used.
     *
     * @param count the amount of objects to allocate
     * @return the allocated block of memory
     * @throws std::bad_alloc if too many objects are requested or if unable to allocate
     */
    [[ nodiscard ]] constexpr auto allocate(const std::size_t count) -> T* {
        if (count > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            throw std::bad_array_new_length();
        }

        if (count > 1) {
            auto toReturn = std::malloc(count * sizeof(T));
            if (toReturn == nullptr) {
                throw std::bad_alloc();
            }
            return static_cast<T*>(toReturn);
        }
        auto toReturn = static_cast<T*>(findPool().allocate());
        if (toReturn == nullptr) {
            throw std::bad_alloc();
        }
        return toReturn;
    }

    /**
     * Deallocates the given block of memory.
     *
     * @param pointer the block of memory to be deallocated
     * @param count the amount of objects to be deallocated
     */
    constexpr void deallocate(T* pointer, const std::size_t count) noexcept {
        if (count > 1) {
            std::free(pointer);
        } else {
            findPool(false).deallocate(pointer);
        }
    }

    template<typename U>
    constexpr auto operator==(const PoolAllocator<U>& other) const noexcept -> bool {
        return pools == other.getPools() || *pools == *other.pools;
    }

    template<typename U>
    constexpr auto operator!=(const PoolAllocator<U>& other) const noexcept -> bool {
        return !(*this == other);
    }

    /**
     * Returns the registered object pools.
     *
     * @return the registered object pools
     */
    auto getPools() const -> std::shared_ptr<Pools> {
        return pools;
    }

    /**
     * @brief Merges this allocator with the given other allocator.
     *
     * After this operation, both allocators will compare equal, but the state is not shared.
     *
     * @param other the other allocator to merge with
     */
    void merge(PoolAllocator&& other) {
        for (auto& pool : *other.getPools()) {
            const auto& it = std::find_if(pools->begin(), pools->end(), [&pool](const auto& element) {
                return element.getObjectSize() == pool.getObjectSize();
            });
            if (it == pools->end()) {
                pools->push_back(pool);
            } else {
                it->merge(pool);
            }
        }
        *other.pools = *pools;
    }

private:
    /** The shared object pools. */
    std::shared_ptr<Pools> pools;

    /**
     * Attempts to find an appropriate object pool for the type of object managed
     * by this allocator.
     *
     * @param create whether to create a new object pool if no appropriate one has been found
     * @return an appropriate object pool
     * @throws std::runtime_error if no appropriate object pool was found and no pool should be created
     */
    constexpr auto findPool(const bool create = true) -> ObjectPool& {
        constexpr std::size_t size = sizeof(T);

        const auto& it = std::find_if(pools->begin(), pools->end(), [&size](const auto& element) {
            return element.getObjectSize() == size;
        });
        if (it != pools->end()) {
            return *it;
        } else if (create) {
            return *pools->insert(pools->end(), ObjectPool(size, 500));
        }
        throw std::runtime_error("Object pool not found! Size = " + std::to_string(size) + ", create = false");
    }
};
}

#endif /* PoolAllocator_hpp */

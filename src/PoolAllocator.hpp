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

#ifndef PoolAllocator_hpp
#define PoolAllocator_hpp

#include <algorithm>
#include <limits>
#include <memory>
#include <vector>

#include "ObjectPool.hpp"
#include "RealAllocator.hpp"

namespace lsan {
template<typename T>
struct PoolAllocator {
    using value_type = T;
    using is_always_equal = std::false_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_copy_assignment = std::true_type;
    using Pools = std::vector<ObjectPool>;

    inline PoolAllocator(): pools(std::allocate_shared<Pools>(RealAllocator<Pools>())) {}

    template<typename U>
    constexpr inline PoolAllocator(const PoolAllocator<U>& other) noexcept: pools(other.getPools()) {}

    constexpr inline PoolAllocator(PoolAllocator&& other) noexcept: pools(other.pools) {}

    template<typename U>
    constexpr inline PoolAllocator(PoolAllocator<U>&& other) noexcept: pools(other.getPools()) {}

    constexpr inline auto operator=(const PoolAllocator& other) noexcept -> PoolAllocator& {
        pools = other.pools;
        return *this;
    }

    constexpr inline auto operator=(PoolAllocator&& other) noexcept -> PoolAllocator& {
        pools = other.pools;
        return *this;
    }

    [[ nodiscard ]] constexpr inline auto allocate(std::size_t count) -> T* {
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

    constexpr inline void deallocate(T* pointer, std::size_t count) noexcept {
        if (count > 1) {
            std::free(pointer);
        } else {
            findPool(false).deallocate(pointer);
        }
    }

    template<typename U>
    constexpr inline auto operator==(const PoolAllocator<U>& other) const noexcept -> bool {
        return pools == other.getPools() || *pools == *other.pools;
    }

    template<typename U>
    constexpr inline auto operator!=(const PoolAllocator<U>& other) const noexcept -> bool {
        return !(*this == other);
    }

    inline auto getPools() const -> std::shared_ptr<Pools> {
        return pools;
    }

    inline void merge(PoolAllocator&& other) {
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
    std::shared_ptr<Pools> pools;

    constexpr inline auto findPool(bool create = true) -> ObjectPool& {
        constexpr const std::size_t size = sizeof(T);

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

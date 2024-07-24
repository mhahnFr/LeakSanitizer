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

#include <limits>

namespace lsan {
template<typename T>
struct PoolAllocator {
    using value_type = T;

    PoolAllocator() = default;

    template<typename U>
    constexpr inline PoolAllocator(const PoolAllocator<U>& other) noexcept {}

    [[ nodiscard ]] auto allocate(std::size_t count) -> T* {
        if (count > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            throw std::bad_array_new_length();
        }

        // TODO: Call the mempool
        auto toReturn = std::malloc(count * sizeof(T));
        if (toReturn == nullptr) {
            throw std::bad_alloc();
        }
        return static_cast<T*>(toReturn);
    }

    void deallocate(T* pointer, std::size_t count) noexcept {
        // TODO: Use the mempool
        std::free(pointer);
    }
};

template<typename T, typename U>
constexpr inline auto operator==(const PoolAllocator<T>&, const PoolAllocator<U>&) noexcept -> bool {
    return true;
}

template<typename T, typename U>
constexpr inline auto operator!=(const PoolAllocator<T>&, const PoolAllocator<U>&) noexcept -> bool {
    return false;
}
}

#endif /* PoolAllocator_hpp */

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

#ifndef RealAllocator_hpp
#define RealAllocator_hpp

#include <limits>

#include "../allocations/realAlloc.hpp"

namespace lsan {
/**
 * This allocator uses the allocation functions of the namespace `real`.
 *
 * @tparam T the type to be allocated by this allocator
 */
template<typename T>
struct RealAllocator
{
    /** The value type of this allocator.                   */
    using value_type = T;
    /** Indicates allocators of this type are always equal. */
    using is_always_equal = std::true_type;

    RealAllocator() = default;

    template<typename U>
    constexpr inline RealAllocator(const RealAllocator<U>&) noexcept {}

    /**
     * Allocates and returns a block of memory fitting for the given amount of objects.
     *
     * @param n the amount of objects to be allocated
     * @return the allocated block
     * @throws if too many objects are requested or when the allocator failed to allocate
     */
    [[ nodiscard ]] constexpr inline auto allocate(std::size_t n) -> T* {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            throw std::bad_array_new_length();
        }

        auto toReturn = real::malloc(n * sizeof(T));
        if (toReturn == nullptr) {
            throw std::bad_alloc();
        }
        return static_cast<T*>(toReturn);
    }

    /**
     * @brief Deallocates the given pointer.
     *
     * The given object amount is ignored.
     *
     * @param p the pointer to be deallocated
     */
    constexpr inline void deallocate(T* p, std::size_t) noexcept {
        real::free(p);
    }

    template<typename U>
    constexpr inline auto operator==(const RealAllocator<U>&) const noexcept -> bool {
        return true;
    }

    template<typename U>
    constexpr inline auto operator!=(const RealAllocator<U>&) const noexcept -> bool {
        return true;
    }
};
}

#endif /* RealAllocator_hpp */

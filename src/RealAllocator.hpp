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

#include "allocations/realAlloc.hpp"

namespace lsan {
template<typename T>
struct RealAllocator
{
    using value_type = T;
    using is_always_equal = std::true_type;

    RealAllocator() = default;

    template<typename U>
    constexpr inline RealAllocator(const RealAllocator<U>&) noexcept {}

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

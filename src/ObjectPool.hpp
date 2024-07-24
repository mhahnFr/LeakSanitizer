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
auto allocate(std::size_t size) -> void*;
void deallocate(void* pointer, std::size_t size);

template<typename T>
constexpr inline auto allocate() -> T* {
    return static_cast<T*>(allocate(sizeof(T)));
}

template<typename T>
constexpr inline void deallocate(T* pointer) {
    deallocate(pointer, sizeof(T));
}
}

#endif /* ObjectPool_hpp */

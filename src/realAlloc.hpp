/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023  mhahnFr
 *
 * This file is part of the LeakSanitizer. This library is free software:
 * you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef realAlloc_hpp
#define realAlloc_hpp

#ifdef __linux__
extern "C" {
void * __libc_malloc(std::size_t);
void * __libc_calloc(std::size_t, std::size_t);
void * __libc_realloc(void *, std::size_t);
void   __libc_free(void *);
}
#endif

namespace lsan::real {
static inline auto malloc(std::size_t size) -> void * {
    void * toReturn;
#ifdef __linux__
    toReturn = __libc_malloc(size);
#else
    toReturn = std::malloc(size);
#endif
    return toReturn;
}

static inline auto calloc(std::size_t count, std::size_t size) -> void * {
    void * toReturn;
#ifdef __linux__
    toReturn = __libc_calloc(count, size);
#else
    toReturn = std::calloc(count, size);
#endif
    return toReturn;
}

static inline auto realloc(void * pointer, std::size_t size) -> void * {
    void * toReturn;
#ifdef __linux__
    toReturn = __libc_realloc(pointer, size);
#else
    toReturn = std::realloc(pointer, size);
#endif
    return toReturn;
}

static inline void free(void * pointer) {
#ifdef __linux__
    __libc_free(pointer);
#else
    std::free(pointer);
#endif
}
}

#endif /* realAlloc_hpp */

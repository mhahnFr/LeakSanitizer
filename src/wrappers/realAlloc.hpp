/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023 - 2025  mhahnFr
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

#ifndef realAlloc_hpp
#define realAlloc_hpp

#ifdef __linux__
extern "C" {
void* __libc_malloc(std::size_t);
void* __libc_valloc(std::size_t);
void* __libc_calloc(std::size_t, std::size_t);
void* __libc_realloc(void *, std::size_t);
void  __libc_free(void *);
void* __libc_memalign(std::size_t, std::size_t);
}
#endif

/**
 * This namespace contains wrapper functions calling the corresponding real function.
 */
namespace lsan::real {
/**
 * Calls the real @c malloc function.
 *
 * @param size the requested allocation size
 * @return the allocated block of memory or @c NULL if no memory was available
 */
static inline auto malloc(std::size_t size) -> void * {
    void * toReturn;
#ifdef __linux__
    toReturn = __libc_malloc(size);
#else
    toReturn = std::malloc(size);
#endif
    return toReturn;
}

/**
 * Calls the real @c valloc function.
 *
 * @param size the requested allocation size
 * @return the allocated block of memory or @c NULL if no memory was available
 */
static inline auto valloc(std::size_t size) -> void* {
    void* toReturn;
#ifdef __linux__
    toReturn = __libc_valloc(size);
#else
    toReturn = ::valloc(size);
#endif
    return toReturn;
}

/**
 * Calls the real @c calloc function.
 *
 * @param count the count of objects
 * @param size the size an individual object
 * @return the allocated block of memory or @c NULL if no memory was available
 */
static inline auto calloc(std::size_t count, std::size_t size) -> void * {
    void * toReturn;
#ifdef __linux__
    toReturn = __libc_calloc(count, size);
#else
    toReturn = std::calloc(count, size);
#endif
    return toReturn;
}

/**
 * Calls the real @c aligned_alloc function.
 *
 * @param alignment the allocation alignment
 * @param size the amount of bytes
 * @return the allocated block of memory
 */
static inline auto aligned_alloc(std::size_t alignment, std::size_t size) -> void* {
    void* toReturn;
#ifdef __linux__
    toReturn = __libc_memalign(alignment, size);
#else
    toReturn = std::aligned_alloc(alignment, size);
#endif
    return toReturn;
}

/**
 * Calls the real @c realloc function.
 *
 * @param pointer the pointer to the memory block to be reallocated
 * @param size the requested new size the memory block
 * @return the reallocated memory block
 */
static inline auto realloc(void * pointer, std::size_t size) -> void * {
    void * toReturn;
#ifdef __linux__
    toReturn = __libc_realloc(pointer, size);
#else
    toReturn = std::realloc(pointer, size);
#endif
    return toReturn;
}

/**
 * Calls the real @c free function.
 *
 * @param pointer the pointer to be freed
 */
static inline void free(void * pointer) {
#ifdef __linux__
    __libc_free(pointer);
#else
    std::free(pointer);
#endif
}
}

#endif /* realAlloc_hpp */

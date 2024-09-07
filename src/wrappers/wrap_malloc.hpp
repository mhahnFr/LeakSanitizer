/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2024  mhahnFr
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

#ifndef wrap_malloc_hpp
#define wrap_malloc_hpp

#include <cstddef>

namespace lsan {
extern "C" {
void * __wrap_malloc(size_t, const char *, int);
void * __wrap_calloc(size_t, size_t, const char *, int);
void * __wrap_realloc(void *, size_t, const char *, int);
void   __wrap_free(void *, const char *, int);

[[ noreturn ]] void __wrap_exit(int, const char *, int);
}

#ifdef __linux__
extern "C" {
#endif /* __linux__ */

auto malloc(std::size_t)              -> void*;
auto calloc(std::size_t, std::size_t) -> void*;
auto realloc(void*, std::size_t)      -> void*;
void free(void*);

#ifdef __linux__
} /* extern "C" */
#endif /* __linux__ */
}

#endif /* wrap_malloc_hpp */

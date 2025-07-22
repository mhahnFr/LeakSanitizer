// LeakSanitizer - Small library showing information about lost memory.
//
// Copyright (C) 2025  mhahnFr
//
// This file is part of the LeakSanitizer.
//
// The LeakSanitizer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The LeakSanitizer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with the
// LeakSanitizer, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.

#ifndef WRAP_MALLOC_HPP
#define WRAP_MALLOC_HPP

#include <cstddef>

#ifdef __APPLE__
# include <malloc/malloc.h>
#endif

namespace lsan {
extern "C" {
auto __wrap_malloc(std::size_t, const char*, int) -> void*;
auto __wrap_calloc(std::size_t, std::size_t, const char*, int) -> void*;
auto __wrap_realloc(void*, std::size_t, const char*, int) -> void*;
void __wrap_free(void*, const char*, int);
[[ noreturn ]] void __wrap_exit(int, const char*, int);
}

#ifdef __APPLE__
auto malloc_zone_malloc(malloc_zone_t*, std::size_t) -> void*;
auto malloc_zone_calloc(malloc_zone_t*, std::size_t, std::size_t) -> void*;
auto malloc_zone_valloc(malloc_zone_t*, std::size_t) -> void*;
auto malloc_zone_memalign(malloc_zone_t*, std::size_t, std::size_t) -> void*;
void malloc_destroy_zone(malloc_zone_t*);
auto malloc_zone_batch_malloc(malloc_zone_t*, std::size_t, void**, unsigned) -> unsigned;
void malloc_zone_batch_free(malloc_zone_t*, void**, unsigned);
void malloc_zone_free(malloc_zone_t*, void*);
auto malloc_zone_realloc(malloc_zone_t*, void*, std::size_t) -> void*;
#endif

#ifdef __linux__
extern "C" {
#endif

auto __lsan_malloc(std::size_t) -> void*;
auto __lsan_calloc(std::size_t, std::size_t) -> void*;
auto __lsan_valloc(std::size_t) -> void*;
auto __lsan_aligned_alloc(std::size_t, std::size_t) -> void*;
auto __lsan_realloc(void*, std::size_t) -> void*;
void __lsan_free(void*);

#ifdef __linux__
}
#endif
}

#endif //WRAP_MALLOC_HPP

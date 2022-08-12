/*
 * LeakSanitizer - A small library showing informations about lost memory.
 *
 * Copyright (C) 2022  mhahnFr
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

#ifndef wrap_malloc_h
#define wrap_malloc_h

#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

void * __wrap_malloc(size_t, const char *, int);
void * __wrap_calloc(size_t, size_t, const char *, int);
void * __wrap_realloc(void *, size_t, const char *, int);
void   __wrap_free(void *, const char *, int);

[[ noreturn ]] void __wrap_exit(int, const char *, int);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* wrap_malloc_h */

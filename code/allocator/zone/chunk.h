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

#ifndef chunk_h
#define chunk_h

#include "../pageHeader.h"

struct chunk {
    struct pageHeader * page;
    
    struct chunk * next;
    struct chunk * previous;
};

static const size_t CHUNK_OVERHEAD = sizeof(struct chunk) - 2 * sizeof(struct chunk *);

static const size_t CHUNK_MINIMUM_SIZE = sizeof(struct chunk *) * 2;

static inline struct chunk * chunk_fromPointer(void * pointer) {
    return pointer - CHUNK_OVERHEAD;
}

static inline void * chunk_toPointer(struct chunk * self) {
    return (void *) self + CHUNK_OVERHEAD;
}

#endif /* chunk_h */

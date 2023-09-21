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

#ifndef chunk_medium_h
#define chunk_medium_h

#include "../../pageHeader.h"

/**
 * This structure represents a chunk of memory in the
 * medium zone.
 */
struct chunkMedium {
    /** The amount of user bytes in this chunk. */
    size_t size;
    
    /** The page this chunk comes from.         */
    struct pageHeader * page;
    
    /**
     * The next chunk in the list.
     *
     * This field is only available in deallocated chunks.
     */
    struct chunkMedium * next;
    /**
     * The previous chunk in the list.
     *
     * This field is only available in deallocated chunks.
     */
    struct chunkMedium * previous;
};

/** The amount of bytes used as overhead for medium chunks. */
static const size_t CHUNK_MEDIUM_OVERHEAD = sizeof(struct chunkMedium) - 2 * sizeof(struct chunkMedium *);

/**
 * Returns the chunk associated with the given pointer.
 *
 * @param pointer the pointer whose chunk to be returned
 * @return the associated chunk
 */
static inline struct chunkMedium * chunkMedium_fromPointer(void * pointer) {
    return pointer - CHUNK_MEDIUM_OVERHEAD;
}

/**
 * Returns the user pointer of the given chunk.
 *
 * @param self the chunk whose user memory block to be returned
 * @return the user pointer
 */
static inline void * chunkMedium_toPointer(struct chunkMedium * self) {
    return (void *) self + CHUNK_MEDIUM_OVERHEAD;
}

#endif /* chunk_medium_h */

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

#include <errno.h>

#include "zone_large.h"

#include "../chunk.h"

void * zoneLarge_allocate(struct zone * self, size_t size) {
    struct pageHeader * page = page_allocateMin(size + sizeof(struct pageHeader) + CHUNK_OVERHEAD, self->pageSize);
    
    if (page == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    page_add(&self->pages, page);
    
    struct chunk * chunk = (void *) page + sizeof(struct pageHeader);
    chunk->page = page;
    
    page->allocCount = size;
    page->zone = self;
    
    return chunk_toPointer(chunk);
}

bool zoneLarge_deallocate(struct zone * self, void * pointer) {
    struct chunk * chunk = chunk_fromPointer(pointer);
    struct pageHeader * page = (void *) chunk - sizeof(struct pageHeader);
   
    page_remove(&self->pages, page);
    page_deallocate(page);
    
    return true;
}

bool zoneLarge_enlarge(void * pointer, size_t newSize) {
    const struct pageHeader * page = (void *) chunk_fromPointer(pointer) - sizeof(struct pageHeader);
    
    return newSize < page->size - sizeof(struct pageHeader) - CHUNK_OVERHEAD;
}

size_t zoneLarge_getAllocationSize(void * pointer) {
    return chunk_fromPointer(pointer)->page->allocCount;
}

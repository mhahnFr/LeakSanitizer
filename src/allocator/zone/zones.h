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

#ifndef zones_h
#define zones_h

#include <stdbool.h>

#include "zone.h"
#include "medium/zone_medium.h"

#include "chunk.h"

#include "../pageHeader.h"

struct zones {
    bool inited;
    
    struct zone small;
    struct zone medium;
    struct zone large;
};

static inline struct zone * zones_getZoneBySize(struct zones * self, size_t size) {
    if (!self->inited) {
        const size_t pageSize = page_getPageSize() * PAGE_FACTOR;
        self->small.pageSize = pageSize;
        self->medium.pageSize = pageSize;
        self->large.pageSize = pageSize;
        self->inited = true;
    }
    
    if (size <= CHUNK_MINIMUM_SIZE) {
        return &self->small;
    } else if (size <= zoneMedium_maximumSize(&self->medium)) {
        return &self->medium;
    }
    return &self->large;
}

static inline struct zone * zones_getZoneByPointer(void * pointer) {
    struct chunk * chunk = chunk_fromPointer(pointer);
    
    if (chunk->page == NULL) {
        return NULL;
    }
    return chunk->page->zone;
}

#endif /* zones_h */

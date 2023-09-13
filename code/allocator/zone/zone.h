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

#ifndef zone_h
#define zone_h

#include <stdbool.h>
#include <stddef.h>

#include "../pageHeader.h"

enum zoneType {
    ZONE_SMALL,
    ZONE_MEDIUM,
    ZONE_LARGE
};

struct zone {
    enum zoneType type;
    
    size_t pageSize;
    
    struct pageHeader * pages;
};

void * zone_allocate(struct zone * self, size_t bytes);
bool   zone_deallocate(struct zone * self, void * pointer);

bool   zone_enlargeAllocation(struct zone * self, void * pointer, size_t newSize);
size_t zone_getAllocationSize(struct zone * self, void * pointer);

#endif /* zone_h */

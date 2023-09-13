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

#include "zone.h"

#include "small/zone_small.h"
#include "medium/zone_medium.h"
#include "large/zone_large.h"

void * zone_allocate(struct zone * self, size_t size) {
    switch (self->type) {
        case ZONE_SMALL:  return zoneSmall_allocate(self);
        case ZONE_MEDIUM: return zoneMedium_allocate(self, size);
        case ZONE_LARGE:  return zoneLarge_allocate(self, size);
            
        default: return NULL;
    }
}

bool zone_deallocate(struct zone * self, void * pointer) {
    switch (self->type) {
        case ZONE_SMALL:  return zoneSmall_deallocate(self, pointer);
        case ZONE_MEDIUM: return zoneMedium_deallocate(self, pointer);
        case ZONE_LARGE:  return zoneLarge_deallocate(self, pointer);
            
        default: return false;
    }
}

bool zone_enlargeAllocation(struct zone * self, void * pointer, size_t newSize) {
    switch (self->type) {
        case ZONE_SMALL:  return zoneSmall_enlarge(newSize);
        case ZONE_MEDIUM: return zoneMedium_enlarge(pointer, newSize);
        case ZONE_LARGE:  return zoneLarge_enlarge(pointer, newSize);
            
        default: return false;
    }
}

size_t zone_getAllocationSize(struct zone * self, void * pointer) {
    switch (self->type) {
        case ZONE_SMALL:  return zoneSmall_getAllocationSize();
        case ZONE_MEDIUM: return zoneMedium_getAllocationSize(pointer);
        case ZONE_LARGE:  return zoneLarge_getAllocationSize(pointer);
            
        default: return 0;
    }
}

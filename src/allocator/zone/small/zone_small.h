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

#ifndef zone_small_h
#define zone_small_h

#include "../zone.h"

/**
 * Allocates a chunk in the small zone.
 *
 * @param self the small zone object
 * @return the pointer to the user memory or `NULL` if unable to allocate
 */
void * zoneSmall_allocate(struct zone * self);
/**
 * Deallocates the given pointer.
 *
 * @param self the small zone object
 * @param pointer the pointer to be deallocated
 * @return whether the pointer was deallocated successfully
 */
bool zoneSmall_deallocate(struct zone * self, void * pointer);

/**
 * Returns whether a chunk of memory from this zone can be
 * enlarged to the given size.
 *
 * @param newSize the new size
 * @return whether the new size fits in a chunk from this zone
 */
bool zoneSmall_enlarge(size_t newSize);
/**
 * Returns the (fixed) allocation size of this zone.
 *
 * @return the fixed allocation size
 */
size_t zoneSmall_getAllocationSize(void);

#endif /* zone_small_h */

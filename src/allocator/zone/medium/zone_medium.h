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

#ifndef zone_medium_h
#define zone_medium_h

#include "../zone.h"

/**
 * Allocates the given amount of bytes.
 *
 * @param self the medium zone object
 * @param bytes the amount of bytes to be allocated
 * @return a pointer to the user memory or `NULL` if unable to allocate
 */
void * zoneMedium_allocate(struct zone * self, size_t bytes);
/**
 * Deallocates the given pointer.
 *
 * @param self the small zone object
 * @param pointer the pointer to be deallocated
 * @return whether the deallocation was successful
 */
bool zoneMedium_deallocate(struct zone * self, void * pointer);

/**
 * Tries to enlarge the memory block pointed to by the given pointer.
 *
 * @param pointer the pointer to be enlarged
 * @param newSize the new size of the memory block
 * @return whether the memory block was enlarged successfully
 */
bool zoneMedium_enlarge(void * pointer, size_t newSize);
/**
 * Returns the size of the memory block pointed to by the given pointer.
 *
 * @param pointer the pointer whose memory block's size to be returned
 * @return the size of the memory block
 */
size_t zoneMedium_getAllocationSize(void * pointer);

/**
 * Returns the biggest amount of bytes a user allocation can have in this zone.
 *
 * @param self the medium zone object
 * @return the maximum bytes valid to be requested from this zone
 */
size_t zoneMedium_maximumSize(struct zone * self);

#endif /* zone_medium_h */

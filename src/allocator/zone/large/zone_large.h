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

#ifndef zone_large_h
#define zone_large_h

#include "../zone.h"

/**
 * Allocates the given amount of bytes in the given large zone.
 *
 * @param self the large zone object
 * @param bytes the amount of bytes to be allocated
 * @return the user pointer or `NULL` if unable to allocate
 */
void * zoneLarge_allocate(struct zone * self, size_t bytes);
/**
 * Deallocates the given user pointer in the given large zone.
 *
 * @param self the large zone object
 * @param pointer the user pointer to be deallocated
 * @return whether the pointer was deallocated successfully
 */
bool zoneLarge_deallocate(struct zone * self, void * pointer);

/**
 * Returns the size of the memory block pointed to by the given pointer.
 *
 * @param pointer the user pointer
 * @return the size of the associated memory block
 */
size_t zoneLarge_getAllocationSize(void * pointer);
/**
 * Tries to enlarge the memory block pointed to by the given user pointer.
 *
 * @param pointer the user pointer
 * @param newSize the new size
 * @return whether the memory block could be enlarged
 */
bool zoneLarge_enlarge(void * pointer, size_t newSize);

#endif /* zone_large_h */

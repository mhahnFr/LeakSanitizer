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

#ifndef lsan_stats_h
#define lsan_stats_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

/**
 * Defaults to false. Setting it to true will cause this sanitizer to print the statistics upon normal
 * termination of the program.
 */
extern bool __lsan_printStatsOnExit;

/**
 * Returns the total count of allocations ever registered by this sanitizer.
 *
 * @return The total allocation count.
 */
size_t __lsan_getTotalMallocs();

/**
 * Returns the total count of allocated bytes ever registered by this sanitizer.
 *
 * @return The total amount of allocated bytes.
 */
size_t __lsan_getTotalBytes();

/**
 * Returns the total count of freed objects that were previously registered by this sanitizer.
 *
 * @return The total count of freed objects.
 */
size_t __lsan_getTotalFrees();

/**
 * Returns the count of the currently allocated objects registered by this sanitizer.
 *
 * @return The count of currently allocated objects.
 */
size_t __lsan_getCurrentMallocCount();

/**
 * Returns the amount of the currently allocated bytes registered by this sanitizer.
 *
 * @return The amount of currently allocated bytes.
 */
size_t __lsan_getCurrentByteCount();

/**
 * Returns the highest count of objects in the heap at the same time.
 *
 * @return The highest count of allocated objects.
 */
size_t __lsan_getMallocPeek();

/**
 * Returns the highest amount of bytes in the heap at the same time.
 *
 * @return The highest amount of allocated bytes.
 */
size_t __lsan_getBytePeek();

/**
 * Returns whether the memory statistics can savely be queried. If it returns false, but the memory
 * statistics are queried regardless, the library might crash!
 *
 * @return Whether the memory statistics can savely be queried.
 */
bool   __lsan_statsAvailable();

/**
 * Prints the statistics of the allocations. The bar has a size of 100 characters, it can be adjusted by
 * using __lsan_printStatsWithWidth(size_t). The output stream defined by __lsan_printCout is used
 * for the printing. This function already checks for the availability of the memory statistics using the
 * function __lsan_statsAvailable(), and guarantees to not crash the program, even in the case the
 * memory statistics are unavailable.
 */
void   __lsan_printStats();

/**
 * Prints the statistics of the allocations. The size of the bar is specified by the given argument. The
 * output stream defined by __lsan_printCout is used for the printing. This function already checks
 * for the availability of the memory statistics using the function __lsan_statsAvailable(), and guarantees
 * to not crash the program, even in the case the memory statistics are unavailable.
 *
 * @param width The width in characters the printed bar should have.
 */
void   __lsan_printStatsWithWidth(size_t width);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* lsan_stats_h */

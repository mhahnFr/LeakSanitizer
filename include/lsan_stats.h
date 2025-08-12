/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2025  mhahnFr
 *
 * This file is part of the LeakSanitizer.
 *
 * The LeakSanitizer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The LeakSanitizer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with the
 * LeakSanitizer, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef lsan_stats_h
#define lsan_stats_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Returns the total count of allocations ever registered by this
 * sanitizer.
 *
 * @return The total allocation count.
 */
size_t __lsan_getTotalMallocs();

/**
 * @brief Returns the total count of allocated bytes ever registered by this
 * sanitizer.
 *
 * @return The total amount of allocated bytes.
 */
size_t __lsan_getTotalBytes();

/**
 * @brief Returns the total count of freed objects that were previously
 * registered by this sanitizer.
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
 * @brief Returns the amount of the currently allocated bytes registered by
 * this sanitizer.
 *
 * @return The amount of currently allocated bytes.
 */
size_t __lsan_getCurrentByteCount();

/**
 * @brief Returns the highest count of objects in the heap at the same time.
 *
 * @return The highest count of allocated objects.
 */
size_t __lsan_getMallocPeek();

/**
 * @brief Returns the highest amount of bytes in the heap at the same time.
 *
 * @return The highest amount of allocated bytes.
 */
size_t __lsan_getBytePeek();

/**
 * @brief Prints the statistics of the memory fragmentation.
 *
 * The size of the bar is specified by the given argument. The output stream
 * defined by @c LSAN_PRINT_COUT is used for the printing. The byte amounts
 * are printed human-readable if @c LSAN_HUMAN_PRINT is set to @c true .<br>
 * This function already checks for the availability of the memory fragmentation
 * statistics using @c LSAN_STATS_ACTIVE, and guarantees to not crash the
 * program, even in the case the memory fragmentation statistics are unavailable.
 *
 * @param width The width in characters the printed bar should have.
 * @since 1.2
 */
void __lsan_printFragmentationStatsWithWidth(size_t width);

/**
 * @brief Prints the statistics of the memory fragmentation.
 *
 * The bar has a size of @c 100 characters, it can be adjusted by using
 * @c __lsan_printFragmentationStatsWithWidth(size_t) . The output stream
 * defined by @c LSAN_PRINT_COUT is used for the printing. The byte amounts
 * are printed human-readable if @c LSAN_HUMAN_PRINT is set to @c true .<br>
 * This function already checks for the availability of the memory statistics
 * using @c LSAN_STATS_ACTIVE and guarantees to not crash the program, even in
 * the case the memory fragmentation statistics are unavailable.
 *
 * @since 1.2
 */
static inline void __lsan_printFragmentationStats() {
    __lsan_printFragmentationStatsWithWidth(100);
}

/**
 * @brief Prints the statistics of the memory fragmentation.
 *
 * The bar has a size of @c 100 characters, it can be adjusted by using
 * @c __lsan_printFStatsWithWidth(size_t) . The output stream defined by
 * @c LSAN_PRINT_COUT is used for the printing. The byte amounts are printed
 * human-readable if @c LSAN_HUMAN_PRINT is set to @c true .<br>
 * This function already checks for the availability of the memory statistics
 * using @c LSAN_STATS_ACTIVE and guarantees to not crash the program, even in
 * the case the memory fragmentation statistics are unavailable.
 *
 * @since 1.2
 */
static inline void __lsan_printFStats() {
    __lsan_printFragmentationStats();
}

/**
 * @brief Prints the statistics of the memory fragmentation.
 *
 * The size of the bar is specified by the given argument. The output stream
 * defined by @c LSAN_PRINT_COUT is used for the printing. The byte amounts
 * are printed human-readable if @c LSAN_HUMAN_PRINT is set to @c true .<br>
 * This function already checks for the availability of the memory fragmentation
 * statistics using @c LSAN_STATS_ACTIVE, and guarantees to not crash the
 * program, even in the case the memory fragmentation statistics are unavailable.
 *
 * @param width The width in characters the printed bar should have.
 * @since 1.2
 */
static inline void __lsan_printFStatsWithWidth(size_t width) {
    __lsan_printFragmentationStatsWithWidth(width);
}

/**
 * @brief Prints the statistics of the allocations.
 *
 * The size of the bar is specified by the given argument. The output stream
 * defined by @c LSAN_PRINT_COUT is used for the printing. The byte amounts
 * are printed human-readable if @c LSAN_HUMAN_PRINT is set to @c true .<br>
 * This function already checks for the availability of the memory statistics
 * using @c LSAN_STATS_ACTIVE, and guarantees to not crash the program, even
 * in the case the memory statistics are unavailable.
 *
 * @param width The width in characters the printed bar should have.
 */
void   __lsan_printStatsWithWidth(size_t width);

/**
 * @brief Prints the statistics of the allocations.
 *
 * The bar has a size of @c 100 characters, it can be adjusted by using
 * @c __lsan_printStatsWithWidth(size_t) . The output stream defined by
 * @c LSAN_PRINT_COUT is used for the printing. The byte amounts are printed
 * human-readable if @c LSAN_HUMAN_PRINT is set to @c true .<br>
 * This function already checks for the availability of the memory statistics
 * using @c LSAN_STATS_ACTIVE, and guarantees to not crash the program, even
 * in the case the memory statistics are unavailable.
 */
static inline void __lsan_printStats() {
    __lsan_printStatsWithWidth(100);
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* lsan_stats_h */

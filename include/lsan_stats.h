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
 * @brief Defaults to false.
 *
 * Setting it to true will cause this sanitizer to print the statistics upon normal termination of the program.
 */
extern bool __lsan_printStatsOnExit;

/**
 * @brief Returns the total count of allocations ever registered by this sanitizer.
 *
 * @return The total allocation count.
 */
size_t __lsan_getTotalMallocs();

/**
 * @brief Returns the total count of allocated bytes ever registered by this sanitizer.
 *
 * @return The total amount of allocated bytes.
 */
size_t __lsan_getTotalBytes();

/**
 * @brief Returns the total count of freed objects that were previously registered by this sanitizer.
 *
 * @return The total count of freed objects.
 */
size_t __lsan_getTotalFrees();

/**
 * @brief Returns the count of the currently allocated objects registered by this sanitizer.
 *
 * @return The count of currently allocated objects.
 */
size_t __lsan_getCurrentMallocCount();

/**
 * @brief Returns the amount of the currently allocated bytes registered by this sanitizer.
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
 * @brief Returns whether the memory statistics can savely be queried.
 *
 * If it returns false, but the memory statistics are queried regardless, the library might crash!
 *
 * @return Whether the memory statistics can savely be queried.
 * @since 1.1
 */
bool   __lsan_statsAvailable();

/**
 * @brief Returns whether the memory fragmentation statistics can be queried savely.
 *
 * If it returns false, the statistics can be queried regardless without crash, but they might be wrong.
 *
 * @return Whether the memory fragmentation statistics are available.
 * @since 1.2
 */
bool   __lsan_fStatsAvailable();

/**
 * @brief Returns whether the memory fragmentation statistics can be queried savely.
 *
 * If it returns false, the statistics can be queried regardless without crash, but they might be wrong.
 *
 * @return Whether the memory fragmentation statisitcs are available.
 * @since 1.2
 */
bool   __lsan_fragStatsAvailable();

/**
 * @brief Returns whether the memory fragmentation statistics can be queried savely.
 *
 * If it returns false, the statistics can be queried regardless without crash, but they might be wrong.
 *
 * @return Whether the memory fragmentation statisitcs are available.
 * @since 1.2
 */
bool   __lsan_fragmentationStatsAvailable();

/**
 * @brief Prints the statistics of the memory fragmentation.
 *
 * The bar has a size of 100 characters, it can be adjusted by using __lsan_printFStatsWithWidth(size_t).
 * The output stream defined by __lsan_printCout is used for the printing. The byte amounts are printed
 * human readable if __lsan_humanPrint is set to true.
 * This function already checks for the availability of the memory statistics using the function
 * __lsan_fStatsAvailable() and guarantees to not crash the program, even in the case the memory
 * fragmentation statistics are unavailable.
 *
 * @since 1.2
 */
void   __lsan_printFStats();

/**
 * @brief Prints the statistics of the memory fragmentation.
 *
 * The bar has a size of 100 characters, it can be adjusted by using __lsan_printFragStatsWithWidth(size_t).
 * The output stream defined by __lsan_printCout is used for the printing. The byte amounts are printed
 * human readable if __lsan_humanPrint is set to true.
 * This function already checks for the availability of the memory statistics using the function
 * __lsan_fragStatsAvailable() and guarantees to not crash the program, even in the case the memory
 * fragmentation statistics are unavailable.
 *
 * @since 1.2
 */
void   __lsan_printFragStats();

/**
 * @brief Prints the statistics of the memory fragmentation.
 *
 * The bar has a size of 100 characters, it can be adjusted by using __lsan_printFragmentationStatsWithWidth(size_t).
 * The output stream defined by __lsan_printCout is used for the printing. The byte amounts are printed
 * human readable if __lsan_humanPrint is set to true.
 * This function already checks for the availability of the memory statistics using the function
 * __lsan_fragmentationStatsAvailable() and guarantees to not crash the program, even in the case the
 * memory fragmentation statistics are unavailable.
 *
 * @since 1.2
 */
void   __lsan_printFragmentationStats();

/**
 * @brief Prints the statistics of the memory fragmentation.
 *
 * The size of the bar is specified by the given argument. The output stream defined by __lsan_printCout
 * is used for the printing. The byte amounts are printed human readable if __lsan_humanPrint is set to true.
 * This function already checks for the availability of the memory fragmentation statistics using the
 * function __lsan_fStatsAvailable(), and guarantees to not crash the program, even in the case the
 * memory fragmentation statistics are unavailable.
 *
 * @param width The width in characters the printed bar should have.
 * @since 1.2
 */
void   __lsan_printFStatsWithWidth(size_t width);

/**
 * @brief Prints the statistics of the memory fragmentation.
 *
 * The size of the bar is specified by the given argument. The output stream defined by __lsan_printCout
 * is used for the printing. The byte amounts are printed human readable if __lsan_humanPrint is set to true.
 * This function already checks for the availability of the memory fragmentation statistics using the
 * function __lsan_fragStatsAvailable(), and guarantees to not crash the program, even in the case the
 * memory fragmentation statistics are unavailable.
 *
 * @param width The width in characters the printed bar should have.
 * @since 1.2
 */
void   __lsan_printFragStatsWithWidth(size_t width);

/**
 * @brief Prints the statistics of the memory fragmentation.
 *
 * The size of the bar is specified by the given argument. The output stream defined by __lsan_printCout
 * is used for the printing. The byte amounts are printed human readable if __lsan_humanPrint is set to true.
 * This function already checks for the availability of the memory fragmentation statistics using the
 * function __lsan_fragmentationStatsAvailable(), and guarantees to not crash the program, even in
 * the case the memory fragmentation statistics are unavailable.
 *
 * @param width The width in characters the printed bar should have.
 * @since 1.2
 */
void   __lsan_printFragmentationStatsWithWidth(size_t width);

/**
 * @brief Prints the statistics of the allocations.
 *
 * The bar has a size of 100 characters, it can be adjusted by using __lsan_printStatsWithWidth(size_t).
 * The output stream defined by __lsan_printCout is used for the printing. The byte amounts are printed
 * human readable if __lsan_humanPrint is set to true.
 * This function already checks for the availability of the memory statistics using the function
 * __lsan_statsAvailable(), and guarantees to not crash the program, even in the case the memory
 * statistics are unavailable.
 */
void   __lsan_printStats();

/**
 * @brief Prints the statistics of the allocations.
 *
 * The size of the bar is specified by the given argument. The output stream defined by __lsan_printCout
 * is used for the printing. The byte amounts are printed human readable if __lsan_humanPrint is set to true.
 * This function already checks for the availability of the memory statistics using the function
 * __lsan_statsAvailable(), and guarantees to not crash the program, even in the case the memory
 * statistics are unavailable.
 *
 * @param width The width in characters the printed bar should have.
 */
void   __lsan_printStatsWithWidth(size_t width);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* lsan_stats_h */

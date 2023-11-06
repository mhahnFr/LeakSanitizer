/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2023  mhahnFr
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

#include "deprecation.h"

#include "lsan_internals.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

/**
 * @deprecated Since v1.7 this option is no longer supported. Will be removed in v2.
 *
 * @brief Defaults to `false`.
 *
 * Setting it to `true` will cause this sanitizer to print the statistics upon normal termination of the program.
 */
DEPRECATED("Since version 1.7 this is no longer supported.")
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
 * @deprecated Since 1.5, refer to `__lsan_statsActive`. Will be removed in v2.
 *
 * @brief Returns whether the memory statistics can safely be queried.
 *
 * If it returns `false`, but the memory statistics are queried regardless, the library might crash!
 *
 * @return Whether the memory statistics can safely be queried.
 * @since 1.1
 */
DEPRECATED("Since v1.5, refer to __lsan_statsActive")
static inline bool __lsan_statsAvailable() {
    return __lsan_statsActive;
}

/**
 * @deprecated Since 1.5, replaced by `__lsan_statsActive`. Will be removed in v2.
 *
 * @brief Returns whether the memory fragmentation statistics can be queried safely.
 *
 * If it returns `false`, the statistics can be queried regardless without crash, but they might be wrong.
 *
 * @return Whether the memory fragmentation statistics are available.
 * @since 1.2
 */
DEPRECATED("Since v1.5 replaced by __lsan_statsActive")
static inline bool __lsan_fStatsAvailable() {
    return __lsan_statsActive;
}

/**
 * @deprecated Since 1.5, replaced by `__lsan_statsActive`. Will be removed in v2.
 *
 * @brief Returns whether the memory fragmentation statistics can be queried safely.
 *
 * If it returns `false`, the statistics can be queried regardless without crash, but they might be wrong.
 *
 * @return Whether the memory fragmentation statistics are available.
 * @since 1.2
 */
DEPRECATED("Since v1.5 replaced by __lsan_statsActive")
static inline bool __lsan_fragStatsAvailable() {
    return __lsan_statsActive;
}

/**
 * @deprecated Since 1.5 replaced by `__lsan_statsActive`. Will be removed in v2.
 *
 * @brief Returns whether the memory fragmentation statistics can be queried safely.
 *
 * If it returns `false`, the statistics can be queried regardless without crash, but they might be wrong.
 *
 * @return Whether the memory fragmentation statistics are available.
 * @since 1.2
 */
DEPRECATED("Since v1.5 replaced by __lsan_statsActive")
static inline bool __lsan_fragmentationStatsAvailable() {
    return __lsan_statsActive;
}

/**
 * @brief Prints the statistics of the memory fragmentation.
 *
 * The size of the bar is specified by the given argument. The output stream defined by `__lsan_printCout`
 * is used for the printing. The byte amounts are printed human readable if `__lsan_humanPrint` is set to true.
 * This function already checks for the availability of the memory fragmentation statistics using
 * `__lsan_statsActive`, and guarantees to not crash the program, even in
 * the case the memory fragmentation statistics are unavailable.
 *
 * @param width The width in characters the printed bar should have.
 * @since 1.2
 */
void __lsan_printFragmentationStatsWithWidth(size_t width);

/**
 * @brief Prints the statistics of the memory fragmentation.
 *
 * The bar has a size of 100 characters, it can be adjusted by using `__lsan_printFragmentationStatsWithWidth(size_t)`.
 * The output stream defined by `__lsan_printCout` is used for the printing. The byte amounts are printed
 * human readable if `__lsan_humanPrint` is set to true.
 * This function already checks for the availability of the memory statistics using
 * `__lsan_statsActive` and guarantees to not crash the program, even in the case the
 * memory fragmentation statistics are unavailable.
 *
 * @since 1.2
 */
static inline void __lsan_printFragmentationStats() {
    __lsan_printFragmentationStatsWithWidth(100);
}

/**
 * @brief Prints the statistics of the memory fragmentation.
 *
 * The bar has a size of 100 characters, it can be adjusted by using `__lsan_printFStatsWithWidth(size_t)`.
 * The output stream defined by `__lsan_printCout` is used for the printing. The byte amounts are printed
 * human readable if `__lsan_humanPrint` is set to true.
 * This function already checks for the availability of the memory statistics using
 * `__lsan_statsActive` and guarantees to not crash the program, even in the case the memory
 * fragmentation statistics are unavailable.
 *
 * @since 1.2
 */
static inline void __lsan_printFStats() {
    __lsan_printFragmentationStats();
}

/**
 * @brief Prints the statistics of the memory fragmentation.
 *
 * The bar has a size of 100 characters, it can be adjusted by using `__lsan_printFragStatsWithWidth(size_t)`.
 * The output stream defined by `__lsan_printCout` is used for the printing. The byte amounts are printed
 * human readable if `__lsan_humanPrint` is set to true.
 * This function already checks for the availability of the memory statistics using
 * `__lsan_statsActive` and guarantees to not crash the program, even in the case the memory
 * fragmentation statistics are unavailable.
 *
 * @since 1.2
 */
static inline void __lsan_printFragStats() {
    __lsan_printFragmentationStats();
}

/**
 * @brief Prints the statistics of the memory fragmentation.
 *
 * The size of the bar is specified by the given argument. The output stream defined by `__lsan_printCout`
 * is used for the printing. The byte amounts are printed human readable if `__lsan_humanPrint` is set to true.
 * This function already checks for the availability of the memory fragmentation statistics using
 * `__lsan_statsActive`, and guarantees to not crash the program, even in the case the
 * memory fragmentation statistics are unavailable.
 *
 * @param width The width in characters the printed bar should have.
 * @since 1.2
 */
static inline void __lsan_printFStatsWithWidth(size_t width) {
    __lsan_printFragmentationStatsWithWidth(width);
}

/**
 * @brief Prints the statistics of the memory fragmentation.
 *
 * The size of the bar is specified by the given argument. The output stream defined by `__lsan_printCout`
 * is used for the printing. The byte amounts are printed human readable if `__lsan_humanPrint` is set to true.
 * This function already checks for the availability of the memory fragmentation statistics using
 * `__lsan_statsActive`, and guarantees to not crash the program, even in the case the
 * memory fragmentation statistics are unavailable.
 *
 * @param width The width in characters the printed bar should have.
 * @since 1.2
 */
static inline void __lsan_printFragStatsWithWidth(size_t width) {
    __lsan_printFragmentationStatsWithWidth(width);
}

/**
 * @brief Prints the statistics of the allocations.
 *
 * The size of the bar is specified by the given argument. The output stream defined by `__lsan_printCout`
 * is used for the printing. The byte amounts are printed human readable if `__lsan_humanPrint` is set to true.
 * This function already checks for the availability of the memory statistics using
 * `__lsan_statsActive`, and guarantees to not crash the program, even in the case the memory
 * statistics are unavailable.
 *
 * @param width The width in characters the printed bar should have.
 */
void   __lsan_printStatsWithWidth(size_t width);

/**
 * @brief Prints the statistics of the allocations.
 *
 * The bar has a size of 100 characters, it can be adjusted by using `__lsan_printStatsWithWidth(size_t)`.
 * The output stream defined by `__lsan_printCout` is used for the printing. The byte amounts are printed
 * human readable if `__lsan_humanPrint` is set to true.
 * This function already checks for the availability of the memory statistics using
 * `__lsan_statsActive`, and guarantees to not crash the program, even in the case the memory
 * statistics are unavailable.
 */
static inline void __lsan_printStats() {
    __lsan_printStatsWithWidth(100);
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* lsan_stats_h */

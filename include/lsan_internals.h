/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2025  mhahnFr and contributors
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

#ifndef lsan_internals_h
#define lsan_internals_h

#include "deprecation.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief If this value is set to @c true, the byte amounts are printed in a
 * human-readable way.
 *
 * Defaults to @c true .
 */
_LSAN_DEPRECATED("The __lsan_* variables will be removed in version 2; use environment variables instead")
extern bool __lsan_humanPrint;

/**
 * @brief If this value is set to @c true, normal messages are printed to the
 * standard output stream.
 *
 * Otherwise, the standard error stream is also used for normal messages.<br>
 * Defaults to @c false .
 *
 * @since 1.1
 */
_LSAN_DEPRECATED("The __lsan_* variables will be removed in version 2; use environment variables instead")
extern bool __lsan_printCout;

/**
 * @brief If this value is set to @c true, ANSI escape codes are used to format
 * the output of this sanitizer.
 *
 * Otherwise, the output is not formatted using escape codes.<br>
 * Defaults to @c true .
 *
 * @since 1.1
 */
_LSAN_DEPRECATED("The __lsan_* variables will be removed in version 2; use environment variables instead")
extern bool __lsan_printFormatted;

/**
 * @deprecated Since v1.8 this option is no longer supported. Will be removed in v2.
 *
 * @brief If this value is set to @c true, the license information is printed
 * upon normal termination of the program.
 *
 * Defaults value is set by the Makefile.
 *
 * @since 1.1
 */
LSAN_DEPRECATED("Since version 1.8 this is no longer supported")
extern bool __lsan_printLicense;

/**
 * @deprecated Since v1.8 this option is no longer supported. Will be removed in v2.
 *
 * @brief If this value is set to @c true, the link to the home page of the
 * LeakSanitizer is printed.
 *
 * Default value is set by the Makefile.
 *
 * @since 1.4
 */
LSAN_DEPRECATED("Since version 1.8 this is no longer supported")
extern bool __lsan_printWebsite;

/**
 * @brief If this value is set to @c true, the program is terminated when doing
 * something invalid regarding the memory management.
 *
 * Defaults to @c true .
 */
_LSAN_DEPRECATED("The __lsan_* variables will be removed in version 2; use environment variables instead")
extern bool __lsan_invalidCrash;

/**
 * @brief If this value is set to @c true, the freed pointers are checked for
 * whether they have previously been allocated using this sanitizer.
 *
 * If a pointer which is unknown to the sanitizer is freed, a warning or a
 * termination is issued, according to the value @c __lsan_invalidCrash .
 * Freeing a null pointer is not affected by this flag, but by @c __lsan_freeNull .<br>
 * Defaults to @c true since version 1.10.
 */
_LSAN_DEPRECATED("The __lsan_* variables will be removed in version 2; use environment variables instead")
extern bool __lsan_invalidFree;

/**
 * @brief If this value is set to @c true, a warning is issued when a null
 * pointer is freed.
 *
 * It does not cause a termination of the program, regardless of
 * @c __lsan_invalidCrash .<br>
 * Defaults to @c false .
 */
_LSAN_DEPRECATED("The __lsan_* variables will be removed in version 2; use environment variables instead")
extern bool __lsan_freeNull;

/**
 * @brief If this value is set to @c true, a warning is issued when zero bytes
 * are allocated.
 *
 * It does not cause a termination of the program, regardless of
 * @c __lsan_invalidCrash .<br>
 * Defaults to @c false .
 *
 * @since 1.8
 */
_LSAN_DEPRECATED("The __lsan_* variables will be removed in version 2; use environment variables instead")
extern bool __lsan_zeroAllocation;

/**
 * @deprecated Since 1.5 replaced by @c __lsan_statsActive . Will be removed in v2.
 *
 * @brief If this value is set to @c true, the memory fragmentation can be analyzed.
 *
 * It should be set at the very beginning of the program in order to get realistic results.
 * Defaults to @c false .
 *
 * @since 1.2
 */
LSAN_DEPRECATED("Since v1.5 replaced by __lsan_statsActive")
extern bool __lsan_trackMemory;

/**
 * @brief If this value is set to @c true, the memory allocation statistics can
 * be analyzed.
 *
 * It should be set at the very beginning of the program in order to get
 * realistic results.<br>
 * Defaults @c false .
 *
 * @since 1.5
 */
_LSAN_DEPRECATED("The __lsan_* variables will be removed in version 2; use environment variables instead")
extern bool __lsan_statsActive;

/**
 * @brief If this value is set to @c true, a callstack of the exit point is
 * printed upon regular termination.
 *
 * Defaults to @c false .
 *
 * @since 1.7
 */
_LSAN_DEPRECATED("The __lsan_* variables will be removed in version 2; use environment variables instead")
extern bool __lsan_printExitPoint;

/**
 * @brief If this value is set to @c true, the name of the binary file a given
 * callstack frame comes from is printed as well.
 *
 * Defaults to @c true .
 *
 * @since 1.8
 */
_LSAN_DEPRECATED("The __lsan_* variables will be removed in version 2; use environment variables instead")
extern bool __lsan_printBinaries;

/**
 * @brief If this value is set to @c false the function names are omitted if
 * source file and line information is available.
 *
 * Defaults to @c true .
 *
 * @since 1.8
 */
_LSAN_DEPRECATED("The __lsan_* variables will be removed in version 2; use environment variables instead")
extern bool __lsan_printFunctions;

/**
 * @brief If this value is set to @c true, the printed file paths are allowed
 * to be relative paths.
 *
 * Defaults to @c true .
 *
 * @since v1.8
 */
_LSAN_DEPRECATED("The __lsan_* variables will be removed in version 2; use environment variables instead")
extern bool __lsan_relativePaths;

/**
 * @brief This value defines the count of leaks that are printed at the exit of
 * the program.
 *
 * If more leaks are detected, the first leaks are printed and a message about
 * the truncation is also printed.<br>
 * Defaults to @c 100 .
 *
 * @since 1.3
 */
_LSAN_DEPRECATED("The __lsan_* variables will be removed in version 2; use environment variables instead")
extern size_t __lsan_leakCount;

/**
 * @brief This value defines the number of functions that are printed in a
 * callstack.
 *
 * If there are more functions in such a callstack, the top most functions are
 * printed and a message about the truncation is printed.<br>
 * Defaults to @c 20 .
 *
 * @since 1.3
 */
_LSAN_DEPRECATED("The __lsan_* variables will be removed in version 2; use environment variables instead")
extern size_t __lsan_callstackSize;

/**
 * @deprecated Since version 1.11 the old suppression system is no longer
 * supported - use the new suppression files instead. Will be removed in version 2.
 *
 * @brief This value defines the number of first party frames upon which
 * callstacks are considered to be first party.
 *
 * Up to this amount of frames callstacks are considered to be user initiated.<br>
 * Defaults to @c 3 .
 *
 * @since 1.7
 */
LSAN_DEPRECATED("Since version 1.11 this is no longer supported")
extern size_t __lsan_firstPartyThreshold;

/**
 * @deprecated Since version 1.11 the old suppression system is no longer
 * supported - use the new suppression files instead. Will be removed in version 2.
 *
 * @brief This string defines the regex pattern for which binary file names are
 * considered to be first party.
 *
 * These regular expressions are applied to the absolute path of the binary files.
 *
 * @since 1.8
 */
LSAN_DEPRECATED("Since version 1.11 this is no longer supported")
extern const char* __lsan_firstPartyRegex;

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* lsan_internals_h */

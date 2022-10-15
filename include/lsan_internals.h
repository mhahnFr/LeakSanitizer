/*
 * LeakSanitizer - A small library showing informations about lost memory.
 *
 * Copyright (C) 2022  mhahnFr and contributors
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

#ifndef lsan_internals_hpp
#define lsan_internals_hpp

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief If this value is set to true, the byte amounts are printed in a human friendly way.
 *
 * Defaults to true.
 */
extern bool __lsan_humanPrint;

/**
 * @brief If this value is set to true, normal messages are printed to the standard output stream.
 *
 * Otherwise the standard error stream is also used for normal messages.
 * Defaults to true.
 *
 * @since 1.1
 */
extern bool __lsan_printCout;

/**
 * @brief If this value is set to true, ANSII escape codes are used to format the output of this sanitizer.
 *
 * Otherwise, the output is not formatted using escape codes.
 * Defaults to true.
 *
 * @since 1.1
 */
extern bool __lsan_printFormatted;

/**
 * @brief If this value is set to true, the license informations are printed upon normal termination
 * of the program.
 *
 * Defaults to true.
 *
 * @since 1.1
 */
extern bool __lsan_printLicense;

/**
 * @brief If this value is set to true, the link to the home page of the LeakSanitizer is printed.
 *
 * Default value is set by the Makefile.
 *
 * @since 1.4
 */
extern bool __lsan_printWebsite;

/**
 * @brief If this value is set to true, the program is terminated when doing something invalid
 * regarding the memory management.
 *
 * Defaults to true.
 */
extern bool __lsan_invalidCrash;

/**
 * @brief If this value is set to true, the freed pointers are checked for whether they have
 * previously been allocated using this sanitizer.
 *
 * If a pointer which is unknown to the sanitizer is freed, a warning or a termination is issued,
 * according to the value __lsan_invalidCrash. Freeing a null pointer is not checked by this flag,
 * but by __lsan_freeNull.
 * Defaults to false.
 */
extern bool __lsan_invalidFree;

/**
 * @brief If this value is set to true, a warning is issued when a null pointer is freed.
 *
 * It does not cause a termination of the program, regardless of __lsan_invalidCrash.
 * Default value is system dependedly set.
 */
extern bool __lsan_freeNull;

/**
 * @brief If this value is set to true, the memory fragmentation can be analyzed.
 *
 * It should be set at the very beginning of the program in order to get realistic results.
 * Defaults to false.
 *
 * @since 1.2
 */
extern bool __lsan_trackMemory;

/**
 * @brief This value defines the count of leaks that are printed at the exit of the program.
 *
 * If more leaks are detected, the first leaks are printed and a message about the truncation
 * is also printed.
 * Defaults to 100.
 *
 * @since 1.3
 */
extern size_t __lsan_leakCount;

/**
 * @brief This value defines the number of functions that are printed in a callstack.
 *
 * If there are more functions in such a callstack, the top most functions are printed
 * and a message about the truncation is printed.
 * Defaults to 20.
 *
 * @since 1.3
 */
extern size_t __lsan_callstackSize;

#ifdef __cplusplus
} // extern "C"
#endif

#ifndef VERSION
# define VERSION "clean build"
#endif

#endif /* lsan_internals_hpp */

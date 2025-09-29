/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2025  mhahnFr
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

#ifndef LeakSanitizer_h
#define LeakSanitizer_h

#ifdef __cplusplus
extern "C" {
#endif

//! Project version number for LeakSanitizer.
extern double LeakSanitizerVersionNumber;

//! Project version string for LeakSanitizer.
extern const unsigned char LeakSanitizerVersionString[];

#ifdef __cplusplus
} // extern "C"
#endif

#include <LeakSanitizer/lsan_stats.h>

#endif /* LeakSanitizer_h */

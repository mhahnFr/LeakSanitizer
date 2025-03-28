/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023 - 2025  mhahnFr
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

#ifndef __lsan_deprecation_h
#define __lsan_deprecation_h

#if (defined(__cplusplus) && __cplusplus >= 201402L) || (defined(__STDC_VERSION__) && __STDC_VERSION >= 202311L)
# define LSAN_DEPRECATED(message) [[ deprecated(message) ]]
#elif defined(__GNUC__) || defined(__clang__)
# define LSAN_DEPRECATED(message) __attribute__((deprecated(message)))
#else
# define LSAN_DEPRECATED(message)
#endif

#ifndef __LSAN_SILENCE_DEPRECATION
# define _LSAN_DEPRECATED(message) LSAN_DEPRECATED(message)
#else
# define _LSAN_DEPRECATED(message)
#endif

#define LSAN_DEPRECATED_PLAIN LSAN_DEPRECATED("")

#endif /* __lsan_deprecation_h */

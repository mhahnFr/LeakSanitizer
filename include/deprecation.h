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

#ifndef deprecation_h
#define deprecation_h

#if (defined(__cplusplus) && __cplusplus >= 201402L) || (defined(__STDC_VERSION__) && __STDC_VERSION >= 202311L)
 #define DEPRECATED(message) [[ deprecated(message) ]]
#elif defined(__GNUC__) || defined(__clang__)
 #define DEPRECATED(message) __attribute__((deprecated(message)))
#else
 #define DEPRECATED(message)
#endif

#endif /* deprecation_h */

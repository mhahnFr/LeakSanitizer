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

#ifndef crashWarner_h
#define crashWarner_h

#ifdef __cplusplus
 #define NORETURN [[ noreturn ]]
#else
 #define NORETURN _Noreturn
#endif

#ifdef __cplusplus
extern "C" {
#endif

void __lsan_warn(const char * message);
NORETURN void __lsan_crash(const char * message);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* crashWarner_h */

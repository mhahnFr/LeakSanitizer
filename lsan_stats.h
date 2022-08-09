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

extern bool __lsan_printStatsOnExit;

size_t __lsan_getTotalMallocs();
size_t __lsan_getTotalBytes();
size_t __lsan_getTotalFrees();

size_t __lsan_getCurrentMallocCount();
size_t __lsan_getCurrentByteCount();

size_t __lsan_getMallocPeek();
size_t __lsan_getBytePeek();

void   __lsan_printStats();
void   __lsan_printStatsWithWidth(size_t);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* lsan_stats_h */

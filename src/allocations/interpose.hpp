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

#ifndef interpose_hpp
#define interpose_hpp

struct interpose {
    const void * newFunc;
    const void * oldFunc;
};

#define INTERPOSE(NEW, OLD)                                                  \
static const struct interpose interpose_##OLD                                \
    __attribute__((used, section("__DATA, __interpose"))) = {                \
        reinterpret_cast<const void *>(reinterpret_cast<uintptr_t>(&(NEW))), \
        reinterpret_cast<const void *>(reinterpret_cast<uintptr_t>(&(OLD)))  \
    }

#endif /* interpose_hpp */

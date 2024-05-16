/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2024  mhahnFr
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

#ifndef helperStructs_hpp
#define helperStructs_hpp

#include "MallocInfo.hpp"

namespace lsan {
struct LeakKindStats {
    std::size_t stack = 0, stackIndirect = 0, global = 0, globalIndirect = 0, tlv = 0, tlvIndirect = 0, lost = 0, lostIndirect = 0;

    std::set<MallocInfo*> recordsStack;
    std::set<MallocInfo*> recordsGlobal;
    std::set<MallocInfo*> recordsTlv;
    std::set<MallocInfo*> recordsLost;
};

struct Region {
    void *begin, *end;

    Region(void* begin, void* end): begin(begin), end(end) {}
};
}

#endif /* helperStructs_hpp */

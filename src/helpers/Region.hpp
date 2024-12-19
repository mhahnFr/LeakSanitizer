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

#ifndef Region_hpp
#define Region_hpp

namespace lsan {
/**
 * This strcuture represents a memory region.
 */
struct Region {
    /** The begin of the region. */
    void* begin,
    /** The end of the region.   */
        * end;

    /**
     * Constructs a region defined by the given bounds.
     *
     * @param begin the begin of the region
     * @param end the end of the region
     */
    Region(void* begin, void* end): begin(begin), end(end) {}
};
}

#endif /* Region_hpp */
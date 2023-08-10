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

#ifndef ATracker_h
#define ATracker_h

#include <atomic>

#include "MallocInfo.hpp"

class ATracker {
    std::atomic_bool ignoreMalloc;
    
public:
    virtual ~ATracker() {}
    
    virtual void addMalloc(MallocInfo && info) = 0;
    virtual auto changeMalloc(const MallocInfo & info) -> bool = 0;
    virtual auto removeMalloc(const void * pointer) -> bool = 0;
    
    inline auto removeMalloc(MallocInfo && info) -> bool {
        return removeMalloc(info.getPointer());
    }
    
    constexpr inline void setIgnoreMalloc(const bool ignoreMalloc) {
        this->ignoreMalloc = ignoreMalloc;
    }
    
    constexpr inline auto getIgnoreMalloc() const -> bool {
        return ignoreMalloc;
    }
};

#endif /* ATracker_h */

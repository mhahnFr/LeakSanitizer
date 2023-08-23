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

#ifndef ATracker_hpp
#define ATracker_hpp

#include "MallocInfo.hpp"

/**
 * This class defines the functionality of an allocation tracker.
 */
class ATracker {
public:
    virtual ~ATracker() {}
    
    /**
     * Adds the given allocation record to the tracked allocations.
     *
     * @param info the allocation record to be added
     */
    virtual void addMalloc(MallocInfo && info) = 0;
    /**
     * Attempts to exchange the allocation record associated with the given
     * allocation record by the given allocation record.
     *
     * Implementors note: the record should be searched for globally if it is not
     * found in this instance.
     *
     * @param info the allocation record to be exchanged
     * @return whether the record was replaced
     */
    virtual auto changeMalloc(const MallocInfo & info) -> bool = 0;
    /**
     * Attempts to remove the allocation record associated with the given pointer.
     *
     * Implementors note: the record should be searched for globally if it is not
     * found in this instance.
     *
     * @param pointer the deallocated pointer
     * @return whether the record was removed
     */
    virtual auto removeMalloc(const void * pointer)    -> bool = 0;
    
    /**
     * Attempts to remove the allocation record associated with the given record.
     *
     * Implementors note: the record should be searched for globally if it is not
     * found in this instance.
     *
     * @param info the allocation record
     * @return whether the record was removed
     */
    inline auto removeMalloc(MallocInfo && info) -> bool {
        return removeMalloc(info.getPointer());
    }
};

#endif /* ATracker_hpp */

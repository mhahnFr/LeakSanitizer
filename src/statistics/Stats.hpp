/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2023  mhahnFr
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

#ifndef Stats_hpp
#define Stats_hpp

#include <cstddef>
#include <mutex>

#include "../MallocInfo.hpp"

namespace lsan {
/**
 * This class contains all statistics that this sanitizer produces.
 */
class Stats {
    /** The mutex used to protect the statistics.                     */
    mutable std::mutex mutex;
    
    /** The count of currently active allocations.                    */
    std::size_t currentMallocCount = 0,
    /** The total count of allocations tracked by this sanitizer.     */
                  totalMallocCount = 0,
    /** The maximal count of active allocations at one time.          */
                   peekMallocCount = 0;

    /** The count of currently allocated bytes.                       */
    std::size_t currentBytes = 0,
    /** The total count of allocated bytes tracked by this sanitizer. */
                  totalBytes = 0,
    /** The maximal count of active allocated bytes at one time.      */
                   peekBytes = 0;
    
    /** The count of deallocations tracked by this sanitizer.         */
    std::size_t freeCount = 0;
    
public:
    Stats() = default;
   ~Stats() = default;
    
    Stats(const Stats & other);
    Stats(Stats && other);
    
    Stats & operator=(const Stats & other);
    Stats & operator=(Stats && other);
    
    /**
     * Returns the count of currently active allocations.
     *
     * @return the count of currently active allocations
     */
    auto getCurrentMallocCount() const -> std::size_t;
    /**
     * Returns the total count of tracked allocations.
     *
     * @return the total count of allocations
     */
    auto getTotalMallocCount()   const -> std::size_t;
    /**
     * Returns the maximal count of allocations active at one time.
     *
     * @return the peek of allocations
     */
    auto getMallocPeek()         const -> std::size_t;
    
    /**
     * Returns the count of currently allocated bytes.
     *
     * @return the amount of currently allocated bytes
     */
    auto getCurrentBytes() const -> std::size_t;
    /**
     * Returns the total count of allocated bytes tracked by this sanitizer.
     *
     * @return the total count of tracked allocated bytes
     */
    auto getTotalBytes()   const -> std::size_t;
    /**
     * Returns the maximal count of allocated bytes active at one time.
     *
     * @return the peek of allocated bytes at one time
     */
    auto getBytePeek()     const -> std::size_t;
    
    /**
     * Returns the total count of deallocations tracked by this sanitizer.
     *
     * @return the total count of deallocations
     */
    auto getTotalFreeCount() const -> std::size_t;
    
    /**
     * Adds the given size to the tracked allocations.
     *
     * Each invocation of this method counts as one allocated object.
     *
     * @param size the size of the allocated object
     */
    void addMalloc(std::size_t size);
    /**
     * Adds the given allocation record to the tracked allocations.
     *
     * @param info the allocation record to append
     */
    inline void addMalloc(const MallocInfo & info) {
        addMalloc(info.getSize());
    }
    
    /**
     * Exchanges an allocation using the given values.
     *
     * @param oldSize the size to substract
     * @param newSize the size to add
     */
    void replaceMalloc(std::size_t oldSize, std::size_t newSize);
    
    /**
     * Adds a deallocation to the statistics.
     *
     * Each invocation of this method counts as one deallocated object.
     *
     * @param size the size to of the deallocated object
     */
    void addFree(std::size_t size);
    /**
     * Adds a deallocation to the statistics.
     *
     * @param info the allocation record that should be removed from the statistics
     */
    inline void addFree(const MallocInfo & info) {
        addFree(info.getSize());
    }
    
    /**
     * Adds the given allocation record to this instance and returns itself.
     *
     * @param info the allocation record to append
     * @return this instance
     */
    inline auto operator+=(const MallocInfo & info) -> Stats & {
        addMalloc(info);
        return *this;
    }
    /**
     * Removes the given allocation record from this instance and returns itself.
     *
     * @param info the allocation record to be substracted
     * @return this instance
     */
    inline auto operator-=(const MallocInfo & info) -> Stats & {
        addFree(info);
        return *this;
    }
};
}

#endif /* Stats_hpp */

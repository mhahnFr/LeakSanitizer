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

#ifndef Stats_hpp
#define Stats_hpp

#include <cstddef>
#include <mutex>
#include <thread>
#include "MallocInfo.hpp"

/// This class contains all statistics that this sanitizer produces.
class Stats {
    /// The mutex used to protect the statistics.
    std::mutex mutex;
    
    /// The count of currently active allocations.
    size_t currentMallocCount = 0,
    /// The total count of allocations tracked by this sanitizer.
             totalMallocCount = 0,
    /// The maximal count of active allocations at one time.
              peekMallocCount = 0;

    /// The count of currently allocated bytes.
    size_t currentBytes = 0,
    /// The total count of allocated bytes tracked by this sanitizer.
             totalBytes = 0,
    /// The maximal count of active allocated bytes at one time.
              peekBytes = 0;
    
    /// The count of deallocations tracked by this sanitizer.
    size_t freeCount = 0;
    
public:
    Stats() = default;
   ~Stats() = default;
    
    /**
     * Trivial copy constructor.
     *
     * @param other the other instance to be copied
     */
    Stats(const Stats & other);
    /**
     * Trivial move constructor.
     *
     * @param other the other instance to be moved into this instance
     */
    Stats(Stats && other);
    
    /**
     * Trivial copy assign operator.
     *
     * @param other the other instance to be copied
     */
    Stats & operator=(const Stats & other);
    /**
     * Trivial move assign operator.
     *
     * @param other the other instance to be moved into this instance
     */
    Stats & operator=(Stats && other);

    /**
     * Returns the count of currently active allocations.
     *
     * @return the count of currently active allocations
     */
    auto getCurrentMallocCount() const -> size_t;
    /**
     * Returns the total count of tracked allocations.
     *
     * @return the total count of allocations
     */
    auto getTotalMallocCount()   const -> size_t;
    /**
     * Returns the maximal count of allocations active at one time.
     *
     * @return the peek of allocations
     */
    auto getMallocPeek()         const -> size_t;
    
    /**
     * Returns the count of currently allocated bytes.
     *
     * @return the amount of currently allocated bytes
     */
    auto getCurrentBytes() const -> size_t;
    /**
     * Returns the total count of allocated bytes tracked by this sanitizer.
     *
     * @return the total count of tracked allocated bytes
     */
    auto getTotalBytes()   const -> size_t;
    /**
     * Returns the maximal count of allocated bytes active at one time.
     *
     * @return the peek of allocated bytes at one time
     */
    auto getBytePeek()     const -> size_t;
    
    /**
     * Returns the total count of deallocations tracked by this sanitizer.
     *
     * @return the total count of deallocations
     */
    auto getTotalFreeCount() const -> size_t;
    
    /**
     * Adds the given size to the tracked allocations.
     *
     * Each invocation of this method counts as one allocated object.
     *
     * @param size the size of the allocated object
     */
    void addMalloc(size_t size);
    /**
     * Adds the given allocation record to the tracked allocations.
     *
     * @param info the allocation record to append
     */
    void addMalloc(const MallocInfo & info);
    
    /**
     * Exchanges an allocation using the given values.
     *
     * @param oldSize the size to substract
     * @param newSize the size to add
     */
    void replaceMalloc(size_t oldSize, size_t newSize);
    
    /**
     * Adds a deallocation to the statistics.
     *
     * Each invocation of this method counts as one deallocated object.
     *
     * @param size the size to of the deallocated object
     */
    void addFree(size_t size);
    /**
     * Adds a deallocation to the statistics.
     *
     * @param info the allocation record that should be removed from the statistics
     */
    void addFree(const MallocInfo & info);
    
    /**
     * Adds the given allocation record to a copy of this instance and returns that copy.
     *
     * @param info the allocation record to append to the copy
     * @return the result of the calculation
     */
    auto operator+ (const MallocInfo & info) -> Stats;
    /**
     * Adds the given allocation record to this instance and returns itself.
     *
     * @param info the allocation record to append
     * @return this instance
     */
    auto operator+=(const MallocInfo & info) -> Stats &;
    
    /**
     * Removes the given allocation record from a copy of this instance and returns
     * that copy.
     *
     * @param info the allocation record to substract from the copy
     * @return the result of te calculation
     */
    auto operator- (const MallocInfo & info) -> Stats;
    /**
     * Removes the given allocation record from this instance and returns itself.
     *
     * @param info the allocation record to be substracted
     * @return this instance
     */
    auto operator-=(const MallocInfo & info) -> Stats &;
};

#endif /* Stats_hpp */

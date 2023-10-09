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

#ifndef LeakSani_hpp
#define LeakSani_hpp

#include <forward_list>
#include <map>
#include <mutex>
#include <optional>
#include <ostream>
#include <utility>
#include <vector>

#include "MallocInfo.hpp"

#include "initialization/init.hpp"
#include "statistics/Stats.hpp"

#include "../include/lsan_internals.h"

namespace lsan {
/**
 * This class manages everything this sanitizer is capable to do.
 */
class LSan {
    /** A pair consisting of a boolean and an optional allocation record.               */
    using MallocInfoRemoved = std::pair<const bool, std::optional<std::reference_wrapper<const MallocInfo>>>;
    
    /** A map containing all allocation records, sorted by their allocated pointers.    */
    std::map<const void * const, MallocInfo> infos;
    /** An object holding all statistics.                                               */
    Stats                                    stats;
    /** Indicates whether the set callstack size has been exceeded during the printing. */
    bool                                     callstackSizeExceeded = false;
    /** The mutex used to synchronize the allocations and tracking.                     */
    std::recursive_mutex                     mutex;
    
    /** The runtime name of this sanitizer.                                             */
    const std::string libName;
    
public:
    LSan();
   ~LSan() {
        inited = false;
    }
    
    LSan(const LSan &)              = delete;
    LSan(const LSan &&)             = delete;
    LSan & operator=(const LSan &)  = delete;
    LSan & operator=(const LSan &&) = delete;
    
    /**
     * Returns the runtime library name of this sanitizer.
     *
     * @return the runtime library name
     */
    constexpr inline auto getLibName() const -> const std::string & {
        return libName;
    }
    
    /**
     * Returns the mutex for the allocations and tracking.
     *
     * @return the mutex
     */
    constexpr inline auto getMutex() -> std::recursive_mutex & {
        return mutex;
    }
    
    /**
     * @brief Attempts to exchange the allocation record associated with the given
     * allocation record by the given allocation record.
     *
     * @param info the allocation record to be exchanged
     * @return whether an allocation record was exchanged
     */
    auto changeMalloc(const MallocInfo & info) -> bool;
    
    /**
     * Removes the allocation record acossiated with the given pointer.
     *
     * @param pointer the allocation pointer
     * @param omitAddress the callstack frame delimiter
     * @return a pair with a boolean indicating the success and optionally the already deleted allocation record
     */
    auto removeMalloc(void * pointer,
                      void * omitAddress = __builtin_return_address(0)) -> MallocInfoRemoved;
    
    /**
     * Adds the given allocation record.
     *
     * @param info the allocation record to be added
     */
    void addMalloc(MallocInfo && info);
    
    /**
     * Calculates and returns the total count of allocated bytes that are stored inside the
     * principal list containing the allocation records.
     *
     * @return the total count of bytes found in the principal list
     */
    auto getTotalAllocatedBytes() -> std::size_t;
    
    /**
     * Calculates the relevant leak information.
     *
     * @return a tuple containing the count of the leaks, the amount of leaked bytes and
     * and a list with the leaked allocation records
     */
    auto getLeakNumbers() -> std::tuple<std::size_t, std::size_t, std::forward_list<std::reference_wrapper<const MallocInfo>>>;
    
    /**
     * Returns the globally tracked allocations.
     *
     * @return the globally tracked allocations
     */
    constexpr inline auto getFragmentationInfos() const -> const std::map<const void * const, MallocInfo> & {
        return infos;
    }
    
    /**
     * Sets whether the maximum callstack size has been exceeded during the printing.
     *
     * @param exceeded whether the maximum callstack size has been exceeded
     */
    constexpr inline void setCallstackSizeExceeded(bool exceeded) {
        callstackSizeExceeded = exceeded;
    }
    
    /**
     * Returns the current instance of the statistics object.
     *
     * @return the current statistics instance
     */
    constexpr inline auto getStats() -> const Stats & {
        return stats;
    }

    friend std::ostream & operator<<(std::ostream &, LSan &);
};
}

#endif /* LeakSani_hpp */

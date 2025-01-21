/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2025  mhahnFr
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

#ifndef MallocInfo_hpp
#define MallocInfo_hpp

#include <chrono>
#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <callstack.h>

#include "LeakType.hpp"

#include "callstacks/callstackHelper.hpp"

namespace lsan {
/**
 * @brief This class acts as a allocation record: all information about the allocation process
 * that is available is stored.
 *
 * It features a callstack and the file name and the line number of the allocation. The size and the
 * pointer are stored as well.
 */
struct MallocInfo {
    /** The preferred reference type of this class.          */
    using Ref = std::reference_wrapper<MallocInfo>;
    /** The preferred constant reference type of this class. */
    using CRef = std::reference_wrapper<const MallocInfo>;

    /** The pointer to the allocated piece of memory.             */
    void* pointer;
    /** The size of the allocated piece of memory.                */
    std::size_t size;
    
    /** The type of leak this record has been classified as.      */
    LeakType leakType = LeakType::unclassified;
    /** The allocation records reachable via this record.         */
    std::vector<Ref> viaMeRecords;

    /** Indicating whether this allocation has been deallocated.  */
    bool deleted = false;
    /** The timestamp when this record was freed.                 */
    std::optional<std::chrono::system_clock::time_point> freeTimestamp;
    /** The callstack where this allocation happened.             */
    mutable lcs::callstack createdCallstack;
    /** The callstack where the deallocation happened.            */
    mutable std::optional<lcs::callstack> deletedCallstack;
    std::pair<const char*, const char*> imageName = { nullptr, nullptr };
    std::thread::id threadId;

    bool printedInRoot = false;
    bool suppressed = false;
    bool enumerated = false;

    /**
     * Initializes this allocation record using the given information.
     *
     * @param pointer the pointer to the allocated piece of memory
     * @param size the size of the allocated piece of memory
     */
    inline MallocInfo(void* const pointer, const std::size_t size, const std::thread::id& id = std::this_thread::get_id()): pointer(pointer), size(size), threadId(id) {}

    /**
     * @brief Marks this allocation record as deleted.
     *
     * Creates a callstack of the point this function is called.
     */
    inline void markDeleted() {
        deleted = true;
        deletedCallstack = lcs::callstack();
        freeTimestamp = std::chrono::system_clock::now();
    }
    
    /**
     * Returns whether this allocation record was freed more recently than the given one.
     *
     * @param other the allocation record to compare to
     * @return whether this record was freed more recently
     */
    constexpr inline auto isMoreRecent(const MallocInfo& other) const -> bool {
        if (!freeTimestamp || !other.freeTimestamp) {
            return false;
        }
        return freeTimestamp > other.freeTimestamp;
    }

    /**
     * Prints the callstack where this allocation happened.
     *
     * @param out the output stream to print to
     */
    inline void printCreatedCallstack(std::ostream& out, const std::string& indent = "") const {
        callstackHelper::format(createdCallstack, out, indent);
    }
    /**
     * Prints the callstack where this allocation was deallocated.
     *
     * @param out the output stream to print to
     */
    inline void printDeletedCallstack(std::ostream& out) const {
        if (!deletedCallstack) {
            throw std::runtime_error("MallocInfo: No deleted callstack! "
                                     "Hint: Check using MallocInfo::getDeletedCallstack()::has_value().");
        }
        
        callstackHelper::format(*deletedCallstack, out);
    }

    void markSuppressed();
    auto enumerate() -> std::pair<std::size_t, std::size_t>;

    friend auto operator<<(std::ostream&, const MallocInfo&) -> std::ostream&;

private:
    bool flag = false;

    void print(std::ostream& stream, unsigned long indent = 0, unsigned long number = 0, unsigned long indent2 = 0) const;

    template<typename F, typename... Args>
    constexpr inline void forEachIndirect(bool mark, F func, Args... args) const;
};
}

#endif /* MallocInfo_hpp */

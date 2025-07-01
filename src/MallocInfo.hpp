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
#include <string>
#include <utility>
#include <vector>

#include <callstack.h>

#include "LeakType.hpp"
#include "callstacks/callstackHelper.hpp"

namespace lsan {
/**
 * This class acts as an allocation record: all information about the allocation
 * process that is available is stored.
 */
struct MallocInfo {
    /** The preferred reference type of this class.                         */
    using Ref = std::reference_wrapper<MallocInfo>;
    /** The preferred constant reference type of this class.                */
    using CRef = std::reference_wrapper<const MallocInfo>;

    /** The type of leak this record has been classified as.                */
    LeakType leakType = LeakType::unclassified;
    /** The allocation records reachable via this record.                   */
    std::vector<Ref> viaMeRecords;

    /** Indicates whether this record has been printed as root leak.        */
    bool printedInRoot = false;
    /** Indicates whether this record is suppressed.                        */
    bool suppressed = false;
    /** Indicates whether this record has been enumerated.                  */
    bool enumerated = false;
    /** The absolute and relative image name this record has been found in. */
    std::pair<const char*, const char*> imageName = { nullptr, nullptr };

    /**
     * Initializes this allocation record using the given information.
     *
     * @param pointer the pointer to the allocated piece of memory
     * @param size the size of the allocated piece of memory
     * @param id the thread number that created the allocation
     */
    inline MallocInfo(void* const pointer, const std::size_t size, const unsigned long id = getThreadId()):
        pointer(pointer), size(size), threadId(id) {}

    /**
     * @brief Marks this allocation record as deleted.
     *
     * Creates a callstack of the point where this function is called.
     */
    inline void markDeleted() {
        deleted = true;
        deletedCallstack = lcs::callstack();
        freeTimestamp = std::chrono::system_clock::now();
        deletedId = getThreadId();
    }
    
    /**
     * Returns whether this allocation record was freed more recently than the
     * given one.
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
     * @param indent the indentation to use while printing this record
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

    /**
     * Returns the pointer to the allocated memory.
     *
     * @return the pointer to the allocated memory
     */
    constexpr inline auto getPointer() const {
        return pointer;
    }

    /**
     * Returns the size of the allocated memory block.
     *
     * @return the size of the allocated memory
     */
    constexpr inline auto getSize() const {
        return size;
    }

    /**
     * Returns whether the represented allocation is deallocated.
     *
     * @return whether the allocation is deallocated
     */
    constexpr inline auto isDeleted() const {
        return deleted;
    }

    /**
     * Returns the timestamp of the deallocation.
     *
     * @return the timestamp of the deallocation
     */
    constexpr inline auto getFreeTimestamp() const -> const auto& {
        return freeTimestamp;
    }

    /**
     * Returns the number of the thread that created the represented allocation.
     *
     * @return the number of the thread
     */
    constexpr inline auto getAllocationThread() const {
        return threadId;
    }

    /**
     * Returns the number of the thread that performed the deallocation.
     *
     * @return the number of the thread
     */
    constexpr inline auto getDeallocationThread() const {
        return deletedId;
    }

    /**
     * Returns the callstack of the deallocation.
     *
     * @return the deallocation callstack
     */
    constexpr inline auto getDeallocationCallstack() const -> const auto& {
        return deletedCallstack;
    }

    /**
     * Returns the callstack of the allocation.
     *
     * @return the allocation callstack
     */
    constexpr inline auto getAllocationCallstack() const -> auto& {
        return createdCallstack;
    }

    /**
     * Marks this allocation record as suppressed.
     */
    void markSuppressed();

    /**
     * Enumerates the represented memory leak and its descendants.
     *
     * @return the amount of descendants and the byte amount
     */
    auto enumerate() -> std::pair<std::size_t, std::size_t>;

    friend auto operator<<(std::ostream&, const MallocInfo&) -> std::ostream&;

private:
    /** Flag used to deduplicate the memory leaks.              */
    bool flag = false;
    /** The pointer to the allocated piece of memory.           */
    void* pointer;
    /** The size of the allocated piece of memory.              */
    std::size_t size;
    /** Indicates whether this allocation has been deallocated. */
    bool deleted = false;
    /** The timestamp when this record was freed.               */
    std::optional<std::chrono::system_clock::time_point> freeTimestamp;
    /** The thread number that created this record.             */
    unsigned long threadId,
    /** The thread number that performed the deallocation.      */
                  deletedId = 0;
    /** The callstack where the allocation happened.            */
    mutable lcs::callstack createdCallstack;
    /** The callstack where the deallocation happened.          */
    mutable std::optional<lcs::callstack> deletedCallstack;

    /**
     * @brief Prints this allocation record onto the given stream.
     *
     * It is printed as a descendant if the given number is bigger than @c 0 .
     *
     * @param stream the output stream to print onto
     * @param indent the indentation to use for this record
     * @param number the number of this leak within a number of leaks printed
     * @param indent2 the indentation of the previous printed leak
     */
    void print(std::ostream& stream, unsigned long indent = 0, unsigned long number = 0, unsigned long indent2 = 0) const;

    /**
     * Calls the given function for each indirect leak.
     *
     * @tparam F the type of the function to be called
     * @tparam Args the arguments to pass to the function
     * @param mark whether to mark the property @c printedInRoot
     * @param func the function to call
     * @param args the arguments to pass to the given function
     */
    template<typename F, typename... Args>
    constexpr inline void forEachIndirect(bool mark, F func, Args... args) const;

    /**
     * Returns the number of the calling thread.
     *
     * @return the thread number of the calling thread
     */
    static auto getThreadId() -> unsigned long;
};
}

#endif /* MallocInfo_hpp */

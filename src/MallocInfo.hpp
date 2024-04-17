/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2024  mhahnFr
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

#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "LeakType.hpp"

#include "callstacks/callstackHelper.hpp"

#include "../CallstackLibrary/include/callstack.h"

namespace lsan {
/**
 * @brief This class acts as a allocation record: all information about the allocation process
 * that is available is stored.
 *
 * It features a callstack and the file name and the line number of the allocation. The size and the
 * pointer are stored as well.
 */
class MallocInfo {
public:
    using Ref = std::reference_wrapper<MallocInfo>;
    using CRef = std::reference_wrapper<const MallocInfo>;

private:
    /** The pointer to the allocated piece of memory.             */
    void * pointer;
    /** The size of the allocated piece of memory.                */
    std::size_t size;
    
    LeakType leakType = LeakType::unclassified;
    std::vector<Ref> viaMeRecords;
    
    /** The filename in which this allocation happened.           */
    std::optional<std::string> createdInFile;
    /** The line number in which this allocation happened.        */
    std::optional<int>         createdOnLine;
    /** The callstack where this allocation happened.             */
    mutable lcs::callstack     createdCallstack;
    
    /** The filename in which this allocation was deallocated.    */
    std::optional<std::string>            deletedInFile;
    /** The line number in which this allocation was deallocated. */
    std::optional<int>                    deletedOnLine;
    /** Indicating whether this allocation has been deallocated.  */
    bool                                  deleted;
    /** The callstack where the deallocation happened.            */
    mutable std::optional<lcs::callstack> deletedCallstack;
    
public:
    /**
     * Initializes this allocation record using the given information.
     *
     * @param pointer the pointer to the allocated piece of memory
     * @param size the size of the allocated piece of memory
     * @param file the filename where the allocation happened
     * @param line the line number inside the file where the allocation happened
     */
    inline MallocInfo(void * const                     pointer,
                      const std::size_t                size,
                      std::optional<const std::string> file,
                      std::optional<const int>         line)
        : pointer(pointer),
          size(size),
          createdInFile(file),
          createdOnLine(line),
          createdCallstack(),
          deletedInFile(std::nullopt),
          deletedOnLine(std::nullopt),
          deleted(false),
          deletedCallstack(std::nullopt)
    {}
    
    /**
     * Initializes this allocation record using the given information.
     *
     * @param pointer the pointer to the allocated piece of memory
     * @param size the size of the allocated piece of memory
     */
    inline MallocInfo(void * const pointer, std::size_t size)
        : MallocInfo(pointer, size, std::nullopt, std::nullopt)
    {}
    
    /**
     * Returns the pointer to the allocated piece of memory.
     *
     * @return the pointer to the allocated memory
     */
    constexpr inline auto getPointer() const -> const void * {
        return pointer;
    }
    /**
     * @brief Returns the filename where the allocation happened.
     *
     * @return the filename where the allocation happened.
     */
    constexpr inline auto getCreatedInFile() const -> const std::optional<std::string> & {
        return createdInFile;
    }
    /**
     * @brief Returns the line number where the allocation happend.
     *
     * @return the line number where the allocation happend
     */
    constexpr inline auto getCreatedOnLine() const -> std::optional<int> {
        return createdOnLine;
    }
    /**
     * Returns the size of the allocated piece of memory.
     *
     * @return the size of the allocated memory block
     */
    constexpr inline auto getSize() const -> std::size_t {
        return size;
    }
    
    /**
     * Sets the filename in which this allocation was deallocated.
     *
     * @param file the filename
     */
    inline void setDeletedInFile(const std::string & file) {
        deletedInFile = file;
    }
    /**
     * Returns the filename where this allocation was deallocated.
     *
     * @return the filename where the deallocation happened
     */
    constexpr inline auto getDeletedInFile() const -> const std::optional<std::string> & {
        return deletedInFile;
    }
    
    /**
     * Sets the line number where this allocation was deallocated.
     *
     * @param line the line number
     */
    constexpr inline void setDeletedOnLine(int line) {
        deletedOnLine = line;
    }
    /**
     * Returns the line number where this allocation was deallocated.
     *
     * @return the line number where the deallocation happened
     */
    constexpr inline auto getDeletedOnLine() const -> std::optional<int> {
        return deletedOnLine;
    }
    
    /**
     * Returns whether this allocation has been deallocated.
     *
     * @return whether this allocation is marked as deallocated
     */
    constexpr inline auto isDeleted() const -> bool {
        return deleted;
    }
    /**
     * Sets whether this alloction has been deallocated.
     *
     * @param deleted whether this allocation is deallocated
     */
    inline void setDeleted(bool deleted) {
        this->deleted = deleted;
        
        deletedCallstack = lcs::callstack();
    }
    
    /**
     * Prints the callstack where this allocation happened.
     *
     * @param out the output stream to print to
     */
    inline void printCreatedCallstack(std::ostream & out) const {
        callstackHelper::format(createdCallstack, out);
    }
    /**
     * Prints the callstack where this allocation was deallocated.
     *
     * @param out the output stream to print to
     */
    inline void printDeletedCallstack(std::ostream & out) const {
        if (!deletedCallstack.has_value()) {
            throw std::runtime_error("MallocInfo: No deleted callstack! "
                                     "Hint: Check using MallocInfo::getDeletedCallstack()::has_value().");
        }
        
        callstackHelper::format(deletedCallstack.value(), out);
    }
    
    /**
     * Returns a reference to the callstack where this allocation was deallocated.
     *
     * @return the callstack where the deallocation happened
     */
    constexpr inline auto getDeletedCallstack() const -> const std::optional<lcs::callstack> & {
        return deletedCallstack;
    }
    /**
     * Returns the optionally callstack where the represented allocation
     * has been deallocated.
     *
     * @return the deleted callstack
     */
    inline auto getDeletedCallstack() -> std::optional<lcs::callstack> & {
        return deletedCallstack;
    }
    /**
     * Returns a reference to the callstack where this allocation was allocated.
     *
     * @return the callstack where the allocation happened
     */
    constexpr inline auto getCreatedCallstack() const -> const lcs::callstack & {
        return createdCallstack;
    }
    /**
     * Returns a reference to the callstack where the represented allocation was allocated.
     *
     * @return the callstack where the allocation happened
     */
    inline auto getCreatedCallstack() -> lcs::callstack & {
        return createdCallstack;
    }
    
    constexpr inline auto getLeakType() const noexcept -> LeakType {
        return leakType;
    }
    
    constexpr inline void setLeakType(LeakType type) noexcept {
        leakType = type;
    }
    
    constexpr inline void addViaMeReachable(MallocInfo& info) {
        viaMeRecords.push_back(info);
    }
    
    constexpr inline auto getViaMeReachables() const -> const std::vector<Ref>& {
        return viaMeRecords;
    }
    
    friend auto operator<<(std::ostream &, const MallocInfo &) -> std::ostream &;
};
}

#endif /* MallocInfo_hpp */

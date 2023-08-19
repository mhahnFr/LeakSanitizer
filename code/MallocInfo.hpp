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

#ifndef MallocInfo_hpp
#define MallocInfo_hpp

#include <ostream>
#include <string>
#include <utility>

#include "../CallstackLibrary/include/callstack.h"

using namespace std::rel_ops;

/**
 * @brief This class acts as a allocation record: all information about the allocation process
 * that is available is stored.
 *
 * It features a callstack and the file name and the line number of the allocation. The size and the
 * pointer are stored as well.
 */
class MallocInfo {
    /**
     * Initializes this allocation record using the given information.
     *
     * @param pointer the pointer to the allocated piece of memory
     * @param size the size of the allocated piece of memory
     * @param file the filename where the allocation happened
     * @param line the line number inside the file where the alocation happened
     * @param omitAddress the return address upon which function calls are ignored
     * @param createdSet whether the file and line information are actually set
     */
    inline MallocInfo(void * const        pointer,
                      std::size_t         size,
                      const std::string & file,
                      const int           line,
                      void *              omitAddress,
                      bool                createdSet):
        pointer(pointer),
        size(size),
        createdInFile(file),
        createdOnLine(line),
        createdSet(createdSet),
        createdCallstack(omitAddress),
        deletedOnLine(0),
        deleted(false),
        deletedCallstack(false)
    {}
    
    /// The pointer to the allocated piece of memory.
    void * pointer;
    /// The size of the allocated piece of memory.
    std::size_t size;
    
    /// The filename in which this allocation happened.
    std::string            createdInFile;
    /// The line number in which this allocation happened.
    int                    createdOnLine;
    /// Indicating whether the file and the line information have been set.
    bool                   createdSet;
    /// The callstack where this allocation happened.
    mutable lcs::callstack createdCallstack;

    /// The filename in which this allocation was deallocated.
    std::string            deletedInFile;
    /// The line number in which this allocation was deallocated.
    int                    deletedOnLine;
    /// Indicating whether this allocation has been deallocated.
    bool                   deleted;
    /// The callstack where the deallocation happened.
    mutable lcs::callstack deletedCallstack;
        
public:
    /**
     * Initializes this allocation record using the given information.
     *
     * @param pointer the pointer to the allocated piece of memory
     * @param size the size of the allocated piece of memory
     * @param omitAddress the return address upon which function calls are ignored
     */
    inline MallocInfo(void * const pointer, std::size_t size, void * omitAddress = __builtin_return_address(0))
        : MallocInfo(pointer, size, "<Unknown>", 1, omitAddress, false)
    {}
    
    /**
     * Initializes this allocation record using the given information.
     *
     * @param pointer the pointer to the allocated piece of memory
     * @param size the size of the allocated piece of memory
     * @param file the filename where the allocation happened
     * @param line the line number inside the file where the allocation happened
     * @param omitAddress the return address upon which function calls are ignored
     */
    inline MallocInfo(void * const        pointer,
                      std::size_t         size,
                      const std::string & file,
                      int                 line,
                      void *              omitAddress = __builtin_return_address(0))
        : MallocInfo(pointer, size, file, line, omitAddress, true)
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
    constexpr inline auto getCreatedInFile() const -> const std::string & {
        return createdInFile;
    }
    /**
     * @brief Returns the line number where the allocation happend.
     *
     * @return the line number where the allocation happend
     */
    constexpr inline auto getCreatedOnLine() const -> int {
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
    constexpr inline void setDeletedInFile(const std::string & file) {
        deletedInFile = file;
    }
    /**
     * Returns the filename where this allocation was deallocated.
     *
     * @return the filename where the deallocation happened
     */
    constexpr inline auto getDeletedInFile() const -> const std::string & {
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
    constexpr inline auto getDeletedOnLine() const -> int {
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
    constexpr inline void setDeleted(bool deleted) {
        this->deleted = deleted;
    }
    
    /**
     * Generates and stores a callstack for the deallocation callstack.
     */
    inline void generateDeletedCallstack() {
        callstack_emplaceWithAddress(deletedCallstack, __builtin_return_address(0));
    }
    
    /**
     * Prints the callstack where this allocation happened.
     *
     * @param out the output stream to print to
     */
    inline void printCreatedCallstack(std::ostream & out) const {
        printCallstack(createdCallstack, out);
    }
    /**
     * Prints the callstack where this allocation was deallocated.
     *
     * @param out the output stream to print to
     */
    inline void printDeletedCallstack(std::ostream & out) const {
        printCallstack(deletedCallstack, out);
    }
    
    /**
     * Returns a reference to the callstack where this allocation was deallocated.
     *
     * @return the callstack where the deallocation happened
     */
    constexpr inline auto getDeletedCallstack() const -> const lcs::callstack & {
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
     * Prints the given callstack formatted on the given output stream.
     *
     * @param callstack the callstack to be printed
     * @param out the output stream to print to
     */
    static void printCallstack(lcs::callstack & callstack,  std::ostream & out);
    /**
     * Prints the given callstack formatted on the given output stream.
     *
     * @param callstack the callstack to be printed
     * @param out the output stream to print to
     */
    static inline void printCallstack(lcs::callstack && callstack, std::ostream & out) {
        printCallstack(callstack, out);
    }
    
    friend auto operator==(const MallocInfo &, const MallocInfo &) -> bool;
    friend auto operator<(const MallocInfo &, const MallocInfo &)  -> bool;
    friend auto operator<<(std::ostream &, const MallocInfo &)     -> std::ostream &;
};

#endif /* MallocInfo_hpp */

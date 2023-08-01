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

#include <map>
#include <mutex>
#include <ostream>
#include <vector>

#include "MallocInfo.hpp"
#include "Stats.hpp"
#include "threadAllocInfo/ThreadAllocInfo.hpp"

/**
 * This class manages everything this sanitizer is capable to do.
 */
class LSan {
    /// A pointer to the statistics instance held by each instance.
    static Stats * stats;
    
    /**
     * Statically holds a boolean value which idicates whether to track allocations
     * or not at the moment of querying.
     *
     * @return a reference to the statically stored boolean value
     */
    static auto getIgnoreMalloc() -> bool &;
    
    /// A map containing all allocation records, sorted by their allocated pointers.
    std::map<const void * const, MallocInfo> infos;
    /// The mutex used to protect the principal map.
    std::recursive_mutex                     infoMutex;
    /// An object holding all statistics.
    Stats                                    realStats;
    /// Indicates whether the set callstack size had been exceeded during the printing.
    bool                                     callstackSizeExceeded = false;
    
    std::vector<ThreadAllocInfo::CRef> threadInfos;
    
public:
    /// Constructs the sanitizer manager. Initializes all variables and sets up the hooks and signal handlers.
    LSan();
   ~LSan() = default;
    
    LSan(const LSan &)              = delete;
    LSan(const LSan &&)             = delete;
    LSan & operator=(const LSan &)  = delete;
    LSan & operator=(const LSan &&) = delete;
    
    /**
     * Adds the given allocation record to the principle list and to the statistics.
     *
     * @param info the allocation record to add
     */
    void addMalloc(MallocInfo && info);
    /**
     * Exchanges the allocation record associated with the pointer of the given allocation record
     * by the given allocation record.
     *
     * @param info the allocation record to exchange
     */
    void changeMalloc(const MallocInfo & info);
    /**
     * @brief Attempts to remove the given allocation record from the principal list.
     *
     * The allocation record is removed from the statistics and marked as deallocated.
     * It is removed from the principal list if the fragmentation tracking is turned off.
     *
     * @param info the info to mark as deallocated
     * @return whether the allocation record was found in the principal list
     */
    auto removeMalloc(const MallocInfo & info) -> bool;
    /**
     * @brief Attempts to remove the allocation record associated with the given pointer.
     *
     * The allocation record is removed from the statistics and marked as deallocated if it
     * was found. It is not removed from the principal list if the fragmentation tracking is enabled.
     *
     * @param pointer the pointer whose associated allocation record should be marked as deallocated
     * @return whether an associated allocation record was found
     */
    auto removeMalloc(const void * pointer)    -> bool;
    
    /**
     * Calculates and returns the total count of allocated bytes that are stored inside the
     * principal list containing the allocation records.
     *
     * @return the total count of bytes found in the principal list
     */
    auto getTotalAllocatedBytes() -> size_t;
    /**
     * Calculates and returns the total count of bytes that are stored inside the principal
     * list and not marked as deallocated.
     *
     * @return the total count of leaked bytes
     */
    auto getTotalLeakedBytes()    -> size_t;
    /**
     * Calculates and returns the count of allocation records stored in the principal list
     * that are not marked as deallocated.
     *
     * @return the total count of leaked objects
     */
    auto getLeakCount()           -> size_t;
    /**
     * Returns a reference to the mutex used to protect the principal list storing the
     * allocation records.
     *
     * @return the mutex protecting the principal list
     */
    auto getInfoMutex()           -> std::recursive_mutex &;
    /**
     * Returns a reference to the principal list storing the allocation records.
     *
     * @return the principal list of allocation records
     */
    auto getInfos()         const -> const std::map<const void * const, MallocInfo> &;
    
    /**
     * Sets whether the maximum callstack size has been exceeded during the printing.
     *
     * @param exceeded whether the maximum callstack size has been exceeded
     */
    void setCallstackSizeExceeded(bool exceeded);
    
    void registerThreadAllocInfo(ThreadAllocInfo::CRef info);
    void removeThreadAllocInfo(ThreadAllocInfo::Ref info);
    
    /// A pointer to the real `malloc` function.
    static void * (*malloc) (size_t);
    /// A pointer to the real `calloc` function.
    static void * (*calloc) (size_t, size_t);
    /// A pointer to the real `realloc` function.
    static void * (*realloc)(void *, size_t);
    /// A pointer to the real `free` function.
    static void   (*free)   (void *);
    /// A pointer to the real `exit` function.
    static void   (*exit)   (int);
    
    /**
     * Returns the current instance of this sanitizer.
     *
     * @return the current instance
     */
    static auto getInstance()  -> LSan &;
    static auto getLocalInstance() -> ThreadAllocInfo &;
    /**
     * Returns the current instance of the statitcs object.
     *
     * @return the current statistics instance
     */
    static auto getStats()     -> Stats &;
    /**
     * Returns whether the statictics are available.
     *
     * @return whether the statistics are available
     */
    static auto hasStats()     -> bool;
    /**
     * Returns whether the allocations should be tracked at the time of querying.
     *
     * @return whether the allocations should be tracked at the moment
     */
    static auto ignoreMalloc() -> bool;
    
    /**
     * Sets whether the next allocations should be tracked.
     *
     * @param ignore whether to ignore the next allocations
     */
    static void setIgnoreMalloc(bool ignore);
    /**
     * Prints the informations of this sanitizer.
     */
    static void printInformations();
    /**
     * Prints the license information of this sanitizer.
     */
    static void printLicense();
    /**
     * Prints the link to the website of this sanitizer.
     */
    static void printWebsite();
    /**
     * @brief The hook to be called on exit.
     *
     * It prints all informations tracked by the sanitizer and performs internal cleaning.
     */
    static void __exit_hook();

    friend void           internalCleanUp();
    friend std::ostream & operator<<(std::ostream &, LSan &);
};

/**
 * Deletes the currently active instance of the sanitizer.
 */
void internalCleanUp();

#endif /* LeakSani_hpp */

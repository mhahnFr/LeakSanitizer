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

#include "ATracker.hpp"
#include "MallocInfo.hpp"

#include "statistics/Stats.hpp"
#include "threadAllocInfo/ThreadAllocInfo.hpp"

#include "../include/lsan_internals.h"

/**
 * This class manages everything this sanitizer is capable to do.
 */
class LSan: public ATracker {
    /// A map containing all allocation records, sorted by their allocated pointers.
    std::map<const void * const, MallocInfo> infos;
    /// The mutex used to protect the principal map.
    std::recursive_mutex                     infoMutex;
    /// An object holding all statistics.
    Stats                                    stats;
    /// Indicates whether the set callstack size had been exceeded during the printing.
    bool                                     callstackSizeExceeded = false;
    /** A vector holding the thread local tracker instances. */
    std::vector<ThreadAllocInfo::Ref>        threadInfos;
    
    /**
     * Attempts to remove an allocation in this instance.
     *
     * @param pointer the deallocated pointer
     * @return whether an alloaction info was removed
     */
    auto removeMallocHere(const void * pointer)    -> bool;
    /**
     * Attempts to exchange the allocation record associated with the given
     * allocation info by the given allocation record.
     *
     * @param info the allocation record
     * @return whether an allocation record was replaced
     */
    auto changeMallocHere(const MallocInfo & info) -> bool;
    
    /**
     * Returns a reference to a thread local boolean value indicating
     * whether to ignore allocations.
     *
     * @return the ignoration flag
     */
    static auto getLocalIgnoreMalloc() -> bool &;
    
public:
    /// Constructs the sanitizer manager. Initializes all variables and sets up the hooks and signal handlers.
    LSan();
   ~LSan() = default;
    
    LSan(const LSan &)              = delete;
    LSan(const LSan &&)             = delete;
    LSan & operator=(const LSan &)  = delete;
    LSan & operator=(const LSan &&) = delete;
    
    /**
     * @brief Attempts to exchange the allocation record associated with the given
     * allocation record by the given allocation record.
     *
     * The allocation record is searched for globally and in all running threads.
     *
     * @param info the allocation record to be exchanged
     * @return whether an allocation record was exchanged
     */
    auto maybeChangeMalloc(const MallocInfo & info) -> bool;
    
    virtual auto removeMalloc(const void * pointer) -> bool override;
    
    virtual void addMalloc(MallocInfo && info) override;
    
    virtual inline auto changeMalloc(const MallocInfo & info) -> bool override {
        return maybeChangeMalloc(info);
    }
    
    virtual inline void setIgnoreMalloc(const bool ignoreMalloc) override {
        getLocalIgnoreMalloc() = ignoreMalloc;
    }
    
    virtual inline auto getIgnoreMalloc() const -> bool override {
        return getLocalIgnoreMalloc();
    }
    
    /**
     * Calculates and returns the total count of allocated bytes that are stored inside the
     * principal list containing the allocation records.
     *
     * @return the total count of bytes found in the principal list
     */
    auto getTotalAllocatedBytes() -> std::size_t;
    /**
     * Calculates and returns the total count of bytes that are stored inside the principal
     * list and not marked as deallocated.
     *
     * @return the total count of leaked bytes
     */
    auto getTotalLeakedBytes() -> std::size_t;
    /**
     * Calculates and returns the count of allocation records stored in the principal list
     * that are not marked as deallocated.
     *
     * @return the total count of leaked objects
     */
    auto getLeakCount() -> std::size_t;
    
    /**
     * Returns the globally tracked allocations.
     *
     * @return the globally tracked allocations
     */
    constexpr inline auto getFragmentationInfos() const -> const std::map<const void * const, MallocInfo> & {
        return infos;
    }
    
    /**
     * Returns the mutex used to protect the global allocation container.
     *
     * @return the mutex
     */
    constexpr inline auto getFragmentationInfoMutex() -> std::recursive_mutex & {
        return infoMutex;
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
     * Registers the given thread local allocation tracker instance.
     *
     * @param info the tracker to be registered
     */
    void registerThreadAllocInfo(ThreadAllocInfo::Ref info);
    /**
     * Removes the given thread local allocation tracker instance.
     *
     * @param info the tracker to be removed
     */
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
    static auto getInstance()      -> LSan &;
    /**
     * Returns the thread local allocation tracker instance.
     *
     * @return the allocation tracker associated with the calling thread
     */
    static auto getLocalInstance() -> ThreadAllocInfo &;

    /**
     * Returns a reference to the tracker used to track allocations
     * for the calling thread.
     *
     * @return the appropriate tracker
     */
    static inline auto getTracker() -> ATracker & {
        if (__lsan_statsActive) {
            return getInstance();
        }
        return getLocalInstance();
    }
    /**
     * Returns the current instance of the statitcs object.
     *
     * @return the current statistics instance
     */
    inline static auto getStats() -> const Stats & {
        return getInstance().stats;
    }
    
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

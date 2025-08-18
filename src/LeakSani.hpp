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

#ifndef LeakSani_hpp
#define LeakSani_hpp

#include <atomic>
#include <map>
#include <mutex>
#include <optional>
#include <pthread.h>
#include <regex>
#include <set>
#include <utility>
#include <vector>

#include "MallocInfo.hpp"
#include "ThreadInfo.hpp"

#ifdef BENCHMARK
# include "timing.hpp"
#endif

#include "behaviour/Behaviour.hpp"
#include "helpers/LeakKindStats.hpp"
#include "statistics/Stats.hpp"
#include "suppression/Suppression.hpp"
#include "trackers/ATracker.hpp"
#include "wrappers/realAlloc.hpp"

namespace lsan {
/**
 * @brief This class manages everything this sanitizer is capable to do.
 *
 * It acts as an allocation tracker.
 */
class LSan final: public trackers::ATracker {
    /** Maps the known thread identifiers to their information structure.               */
    std::map<std::thread::id, ThreadInfo> threads;
    /** An object holding all statistics.                                               */
    Stats stats;
    /** The behaviour handling object.                                                  */
    behaviour::Behaviour behaviour;
    /** Indicates whether the set callstack size has been exceeded during the printing. */
    bool callstackSizeExceeded = false;
    /** The general suppressions.                                                       */
    std::optional<std::vector<suppression::Suppression>> suppressions;
    /** The system library regular expressions.                                         */
    std::optional<std::vector<std::regex>> systemLibraries;
    /** Maps the thread numbers to their description.                                   */
    std::map<unsigned long, std::string> threadDescriptions;
    /** The registered thread-local allocation trackers.                                */
    std::set<ATracker*> tlsTrackers;
    /** The mutex to manage the access to the registered thread-local trackers.         */
    std::mutex tlsTrackerMutex;
    /** Indicates whether multithreading was used.                                      */
    bool isThreaded = false;
    bool alwaysEqual = false;
    /** The thread identifier of the main thread.                                       */
    const std::thread::id mainId = std::this_thread::get_id();
    /** The thread-local storage key used for the thread-local allocation trackers.     */
    const pthread_key_t saniKey;

#ifdef BENCHMARK
    /** The registered timings of the allocations.                                      */
    std::map<timing::AllocType, timing::Timings> timingMap;
    /** The mutex to manage access to the allocation timings.                           */
    std::mutex timingMutex;
#endif

    /**
     * Classifies all memory leaks.
     *
     * @return the aggregated leak information
     */
    auto classifyLeaks() -> LeakKindStats;

    /**
     * Classifies the given allocation record.
     *
     * @param info        the allocation record to be classified
     * @param currentType the leak type to be used
     * @param reclassify  whether to reclassify classified records
     */
    void classifyRecord(MallocInfo& info, const LeakType& currentType, bool reclassify = false);

    /**
     * Searches the Objective-C runtime for memory leaks and places their
     * allocation records into the given list.
     *
     * @param records the place to store related allocation records in
     */
    void classifyObjC(std::deque<MallocInfo::Ref>& records);

    /**
     * Creates a thread-safe copy of the thread-local tracker list.
     *
     * @return the copy
     */
    auto copyTrackerList() -> decltype(tlsTrackers);

    /**
     * Searches for the allocation record referred to by the given pointer,
     * applying pointer demasking if necessary.
     *
     * @param ptr the pointer
     * @return the iterator to the found allocation record
     */
    auto findWithSpecials(void* ptr) -> decltype(infos)::iterator;

    /**
     * Classifies the memory leaks found in the given memory range.
     *
     * @param begin           the pointer to the beginning of the memory
     * @param end             the pointer to the end of the memory
     * @param direct          the leak type of leaks found in the given memory range
     * @param indirect        the type of leaks found via direct leaks
     * @param directs         the list to place direct memory leaks in
     * @param skipClassifieds whether to skip already classified records
     * @param name            the absolute name of the memory range
     * @param nameRelative    the relative name of the memory range
     * @param reclassify      whether to reclassify already classified records
     */
    void classifyLeaks(uintptr_t begin, uintptr_t end,
                       LeakType direct, LeakType indirect,
                       std::deque<MallocInfo::Ref>& directs, bool skipClassifieds = false,
                       const char* name = nullptr, const char* nameRelative = nullptr, bool reclassify = false);

    /**
     * Classifies a pointer used as union of multiple pointers.
     *
     * @tparam Four whether the pointer union can hold four pointers
     * @param ptr      the pointer union
     * @param directs  the list to place direct memory leaks in
     * @param direct   the direct leak type
     * @param indirect the indirect leak type
     */
    template<bool Four = false>
    constexpr inline void classifyPointerUnion(void* ptr, std::deque<MallocInfo::Ref>& directs,
                                               const LeakType direct, const LeakType indirect) {
        constexpr auto order = Four ? 3 : 1;

        if (const auto& it = infos.find(reinterpret_cast<void*>(uintptr_t(ptr) & ~order));
            it != infos.end() && it->second.leakType > direct) {
            it->second.leakType = direct;
            classifyRecord(it->second, indirect);
            directs.push_back(it->second);
        }
    }

    /**
     * Classifies the given Objective-C class.
     *
     * @param cls      the pointer to the Objective-C class
     * @param directs  the list to place direct memory leaks in
     * @param direct   the direct leak type
     * @param indirect the indirect leak type
     */
    void classifyClass(void* cls, std::deque<MallocInfo::Ref>& directs, LeakType direct, LeakType indirect);

    /**
     * Returns whether the given allocation record should be suppressed.
     *
     * @param info the allocation record in question
     * @return whether the allocation record should be suppressed
     */
    auto isSuppressed(const MallocInfo& info) -> bool;

#ifdef __linux__
    /**
     * Gathers and returns the size of the memory referred to by @c pthread_t .
     *
     * @return the size of the POSIX thread structure
     */
    auto gatherPthreadSize() -> std::size_t;
#endif

protected:
    /**
     * Adds the given allocation record to the statistics if they are active.
     *
     * @param info the allocation record
     */
    inline void maybeAddToStats(const MallocInfo& info) override {
        if (behaviour.statsActive()) {
            stats += info;
        }
    }

public:
    /** Indicates whether the allocation tracking has finished.           */
    static std::atomic_bool finished;
    /** Indicates whether to ignore deallocations in the TLS deallocator. */
    static std::atomic_bool preventDealloc;
    /** Whether the exit has already been printed.                        */
    bool hasPrintedExit = false;
    /** Whether indirect memory leaks have been found.                    */
    bool hadIndirects = false;

    LSan();
   ~LSan() override;

    LSan(const LSan&) = delete;
    LSan(LSan&&)      = delete;

    auto operator=(const LSan&) -> LSan& = delete;
    auto operator=(LSan&&)      -> LSan& = delete;

    inline auto operator new(const std::size_t count) -> void* {
        return real::malloc(count);
    }

    inline void operator delete(void* ptr) {
        real::free(ptr);
    }

    /**
     * @brief Attempts to remove the allocation record associated with the
     * given pointer.
     *
     * If no record is found in this instance, all registered trackers except
     * the given one are searched for the record.
     *
     * @param tracker the tracker to not be searched
     * @param pointer the pointer to the allocation
     * @return whether a record was removed and the potentially existing record
     */
    auto removeMalloc(const ATracker* tracker, void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>>;

    /**
     * @brief Replaces the allocation record with the given one.
     *
     * If no record is found in this instance, all registered trackers except
     * the given one are searched for the record.
     *
     * @param tracker the allocation tracker to not be searched
     * @param info the new allocation record
     */
    void changeMalloc(const ATracker* tracker, MallocInfo&& info);

    /**
     * Registers the given allocation tracker.
     *
     * @param tracker the allocation tracker to be registered
     */
    void registerTracker(ATracker* tracker);

    /**
     * Deregisters the given allocation tracker.
     *
     * @param tracker the allocation tracker to be deregistered
     */
    void deregisterTracker(ATracker* tracker);

    /**
     * Absorbs the given allocation records.
     *
     * @param leaks the memory leaks to absorb
     */
    void absorbLeaks(PoolMap<const void* const, MallocInfo>&& leaks);

    void finish() override;

#ifdef BENCHMARK
    /**
     * Returns the allocation timings.
     *
     * @return the allocation timings
     */
    constexpr inline auto getTimingMap() -> std::map<timing::AllocType, timing::Timings>& {
        return timingMap;
    }

    /**
     * Returns the timing mutex.
     *
     * @return the timing mutex
     */
    constexpr inline auto getTimingMutex() -> std::mutex& {
        return timingMutex;
    }
#endif

    /**
     * @brief Returns the global suppressions.
     *
     * They are loaded if necessary.
     *
     * @return the global suppressions
     */
    auto getSuppressions() -> const std::vector<suppression::Suppression>&;

    /**
     * @brief Returns the system library regular expressions.
     *
     * They are loaded if necessary.
     *
     * @return the system library regular expressions
     */
    auto getSystemLibraries() -> const std::vector<std::regex>&;

    /**
     * Returns the mutex for the allocations and tracking.
     *
     * @return the mutex
     */
    constexpr inline auto getMutex() -> std::recursive_mutex & {
        return mutex;
    }

    /**
     * Returns the mutex for the memory allocation infos.
     *
     * @return the mutex
     */
    constexpr inline auto getInfoMutex() -> std::mutex & {
        return infoMutex;
    }

    void changeMalloc(MallocInfo&& info) override;

    /**
     * Removes the allocation record associated with the given pointer.
     *
     * @param pointer the allocation pointer
     * @return a pair with a boolean indicating the success and optionally the
     * already deleted allocation record
     */
    auto removeMalloc(void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>> override;

    /**
     * @brief Attempts to remove the allocation record associated with the
     * given pointer.
     *
     * Does not search in the thread-local trackers.
     *
     * @param pointer the allocation pointer
     * @return whether a record was removed and the potentially existing record
     */
    auto maybeRemoveMalloc(void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>> override;

    /**
     * Prints a hint about the exceeded callstack size if it was exceeded.
     *
     * @param out the output stream to print to
     * @return the given output stream
     */
    auto maybeHintCallstackSize(std::ostream & out) const -> std::ostream &;

    /**
     * Returns the globally tracked allocations.
     *
     * @return the globally tracked allocations
     */
    constexpr inline auto getFragmentationInfos() const -> const decltype(infos)& {
        return infos;
    }

    /**
     * Sets whether the maximum callstack size has been exceeded during the printing.
     *
     * @param exceeded whether the maximum callstack size has been exceeded
     */
    constexpr inline void setCallstackSizeExceeded(const bool exceeded) {
        callstackSizeExceeded = exceeded;
    }

    /**
     * Returns the current instance of the statistics object.
     *
     * @return the current statistics instance
     */
    constexpr inline auto getStats() const -> const Stats & {
        return stats;
    }

    /**
     * Registers the given thread information.
     *
     * @param info the thread information to be registered
     */
    void addThread(ThreadInfo&& info);

    /**
     * Adds the calling thread.
     */
    void addThread();

    /**
     * Removes the thread with the given thread identifier.
     *
     * @param id the identifier of the thread to be removed
     */
    void removeThread(const std::thread::id& id = std::this_thread::get_id());

#ifdef __linux__
    /**
     * Sets the stack pointer for the calling thread.
     *
     * @param sp the stack pointer
     */
    inline void setSP(void* sp) {
        threads.at(std::this_thread::get_id()).setSP(sp);
    }
#endif

    /**
     * Returns the number of the given thread.
     *
     * @param id the @c std::thread::id
     * @return the number of the thread
     */
    auto getThreadId(const std::thread::id& id = std::this_thread::get_id()) -> unsigned long;

    /**
     * Returns the description for the given thread.
     *
     * @param id the thread number
     * @param thread the POSIX thread identifier
     * @return the thread description for the requested thread number
     */
    auto getThreadDescription(unsigned long id,
                              const std::optional<pthread_t>& thread = std::nullopt) -> const std::string&;

    /**
     * Returns the behaviour object associated with this instance.
     *
     * @return the associated behaviour object
     */
    constexpr inline auto getBehaviour() const -> const behaviour::Behaviour& {
        return behaviour;
    }

    /**
     * Returns whether at any time in the lifetime of the program multiple threads
     * have been used.
     *
     * @return whether multithreading was used
     */
    constexpr inline auto getIsThreaded() const -> bool {
        return isThreaded;
    }

    /**
     * Returns the thread-local key used to store the memory tracker for each
     * thread.
     *
     * @return the thread-local key used by this sanitizer
     */
    constexpr inline auto getTlsKey() const {
        return saniKey;
    }

    constexpr inline auto isAlwaysEqual() const {
        return alwaysEqual;
    }

    friend auto operator<<(std::ostream&, LSan&) -> std::ostream&;
};
}

#endif /* LeakSani_hpp */

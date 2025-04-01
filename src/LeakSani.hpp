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
#include <ostream>
#include <regex>
#include <set>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <pthread.h>

#include "MallocInfo.hpp"
#include "ThreadInfo.hpp"

#ifdef BENCHMARK
# include "timing.hpp"
#endif

#include "behaviour/Behaviour.hpp"
#include "helpers/LeakKindStats.hpp"
#include "helpers/Region.hpp"
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
    std::map<std::thread::id, ThreadInfo> threads;
    /** An object holding all statistics.                                               */
    Stats stats;
    /** The behaviour handling object.                                                  */
    behaviour::Behaviour behaviour;
    /** Indicates whether the set callstack size has been exceeded during the printing. */
    bool callstackSizeExceeded = false;
    std::optional<std::vector<suppression::Suppression>> suppressions;
    std::optional<std::vector<std::regex>> systemLibraries;
    std::unordered_map<unsigned long, std::string> threadDescriptions;
    /** The registered thread-local allocation trackers.                                */
    std::set<ATracker*> tlsTrackers;
    /** The mutex to manage the access to the registered thread-local trackers.         */
    std::mutex tlsTrackerMutex;
    const std::thread::id mainId = std::this_thread::get_id();
    bool isThreaded = false;

#ifdef BENCHMARK
    /** The registered timings of the allocations.                                      */
    std::map<timing::AllocType, timing::Timings> timingMap;
    /** The mutex to manage access to the allocation timings.                           */
    std::mutex timingMutex;
#endif

    auto classifyLeaks() -> LeakKindStats;
    void classifyRecord(MallocInfo& info, const LeakType& currentType, bool reclassify = false);
    void classifyObjC(std::deque<MallocInfo::Ref>& records);

    /**
     * Creates a thread-safe copy of the thread-local tracker list.
     *
     * @return the copy
     */
    auto copyTrackerList() -> decltype(tlsTrackers);

    auto findWithSpecials(void* ptr) -> decltype(infos)::iterator;

    void classifyLeaks(uintptr_t begin, uintptr_t end,
                       LeakType direct, LeakType indirect,
                       std::deque<MallocInfo::Ref>& directs, bool skipClassifieds = false,
                       const char* name = nullptr, const char* nameRelative = nullptr, bool reclassify = false);

    template<bool Four = false>
    constexpr inline void classifyPointerUnion(void* ptr, std::deque<MallocInfo::Ref>& directs,
                                               LeakType direct, LeakType indirect) {
        constexpr const auto order = Four ? 3 : 1;

        const auto& it = infos.find(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) & ~order));
        if (it != infos.end() && it->second.leakType > direct) {
            it->second.leakType = direct;
            classifyRecord(it->second, indirect);
            directs.push_back(it->second);
        }
    }

    void classifyClass(void* cls, std::deque<MallocInfo::Ref>& directs, LeakType direct, LeakType indirect);
    auto getGlobalRegionsAndTLVs(std::vector<std::pair<char*, char*>>& binaryFilenames) -> std::pair<std::vector<Region>, std::vector<std::tuple<const void*, const char*, const char*>>>;
    auto isSuppressed(const MallocInfo& info) -> bool;
    void applySuppressions(const std::deque<MallocInfo::Ref>& leaks);

protected:
    virtual inline void maybeAddToStats(const MallocInfo& info) final override {
        if (behaviour.statsActive()) {
            stats += info;
        }
    }

public:
    /** Indicates whether the allocation tracking has finished.                     */
    static std::atomic_bool finished;
    /** Indicates whether to ignore deallocations in the TLS deallocator.           */
    static std::atomic_bool preventDealloc;
    /** The thread-local storage key used for the thread-local allocation trackers. */
    const pthread_key_t saniKey;
    bool hasPrintedExit = false;

    LSan();
   ~LSan();

    LSan(const LSan&) = delete;
    LSan(LSan&&)      = delete;

    auto operator=(const LSan&) -> LSan& = delete;
    auto operator=(LSan&&)      -> LSan& = delete;

    inline static auto operator new(std::size_t count) -> void* {
        return real::malloc(count);
    }

    inline static void operator delete(void* ptr) {
        real::free(ptr);
    }

    /**
     * @brief Attempts to remove the allocation record associated with the given pointer.
     *
     * If no record is found in this instance, all registered trackers except
     * the given one are searched for the record.
     *
     * @param tracker the tracker to not be searched
     * @param pointer the pointer to the allocation
     * @return whether a record was removed and the potentially existing record
     */
    auto removeMalloc(ATracker* tracker, void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>>;

    /**
     * @brief Replaces the allocation record with the given one.
     *
     * If no record is found in this instance, all registered trackers except
     * the given one are searched for the record.
     *
     * @param tracker the allocation tracker to not be searched
     * @param info the new allocation record
     */
    void changeMalloc(ATracker* tracker, MallocInfo&& info);

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
     */
    void absorbLeaks(PoolMap<const void* const, MallocInfo>&& leaks);

    virtual void finish() final override;

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

    auto getSuppressions() -> const std::vector<suppression::Suppression>&;
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

    virtual void changeMalloc(MallocInfo&& info) final override;

    /**
     * Removes the allocation record associated with the given pointer.
     *
     * @param pointer the allocation pointer
     * @return a pair with a boolean indicating the success and optionally the already deleted allocation record
     */
    virtual auto removeMalloc(void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>> final override;

    /**
     * @brief Attempts to remove the allocation record associated with the given pointer.
     *
     * Does not search in the thread-local trackers.
     *
     * @param pointer the allocation pointer
     * @return whether a record was removed and the potentially existing record
     */
    virtual auto maybeRemoveMalloc(void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>> final override;

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
    constexpr inline void setCallstackSizeExceeded(bool exceeded) {
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

    void addThread(ThreadInfo&& info);
    void addThread();
    void removeThread(const std::thread::id& id = std::this_thread::get_id());

    auto getThreadId(const std::thread::id& id = std::this_thread::get_id()) -> unsigned long;
    auto getThreadDescription(unsigned long id, const std::optional<pthread_t>& thread = std::nullopt) -> const std::string&;

    /**
     * Returns the behaviour object associated with this instance.
     *
     * @return the associated behaviour object
     */
    constexpr inline auto getBehaviour() const -> const behaviour::Behaviour& {
        return behaviour;
    }

    constexpr inline auto getIsThreaded() const -> bool {
        return isThreaded;
    }

    friend auto operator<<(std::ostream&, LSan&) -> std::ostream&;
};
}

#endif /* LeakSani_hpp */

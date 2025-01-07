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
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include <pthread.h>
#include <lsan_internals.h>

#include "ATracker.hpp"
#include "MallocInfo.hpp"

#ifdef BENCHMARK
 #include "timing.hpp"
#endif

#include "helpers/LeakKindStats.hpp"
#include "behaviour/Behaviour.hpp"
#include "statistics/Stats.hpp"
#include "wrappers/realAlloc.hpp"
#include "suppression/Suppression.hpp"

namespace lsan {
/**
 * @brief This class manages everything this sanitizer is capable to do.
 *
 * It acts as an allocation tracker.
 */
class LSan final: public ATracker {
    std::set<pthread_key_t> keys;
    std::mutex tlsKeyMutex;
    std::map<std::pair<pthread_t, pthread_key_t>, const void*> tlsKeyValues;
    std::mutex tlsKeyValuesMutex;
    /** An object holding all statistics.                                               */
    Stats stats;
    /** The behaviour handling object.                                                  */
    behaviour::Behaviour behaviour;
    /** Indicates whether the set callstack size has been exceeded during the printing. */
    bool callstackSizeExceeded = false;
    std::optional<std::vector<suppression::Suppression>> suppressions;
    std::vector<std::pair<char*, char*>> binaryFilenames;
    /** The registered thread-local allocation trackers.                                */
    std::set<ATracker*> tlsTrackers;
    /** The mutex to manage the access to the registered thread-local trackers.         */
    std::mutex tlsTrackerMutex;

#ifdef BENCHMARK
    /** The registered timings of the allocations.                                      */
    std::map<timing::AllocType, timing::Timings> timingMap;
    /** The mutex to manage access to the allocation timings.                           */
    std::mutex timingMutex;
#endif

    auto classifyLeaks() -> LeakKindStats;
    void classifyRecord(MallocInfo& info, const LeakType& currentType);

    /**
     * Creates a thread-safe copy of the thread-local tracker list.
     *
     * @return the copy
     */
    auto copyTrackerList() -> decltype(tlsTrackers);

    inline auto findWithSpecials(void* ptr) -> decltype(infos)::iterator {
        auto toReturn = infos.find(ptr);
        if (toReturn == infos.end()) {
            toReturn = infos.find(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) - 2 * sizeof(void*)));
        }
        if (toReturn == infos.end()) {
            toReturn = infos.find(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) - sizeof(void*)));
        }
        return toReturn;
    }

    inline void classifyLeaks(uintptr_t begin, uintptr_t end,
                              LeakType direct, LeakType indirect,
                              std::deque<MallocInfo::Ref>& directs, bool skipClassifieds = false,
                              const char* name = nullptr, const char* nameRelative = nullptr) {
        for (uintptr_t it = begin; it < end; it += sizeof(uintptr_t)) {
            const auto& record = infos.find(*reinterpret_cast<void**>(it));
            if (record == infos.end() || record->second.deleted || (skipClassifieds && record->second.leakType != LeakType::unclassified)) {
                continue;
            }
            if (record->second.leakType > direct) {
                record->second.leakType = direct;
                record->second.imageName.first = name;
                record->second.imageName.second = nameRelative;
                directs.push_back(record->second);
            }
            classifyRecord(record->second, indirect);
        }
    }

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

    inline void classifyClass(void* cls, std::deque<MallocInfo::Ref>& directs, LeakType direct, LeakType indirect) {
        auto classWords = reinterpret_cast<void**>(cls);
        auto cachePtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(classWords[2]) & ((uintptr_t) 1 << 48) - 1);
        const auto& cacheIt = infos.find(cachePtr);
        if (cacheIt != infos.end() && cacheIt->second.leakType > direct) {
            cacheIt->second.leakType = direct;
            classifyRecord(cacheIt->second, indirect);
            directs.push_back(cacheIt->second);
        }

        auto ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(classWords[4]) & 0x0f007ffffffffff8UL);
        const auto& it = infos.find(ptr);
        if (it != infos.end()) {
            if (it->second.leakType > direct) {
                it->second.leakType = direct;
                classifyRecord(it->second, indirect);
                directs.push_back(it->second);
            }

            auto rwStuff = reinterpret_cast<void**>(it->second.pointer);
            auto ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(rwStuff[1]) & ~1);
            const auto& it = infos.find(ptr);
            if (it != infos.end()) {
                if (it->second.leakType > direct) {
                    it->second.leakType = direct;
                    classifyRecord(it->second, indirect);
                    directs.push_back(it->second);
                }
                if (it->second.size >= 4 * sizeof(void*)) {
                    const auto ptrArr = reinterpret_cast<void**>(it->second.pointer);
                    for (unsigned char i = 1; i < 4; ++i) {
                        classifyPointerUnion<true>(ptrArr[i], directs, direct, indirect);
                    }
                }
            }
        }
    }

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

    void classifyStackLeaksShallow();
    void addTLSKey(const pthread_key_t& key);
    auto removeTLSKey(const pthread_key_t& key) -> bool;

    auto hasTLSKey(const pthread_key_t& key) -> bool;

    virtual auto addTLSValue(const pthread_key_t& key, const void* value) -> bool final override;

    /**
     * Returns the behaviour object associated with this instance.
     *
     * @return the associated behaviour object
     */
    constexpr inline auto getBehaviour() const -> const behaviour::Behaviour& {
        return behaviour;
    }

    friend auto operator<<(std::ostream&, LSan&) -> std::ostream&;
};
}

#endif /* LeakSani_hpp */
